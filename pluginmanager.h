#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#pragma push_macro("slots")
#undef slots
#include "Python.h"
#pragma pop_macro("slots")

#include <QObject>
#include "plugin.h"

#include "QLogger.h"
#include "QLoggerWriter.h"
#include "QFileSystemWatcher"

typedef std::function<void(std::string)> stdout_write_type;
void set_stdout(stdout_write_type write);
void reset_stdout();

class PluginManager : public QObject
{
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager();

    QList<Plugin*> plugins() const { return m_pluginlist; }

signals:
    void pluginLoaded(const QString& t_path);
    void pluginUnLoaded(const QString& t_path);

public slots:
    void unloadAll();

private slots:
    void onLoadComplete(const QString& t_modulepath);
    void onUnloadComplete(const QString& t_modulepath);
    void onFileChanged(const QString& filePath);
    void onFolderChanged(const QString& folderPath);

private:
    QList<Plugin*> m_pluginlist;
    PyThreadState* m_pythreadstate;
    QFileSystemWatcher *m_fileWatcher;
    QString m_pluginspath;
    QString m_threadspath;

    void loadPluginFiles(const QString& t_path, bool t_isthread);
};

#endif // PLUGINMANAGER_H
