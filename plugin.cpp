#include <QCoreApplication>
#include <QDomDocument>

#include "plugin.h"
#include "QLogger.h"
#include "pndatabaseobjects.h"
#include "pnsettings.h"

Plugin::Plugin(QObject *parent, bool isthread)
    : QObject{parent}
{
    m_pythonworker = new PythonWorker();

    if (isthread)
    {
        m_thread = new QThread();

        QLog_Debug(PLUGINSMOD, QString("Created plugin thread %1 for PythonWorker.").arg((quintptr)m_thread, QT_POINTER_SIZE * 2, 16, QChar('0')));

        m_pythonworker->moveToThread(m_thread);

        if (m_thread != m_pythonworker->thread())
        {
            QLog_Debug(PLUGINSMOD, "FAILED TO MOVE THREAD TO NEW PYTHONWORKER.");
        }
    }

    // send commands to python worker
    QObject::connect(this, &Plugin::sendMethodXml, m_pythonworker, &PythonWorker::sendMethodXml);
    QObject::connect(this, &Plugin::sendMethod, m_pythonworker, &PythonWorker::sendMethod);
    QObject::connect(this, &Plugin::loadModule, m_pythonworker, &PythonWorker::loadModule);
    QObject::connect(this, &Plugin::unloadModule, m_pythonworker, &PythonWorker::unloadModule);
    QObject::connect(this, &Plugin::reloadModule, m_pythonworker, &PythonWorker::reloadModule);

    // process returned signals
    QObject::connect(m_pythonworker, &PythonWorker::returnXml, this, &Plugin::onReturnedXml);
    QObject::connect(m_pythonworker, &PythonWorker::loadComplete, this, &Plugin::onLoadComplete);
    QObject::connect(m_pythonworker, &PythonWorker::unloadComplete, this, &Plugin::onUnLoadComplete);

    if (isthread)
        m_thread->start();
}

Plugin::~Plugin()
{
    // TODO: This doesn't work because the signal won't get to the queue in time.
    emit unloadModule();

    QCoreApplication::processEvents();

    if (m_thread)
    {
        // give the thead 15 seconds to quit
        m_thread->quit();
        m_thread->wait(15000);
    }

    // send commands to python worker
    QObject::disconnect(this, &Plugin::sendMethodXml, m_pythonworker, &PythonWorker::sendMethodXml);
    QObject::disconnect(this, &Plugin::sendMethod, m_pythonworker, &PythonWorker::sendMethod);
    QObject::disconnect(this, &Plugin::loadModule, m_pythonworker, &PythonWorker::loadModule);
    QObject::disconnect(this, &Plugin::unloadModule, m_pythonworker, &PythonWorker::unloadModule);
    QObject::disconnect(this, &Plugin::reloadModule, m_pythonworker, &PythonWorker::reloadModule);

    // process returned signals
    QObject::disconnect(m_pythonworker, &PythonWorker::returnXml, this, &Plugin::onReturnedXml );
    QObject::disconnect(m_pythonworker, &PythonWorker::loadComplete, this, &Plugin::onLoadComplete);
    QObject::disconnect(m_pythonworker, &PythonWorker::unloadComplete, this, &Plugin::onUnLoadComplete);

    delete m_pythonworker;

    if (m_thread)
        delete m_thread;
}

void Plugin::callXmlMethod(const QString& t_method, const QString& t_xml)
{
    emit sendMethodXml(t_method, t_xml);
}

void Plugin::callMethod(const QString& t_method)
{
    emit sendMethod(t_method);
}

bool Plugin::hasMethod(const QString& t_method) const
{
    return m_plugin.hasMember(t_method);
}

void Plugin::setEnabled(const bool t_enabled)
{
    // TODO: save the enabled disabled value in settings

    if (t_enabled && !m_enabled)
    {
        m_enabled = true;

        if (!m_loaded)
        {
            emit loadModule(m_modulepath);
        }
    }
    else if (!t_enabled && m_enabled)
    {
        m_enabled = false;
        if (m_loaded)
        {
            emit unloadModule();
        }
    }
}

void Plugin::onReturnedXml(const QString& t_xml)
{
    QLog_Debug(PLUGINSMOD, QString("Plugin: %1 Returned Xml: %2").arg(m_modulepath, t_xml));

    QDomDocument xmldoc;

    xmldoc.setContent(t_xml);

    PNDatabaseObjects dbo;
    dbo.openDatabase(global_DBObjects.getDatabaseFile());
    dbo.importXMLDoc(xmldoc);
    dbo.closeDatabase();
}

void Plugin::onLoadComplete(const PythonPlugin& t_plugin)
{
    m_plugin = t_plugin;

    m_loaded = true;

    QString logMsg;

    QString members;
    for (QString s : m_plugin.members())
    {
        if (!members.isEmpty())
            members += ", ";
        members += s;
    }

    if (!members.isEmpty())
        members = "Provided Function(s): " + members;

    emit moduleLoaded(m_modulepath);

    QLog_Debug(PLUGINSMOD, QString("Loaded Plugin: %1 %2").arg(m_plugin.name(), logMsg));
}

void Plugin::onUnLoadComplete()
{
    QLog_Debug(PLUGINSMOD, QString("Unloaded Plugin: %1").arg(m_plugin.name()));

    m_loaded = false;

    emit moduleUnloaded(m_modulepath);
}

void Plugin::loadPlugin(const QString& t_module)
{
    m_modulepath = t_module;

    emit loadModule(t_module);
}

void Plugin::unloadPlugin()
{
    emit unloadModule();
}

void Plugin::reloadPlugin()
{
    emit reloadModule();
}

