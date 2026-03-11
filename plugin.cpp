#include <QCoreApplication>
#include <QDomDocument>

#include "plugin.h"
#include "QLogger.h"
#include "databaseobjects.h"
#include "appsettings.h"

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

void Plugin::callXmlMethod(const QString& method, const QString& xml, const QString& parameter)
{
    emit sendMethodXml(method, xml, parameter);
}

void Plugin::callMethod(const QString& method, const QString& parameter)
{
    emit sendMethod(method, parameter);
}

bool Plugin::hasMethod(const QString& method) const
{
    return m_plugin.hasMember(method);
}

void Plugin::setEnabled(const bool enabled)
{
    if (enabled && !m_enabled)
    {
        m_enabled = true;

        if (!m_loaded)
        {
            emit loadModule(m_modulepath);
        }
    }
    else if (!enabled && m_enabled)
    {
        m_enabled = false;
        if (m_loaded)
        {
            emit unloadModule();
        }
    }
}

void Plugin::onReturnedXml(const QString& xml)
{
#ifdef QT_DEBUG
    QLog_Debug(DEBUGLOG, QString("Plugin: %1 Returned Xml: %2").arg(m_modulepath, xml));
#endif

    if (!xml.isEmpty())
    {
        QDomDocument xmldoc;

        xmldoc.setContent(xml);

        if (!global_DBObjects.getDatabaseFile().isEmpty())
        {
            global_DBObjects.importXMLDoc(xmldoc);
            global_DBObjects.updateDisplayData();
        }
#ifdef QT_DEBUG
        else
        {
            QLog_Debug(DEBUGLOG, QString("Plugin: %1\nDatabase was already closed.  XML was not processed.\nReturned Xml: %2").arg(m_modulepath, xml));
        }
#endif
    }
}

void Plugin::onLoadComplete(const PythonPlugin& plugin)
{
    m_plugin = plugin;

    m_pluginname = plugin.name(); // keep the plugin name handy for displays

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

void Plugin::loadPlugin(const QString& module)
{
    m_modulepath = module;

    emit loadModule(module);
}

void Plugin::unloadPlugin()
{
    emit unloadModule();
}

void Plugin::reloadPlugin()
{
    emit reloadModule();
}

