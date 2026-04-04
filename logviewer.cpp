// Copyright (C) 2025, 2026 Paul McKinney
#include "logviewer.h"
#include "appsettings.h"
#include <QStandardPaths>
#include <QPushButton>
#include <QTextBlock>
#include <QWindow>
#include <QApplication>

LogLoader::LogLoader(const QString& filePath)
{
    m_filePath = filePath;
}


LogLoader::~LogLoader()
{
    if (m_pollTimer) {
        m_pollTimer->stop();
        delete m_pollTimer;
        m_pollTimer = nullptr;
    }

    if (m_topLoadTimer)
    {
        disconnect(m_topLoadTimer, SIGNAL(timeout()), this, SLOT(timerUpdate()));

        m_topLoadTimer->stop();
        delete m_topLoadTimer;
        m_topLoadTimer = nullptr;
    }
}

void LogLoader::onFileChanged(const QString &filePath)
{
    loadFile();
}

void LogLoader::onPollTimer()
{
    QFileInfo fi(m_filePath);
    if (!fi.exists()) return;
    const qint64 newSize = fi.size();
    const QDateTime newMod = fi.lastModified();

    if (newSize != m_pollLastSize || newMod != m_pollLastModified) {
        m_pollLastSize     = newSize;
        m_pollLastModified = newMod;
        onFileChanged(m_filePath);
    }
}

void LogLoader::loadFile()
{
    // Poll timer must be created on the worker thread (same pattern as m_topLoadTimer)
    if (m_pollTimer == nullptr) {
        m_pollLastSize     = QFileInfo(m_filePath).size();
        m_pollLastModified = QFileInfo(m_filePath).lastModified();
        m_pollTimer = new QTimer();  // no parent — lives on worker thread
        connect(m_pollTimer, &QTimer::timeout, this, &LogLoader::onPollTimer, Qt::DirectConnection);
        m_pollTimer->start(500);
    }

    // just load the bottom at first
    QFile file(m_filePath);
    if (file.open(QIODevice::ReadOnly))
    {
        // we need to start with the end of the file
        if (m_lastPosition == 0)
        {
            qint64 topofchunck = qMax(file.size() - 8192, 0);

            // on the first call
            if (m_topPosition == -1)
            {
                m_topPosition = topofchunck;
            }

            file.seek(topofchunck);

            // if we aren't already loading the top of file start the laod
            if (topofchunck > 0 && m_topLoadTimer == nullptr)
            {
                m_topLoadTimer = new QTimer();

                connect(m_topLoadTimer, SIGNAL(timeout()), this, SLOT(timerUpdate()), Qt::DirectConnection);

                m_topLoadTimer->start(100);
            }
        }
        else
        {
            // Guard against file rotation/truncation (e.g. log file renamed at max size) start akk iver
            if (m_lastPosition > file.size())
                m_lastPosition = 0;

            file.seek(m_lastPosition);
        }

        // Read in chunks to keep UI responsive
        while (!file.atEnd() && !QThread::currentThread()->isInterruptionRequested())
        {
            QByteArray chunk = file.read(8192);
            if (!chunk.isEmpty())
            {
                emit contentLoaded(m_filePath, QString::fromUtf8(chunk));
                QThread::msleep(50);
            }
        }

        m_lastPosition = file.pos();

        file.close();
    }
}

void LogLoader::timerUpdate()
{
    // if all done loading stop the timer
    if (m_topPosition == 0)
        if (m_topLoadTimer)
        {
            m_topLoadTimer->stop();
            delete m_topLoadTimer;
            m_topLoadTimer = nullptr;

            return;
        }

    QFile file(m_filePath);
    if (file.open(QIODevice::ReadOnly))
    {
        // we need to start with the end of the file
        qint64 bottomofchunck = m_topPosition;
        qint64 topofchunck = qMax(bottomofchunck - 8192, (qint64)0);
        qint64 readsize = bottomofchunck - topofchunck;

        // Read in chunks to keep UI responsive
        if (readsize > 0 && !QThread::currentThread()->isInterruptionRequested())
        {
            file.seek(topofchunck);
            QByteArray chunk = file.read(readsize);
            if (!chunk.isEmpty())
                emit topContentLoaded(m_filePath, QString::fromUtf8(chunk));
        }

        m_topPosition = topofchunck;

        file.close();
    }
}


LogViewer::LogViewer(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Log Viewer");
    setMinimumSize(300, 200);

    m_folderPath = getLogFileLocation();

    // Initialize UI
    m_tabWidget = new QTabWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_tabWidget);

    m_clearlog = new QPushButton(this);
    m_clearlog->setText("Clear Log");
    m_clearlog->setFixedHeight(24);
    m_clearlog->setFixedWidth(75);

    m_close = new QPushButton(this);
    m_close->setText("Close");
    m_close->setFixedHeight(24);
    m_close->setFixedWidth(75);

    QHBoxLayout* buttonLayout = new QHBoxLayout;

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_clearlog, 0, Qt::AlignRight);
    buttonLayout->addWidget(m_close, 0, Qt::AlignRight);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    connect(m_clearlog, &QPushButton::clicked, this, &LogViewer::onClearLog);
    connect(m_close, &QPushButton::clicked, this, &QDialog::close);

    // Load existing log files and seed the known-files set
    onFolderChanged(m_folderPath);
    for (const QString& path : m_fileTabs.keys())
        m_knownLogFiles.insert(path);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(1500);
    connect(m_pollTimer, &QTimer::timeout, this, &LogViewer::onPollTimer);
    m_pollTimer->start();
}

void LogViewer::showEvent(QShowEvent *event)
{
    global_Settings.getWindowState(objectName(), this);
    QDialog::showEvent(event);
}

void LogViewer::closeEvent(QCloseEvent *e)
{
    for (QThread* t : m_loadingThreads)
    {
        t->requestInterruption();
    }

    QDialog::closeEvent(e);
}

void LogViewer::hideEvent(QHideEvent *event)
{
    global_Settings.setWindowState(objectName(), this);
    emit closed();
    QDialog::hideEvent(event);
}

LogViewer::~LogViewer()
{
    for (QThread* t : m_loadingThreads)
    {
        t->quit();
        t->wait(5000);
        // don't delete t - deleteLater handles it
    }

    m_loadingThreads.clear();

    disconnect(m_clearlog, &QPushButton::clicked, this, &LogViewer::onClearLog);
    disconnect(m_close, &QPushButton::clicked, this, &QDialog::close);
}

QString LogViewer::getLogFileLocation()
{
    QString logFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";

    QDir dir(logFilePath);

    if (!dir.exists())
    {
        dir.mkpath(logFilePath);
    }

    return logFilePath;
}

void LogViewer::onInsertContent(const QString& filePath, const QString& content)
{
    if (m_fileTabs.contains(filePath))
    {
        QPlainTextEdit *editor = m_fileTabs[filePath];

        bool wasatbottom = (editor->verticalScrollBar()->maximum() == editor->verticalScrollBar()->value());

        editor->setUpdatesEnabled(false);
        QTextCursor cursor = editor->textCursor();

        int lines = editor->verticalScrollBar()->maximum();
        int scrollvalue = editor->verticalScrollBar()->value();

        cursor.movePosition(QTextCursor::Start);
        cursor.insertText(content);

        int newlines = editor->verticalScrollBar()->maximum();

        // hold at bottom and show scrolling if it was
        if (wasatbottom)
            editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->maximum());
        else
        {
            editor->verticalScrollBar()->setValue(scrollvalue + (newlines - lines));
        }

        editor->setUpdatesEnabled(true);
    }
}

void LogViewer::onUpdateContent(const QString& filePath, const QString& content)
{
    if (m_fileTabs.contains(filePath))
    {
        QPlainTextEdit *editor = m_fileTabs[filePath];

        bool wasatbottom = (editor->verticalScrollBar()->maximum() == editor->verticalScrollBar()->value());

        editor->setUpdatesEnabled(false);

        int lines = editor->verticalScrollBar()->maximum();
        int scrollvalue = editor->verticalScrollBar()->value();

        QTextCursor cursor = editor->textCursor();

        cursor.movePosition(QTextCursor::End);
        cursor.insertText(content);

        int newlines = editor->verticalScrollBar()->maximum();

        // hold at bottom and show scrolling if it was
        if (wasatbottom)
            editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->maximum());
        else
        {
            editor->verticalScrollBar()->setValue(scrollvalue + (newlines - lines));
        }

        editor->setUpdatesEnabled(true);
    }
}

void LogViewer::onFolderChanged(const QString &folderPath)
{
    QDir dir(folderPath);
    QStringList filters;
    filters << "*.log";
    dir.setNameFilters(filters);
    QFileInfoList files = dir.entryInfoList(QDir::Files);

    for (const QFileInfo &fileInfo : files)
    {
        const QString filePath = fileInfo.absoluteFilePath();

        if (!m_fileTabs.contains(filePath) && m_fileTabs.count() < 20)  // only load 20 tabs
        {            
            QPlainTextEdit *editor = new QPlainTextEdit(this);
            editor->setReadOnly(true);
            editor->setWordWrapMode(QTextOption::NoWrap);
            editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->maximum());

            m_tabWidget->addTab(editor, fileInfo.fileName());

            m_fileTabs[filePath] = editor;
            QThread* tt =  new QThread();
            m_loadingThreads[filePath] = tt;

            LogLoader* ll = new LogLoader(filePath);
            ll->moveToThread(tt);

            connect(tt, &QThread::started, ll, &LogLoader::loadFile);

            connect(tt, &QThread::finished, ll, &LogLoader::deleteLater);
            connect(tt, &QThread::finished, tt, &QThread::deleteLater);

            connect(ll, &LogLoader::contentLoaded, this, &LogViewer::onUpdateContent);
            connect(ll, &LogLoader::topContentLoaded, this, &LogViewer::onInsertContent);

            tt->start();
            tt->setPriority(QThread::LowPriority);
        }
    }

    m_clearlog->setEnabled(m_fileTabs.count() > 0);
}

void LogViewer::onClearLog()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex != -1)
    {
        QString fileName = m_tabWidget->tabText(currentIndex);
        fileName.remove('&');
        QString filePath = QString("%1/%2").arg(m_folderPath, fileName);

        if (QFile::remove(filePath))
        {
            m_tabWidget->removeTab(currentIndex);
            m_knownLogFiles.remove(filePath);
            m_fileTabs.remove(filePath);
        }
    }
}

void LogViewer::onPollTimer()
{
    QDir dir(m_folderPath);
    bool changed = false;
    for (const QFileInfo& fi : dir.entryInfoList({"*.log"}, QDir::Files)) {
        const QString path = fi.absoluteFilePath();
        if (!m_knownLogFiles.contains(path)) {
            m_knownLogFiles.insert(path);
            changed = true;
        }
    }
    if (changed)
        onFolderChanged(m_folderPath);
}

