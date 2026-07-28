// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "LogViewerController.h"
#include "DesktopAppController.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QThread>

// ── LogFileLoader ──────────────────────────────────────────────────────────────
// A near-verbatim port of the Widgets LogViewer::LogLoader so the QML app gets the
// exact same proven background-loading behaviour for large logs.

LogFileLoader::LogFileLoader(const QString& filePath)
{
    m_filePath = filePath;
}

LogFileLoader::~LogFileLoader()
{
    if (m_fileWatcher) {
        delete m_fileWatcher;
        m_fileWatcher = nullptr;
    }

    if (m_topLoadTimer) {
        disconnect(m_topLoadTimer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
        m_topLoadTimer->stop();
        delete m_topLoadTimer;
        m_topLoadTimer = nullptr;
    }
}

void LogFileLoader::onFileChanged(const QString& filePath)
{
    // Re-add the path in case the platform dropped the watch after the event.
    if (m_fileWatcher && !m_fileWatcher->files().contains(filePath))
        m_fileWatcher->addPath(filePath);

    // Small debounce to let the writer finish.
    QTimer::singleShot(10, this, [this]() { loadFile(); });
}

void LogFileLoader::loadFile()
{
    if (m_isLoading)
        return;                    // prevent overlapping calls

    m_isLoading = true;

    // Create file watcher once (on the worker thread).
    if (m_fileWatcher == nullptr) {
        m_fileWatcher = new QFileSystemWatcher();
        m_fileWatcher->addPath(m_filePath);
        connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this,
                &LogFileLoader::onFileChanged, Qt::DirectConnection);
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_isLoading = false;
        return;
    }

    // Guard against truncation / rotation.
    if (m_lastPosition > file.size())
        m_lastPosition = 0;

    if (m_lastPosition == 0) {
        // First load — start near the end.
        qint64 topOfChunk = qMax(file.size() - 8192, 0LL);
        if (m_topPosition == -1)
            m_topPosition = topOfChunk;

        file.seek(topOfChunk);

        if (topOfChunk > 0 && m_topLoadTimer == nullptr) {
            m_topLoadTimer = new QTimer();
            connect(m_topLoadTimer, SIGNAL(timeout()), this, SLOT(timerUpdate()),
                    Qt::DirectConnection);
            m_topLoadTimer->start(100);
        }
    } else {
        file.seek(m_lastPosition);
    }

    // Read in chunks.
    while (!file.atEnd() && !QThread::currentThread()->isInterruptionRequested()) {
        QByteArray chunk = file.read(8192);
        if (!chunk.isEmpty()) {
            emit contentLoaded(m_filePath, QString::fromUtf8(chunk));
            m_lastPosition = file.pos();        // update incrementally
            QThread::msleep(30);
        }
    }

    file.close();
    m_isLoading = false;
}

void LogFileLoader::timerUpdate()
{
    // If we have walked back to the start of the file, stop the head-fill timer.
    if (m_topPosition == 0)
        if (m_topLoadTimer) {
            m_topLoadTimer->stop();
            delete m_topLoadTimer;
            m_topLoadTimer = nullptr;
            return;
        }

    QFile file(m_filePath);
    if (file.open(QIODevice::ReadOnly)) {
        qint64 bottomOfChunk = m_topPosition;
        qint64 topOfChunk = qMax(bottomOfChunk - 8192, (qint64)0);
        qint64 readSize = bottomOfChunk - topOfChunk;

        if (readSize > 0 && !QThread::currentThread()->isInterruptionRequested()) {
            file.seek(topOfChunk);
            QByteArray chunk = file.read(readSize);
            if (!chunk.isEmpty())
                emit topContentLoaded(m_filePath, QString::fromUtf8(chunk));
        }

        m_topPosition = topOfChunk;
        file.close();
    }
}

// ── LogViewerController ─────────────────────────────────────────────────────────

LogViewerController::LogViewerController(QObject* parent) : QObject(parent)
{
}

LogViewerController::~LogViewerController()
{
    stop();
}

LogViewerController* LogViewerController::create(QQmlEngine*, QJSEngine*)
{
    return new LogViewerController();
}

QString LogViewerController::logFolder() const
{
    const QString path = DesktopAppController::dataLocation() + "/logs";
    QDir dir(path);
    if (!dir.exists())
        dir.mkpath(path);
    return path;
}

void LogViewerController::start()
{
    if (m_started)
        return;                    // window persists across opens; load once
    m_started = true;

    m_folderPath = logFolder();

    // Load any log files that already exist.
    onFolderChanged(m_folderPath);

    // Watch the folder so files created later (a log level's first write) appear.
    m_folderWatcher = new QFileSystemWatcher(this);
    m_folderWatcher->addPath(m_folderPath);
    connect(m_folderWatcher, &QFileSystemWatcher::directoryChanged, this,
            &LogViewerController::onFolderChanged);
}

void LogViewerController::stop()
{
    for (QThread* t : m_loadingThreads) {
        if (!t)
            continue;
        t->requestInterruption();
        t->quit();
        t->wait(5000);
        // The thread deletes itself and its loader via QThread::finished.
    }
    m_loadingThreads.clear();
}

void LogViewerController::onFolderChanged(const QString& folderPath)
{
    QDir dir(folderPath);
    dir.setNameFilters(QStringList() << "*.log");
    const QFileInfoList files = dir.entryInfoList(QDir::Files);

    for (const QFileInfo& fileInfo : files) {
        const QString filePath = fileInfo.absoluteFilePath();

        // Only track new files, capped at 20 tabs (matches the Widgets viewer).
        if (!m_loadingThreads.contains(filePath) && m_loadingThreads.size() < 20)
            openFile(filePath, fileInfo.fileName());
    }
}

void LogViewerController::openFile(const QString& filePath, const QString& fileName)
{
    // Tell QML to build the tab before content starts streaming in.
    emit logFileOpened(filePath, fileName);

    QThread* thread = new QThread();
    m_loadingThreads[filePath] = thread;

    LogFileLoader* loader = new LogFileLoader(filePath);
    loader->moveToThread(thread);

    connect(thread, &QThread::started, loader, &LogFileLoader::loadFile);
    connect(thread, &QThread::finished, loader, &LogFileLoader::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    // Cross-thread (auto/queued) → the viewer's signals fire on the GUI thread.
    connect(loader, &LogFileLoader::contentLoaded, this,
            &LogViewerController::contentAppended);
    connect(loader, &LogFileLoader::topContentLoaded, this,
            &LogViewerController::contentPrepended);

    thread->start();
    thread->setPriority(QThread::LowPriority);
}

void LogViewerController::clearLog(const QString& filePath)
{
    if (filePath.isEmpty())
        return;

    // Stop the loader for this file so it releases the handle/watch before delete.
    QThread* thread = m_loadingThreads.take(filePath);
    if (thread) {
        thread->requestInterruption();
        thread->quit();
        thread->wait(3000);
    }

    QFile::remove(filePath);
    emit logFileClosed(filePath);
}
