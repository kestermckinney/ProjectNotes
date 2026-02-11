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
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, QString("Created plugin thread %1 for PythonWorker.").arg((quintptr)m_thread, QT_POINTER_SIZE * 2, 16, QChar('0')));
#endif
        m_pythonworker->moveToThread(m_thread);

#ifdef QT_DEBUG
        if (m_thread != m_pythonworker->thread())
        {
            QLog_Debug(DEBUGLOG, "FAILED TO MOVE THREAD TO NEW PYTHONWORKER.");
        }
#endif
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
    {
        delete m_thread;
        m_thread = nullptr;
    }
}

void Plugin::callXmlMethod(const QString& t_method, const QString& t_xml, const QString& t_parameter)
{
    emit sendMethodXml(t_method, t_xml, t_parameter);
}

void Plugin::callMethod(const QString& t_method, const QString& t_parameter)
{
    emit sendMethod(t_method, t_parameter);
}

bool Plugin::hasMethod(const QString& t_method) const
{
    return m_plugin.hasMember(t_method);
}

void Plugin::setEnabled(const bool t_enabled)
{
    // TODO: VER 4.1 save the enabled disabled value in settings

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
#ifdef QT_DEBUG
    QLog_Debug(DEBUGLOG, QString("Plugin: %1 Returned Xml: %2").arg(m_modulepath, t_xml));
#endif

    if (!t_xml.isEmpty())
    {
        QDomDocument xmldoc;

        xmldoc.setContent(t_xml);

        if (!global_DBObjects.getDatabaseFile().isEmpty())
        {
            global_DBObjects.importXMLDoc(xmldoc);
            global_DBObjects.updateDisplayData();
        }
        else
        {
            QLog_Info(APPLOG, QString("Database was already closed.  XML was not processed."));
        }
    }
}

void Plugin::onLoadComplete(const PythonPlugin& t_plugin)
{
    m_plugin = t_plugin;

    m_pluginname = t_plugin.name(); // keep the plugin name handy for displays

    m_loaded = true;

    emit moduleLoaded(m_modulepath);
}

void Plugin::onUnLoadComplete()
{
#ifdef QT_DEBUG
    QLog_Debug(DEBUGLOG, QString("Unloaded Plugin: %1").arg(m_plugin.name()));
#endif
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

