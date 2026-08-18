// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef LOGVIEWERCONTROLLER_H
#define LOGVIEWERCONTROLLER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QTimer>

class QQmlEngine;
class QJSEngine;
class QThread;
class QFileSystemWatcher;

// LogFileLoader — background worker that streams one log file to the viewer.
//
// Ported from the Widgets LogViewer's LogLoader: on first read it emits the tail
// of the file (so the newest lines appear immediately), then walks backwards in
// 8 KB chunks on a timer to fill the head, and tails new writes via a
// QFileSystemWatcher. Runs on its own QThread with LowPriority so loading a large
// log never blocks the UI.
class LogFileLoader : public QObject
{
    Q_OBJECT

signals:
    // Prepend to the top of the editor (head fill, oldest-first backfill).
    void topContentLoaded(const QString& filePath, const QString& content);
    // Append to the bottom of the editor (tail + live updates).
    void contentLoaded(const QString& filePath, const QString& content);

public slots:
    void loadFile();
    void timerUpdate();
    void onFileChanged(const QString& filePath);

public:
    explicit LogFileLoader(const QString& filePath);
    ~LogFileLoader() override;

private:
    qint64 m_topPosition = -1;
    qint64 m_lastPosition = 0;
    QTimer* m_topLoadTimer = nullptr;
    QFileSystemWatcher* m_fileWatcher = nullptr;
    QString m_filePath;
    bool m_isLoading = false;
};

// LogViewerController — QML singleton bridge for the desktop Log Viewer window.
//
// Discovers *.log files in the app's logs folder, spins a LogFileLoader thread
// per file, and forwards their content to QML via signals. Watches the folder so
// files created later (e.g. once a log level first fires) appear automatically.
// The QML window (LogViewerWindow.qml) builds a tab per file and appends/prepends
// text as the signals arrive.
class LogViewerController : public QObject
{
    Q_OBJECT

public:
    explicit LogViewerController(QObject* parent = nullptr);
    ~LogViewerController() override;

    static LogViewerController* create(QQmlEngine* engine, QJSEngine* scriptEngine);

    // Begin (idempotent) — enumerate existing logs, start loaders, watch folder.
    Q_INVOKABLE void start();
    // Stop all loader threads (called on application shutdown).
    Q_INVOKABLE void stop();
    // Delete the given log file and drop its tab.
    Q_INVOKABLE void clearLog(const QString& filePath);
    // Absolute path of the logs folder (created if missing).
    Q_INVOKABLE QString logFolder() const;

signals:
    // A new log file was discovered — QML should create a tab for it.
    void logFileOpened(const QString& filePath, const QString& fileName);
    // Append text to the file's editor (tail / live updates).
    void contentAppended(const QString& filePath, const QString& content);
    // Prepend text to the file's editor (head backfill).
    void contentPrepended(const QString& filePath, const QString& content);
    // The file was cleared/removed — QML should drop its tab.
    void logFileClosed(const QString& filePath);

private slots:
    void onFolderChanged(const QString& folderPath);

private:
    void openFile(const QString& filePath, const QString& fileName);

    QString m_folderPath;
    QMap<QString, QThread*> m_loadingThreads;
    QFileSystemWatcher* m_folderWatcher = nullptr;
    bool m_started = false;
};

#endif // LOGVIEWERCONTROLLER_H
