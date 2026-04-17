// Copyright (C) 2025, 2026 Paul McKinney
#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#ifdef _DEBUG
#pragma push_macro("slots")
#undef slots
#undef _DEBUG
#include "Python.h"
#define _DEBUG
#pragma pop_macro("slots")
#else
#pragma push_macro("slots")
#undef slots
#include "Python.h"
#pragma pop_macro("slots")
#endif

#include <QObject>
#include "plugin.h"

#include "QLogger.h"
#include "QLoggerWriter.h"
#include <QFileSystemWatcher>
#include <QFileInfo>

typedef std::function<void(std::string)> stdout_write_type;
void set_stdout(stdout_write_type write);
void reset_stdout();

class PluginManager : public QObject
{
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager();

    const QList<Plugin*>& plugins() const { return m_pluginlist; }
    void forceReload(const QString& module);
    int loadedCount();
    void stopWatcher() {
        if (m_fileWatcher) {
            m_fileWatcher->removePaths(m_fileWatcher->files());
            m_fileWatcher->removePaths(m_fileWatcher->directories());
        }
    }

signals:
    void pluginLoaded(const QString& path);
    void pluginUnLoaded(const QString& path);
    void pluginForceReload(const QString& module);
    void pluginRefreshRequest();

public slots:
    void unloadAll();

private slots:
    void onLoadComplete(const QString& modulepath);
    void onUnloadComplete(const QString& modulepath);
    void onFileChanged(const QString& filepath);
    void onFolderChanged(const QString& folderPath);
    void onForceReload(const QString& module);

private:
    QList<Plugin*> m_pluginlist;
    PyThreadState* m_pythreadstate;
    QFileSystemWatcher* m_fileWatcher;
    QString m_pluginspath;
    QString m_threadspath;
    QString m_userPluginspath;
    QString m_userThreadspath;

    void loadPluginFiles(const QString& path, bool isthread);
};

#endif // PLUGINMANAGER_H
