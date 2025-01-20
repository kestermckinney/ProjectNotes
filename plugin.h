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
    bool enabled() const { return m_enabled; }
    void setEnabled(const bool t_enabled);

    void loadPlugin(const QString& t_module);
    void unloadPlugin();
    void reloadPlugin();

    void callXmlMethod(const QString& t_method, const QString& t_xml);
    void callMethod(const QString& t_method);
    bool hasMethod(const QString& t_method) const;

    QString modulepath() const { return m_modulepath; }

signals:
    void loadModule(const QString& t_module);
    void unloadModule();
    void reloadModule();

    void sendMethodXml(const QString& t_method, const QString& t_xml);
    void sendMethod(const QString& t_method);

    void moduleUnloaded(const QString& t_modulepath);

private slots:
    void onReturnedXml(const QString& t_xml);
    void onLoadComplete(const PythonPlugin& t_plugin); // return all needed values
    void onUnLoadComplete();

private:
    PythonWorker* m_pythonworker;
    QThread* m_thread = nullptr;
    PythonPlugin m_plugin;
    QString m_modulepath;

    bool m_enabled = false;
    bool m_loaded = false;
};

#endif // PLUGIN_H
