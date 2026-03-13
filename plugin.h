#ifndef PLUGIN_H
#define PLUGIN_H

#include <QObject>

#include "pythonworker.h"

class Plugin : public QObject
{
    Q_OBJECT
public:
    explicit Plugin(QObject *parent = nullptr, bool isthread = false);
    ~Plugin();

    PythonPlugin pythonplugin() const { return m_plugin; }
    bool loaded() const { return m_loaded; }

    void loadPlugin(const QString& module);
    void unloadPlugin();
    void reloadPlugin();

    void callXmlMethod(const QString& method, const QString& xml, const QString& parameter);
    void callMethod(const QString& method, const QString& parameter);
    bool hasMethod(const QString& method) const;

    QString modulepath() const { return m_modulepath; }
    QString pluginname() const { return m_pluginname; }

signals:
    void loadModule(const QString& module);
    void unloadModule();
    void reloadModule();

    void sendMethodXml(const QString& method, const QString& xml, const QString& parameter);
    void sendMethod(const QString& method, const QString& parameter);

    void moduleUnloaded(const QString& modulepath);
    void moduleLoaded(const QString& modulepath);

private slots:
    void onReturnedXml(const QString& xml);
    void onLoadComplete(const PythonPlugin& plugin); // return all needed values
    void onUnLoadComplete();

private:
    PythonWorker* m_pythonworker;
    QThread* m_thread = nullptr;
    PythonPlugin m_plugin;
    QString m_modulepath;
    QString m_pluginname;

    bool m_enabled = false;
    bool m_loaded = false;
};

#endif // PLUGIN_H
