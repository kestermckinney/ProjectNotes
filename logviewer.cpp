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

// this member needs to be called after the object has been moved to a different thread
void LogLoader::startFileWatcher()
{
    // Set up file watcher
    m_fileWatcher = new QFileSystemWatcher(this);
    m_fileWatcher->addPath(m_filePath);

    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &LogLoader::onFileChanged);
}

// the file watcher is not reliable and needs reset sometimes
void LogLoader::resetFileWatcher(QWidget* oldWidget, QWidget* newWidget)
{
    if (m_fileWatcher)
    {
        m_fileWatcher->removePath(m_filePath);
        m_fileWatcher->addPath(m_filePath);
    }
}

LogLoader::~LogLoader()
{
    disconnect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &LogLoader::onFileChanged);

    if (m_topLoadTimer)
    {
        disconnect(m_topLoadTimer, SIGNAL(timeout()), this, SLOT(timerUpdate()));

        m_topLoadTimer->stop();
        delete m_topLoadTimer;
        m_topLoadTimer = nullptr;
    }

    if (m_fileWatcher)
        delete m_fileWatcher;
}

void LogLoader::onFileChanged(const QString &filePath)
{
    loadFile();
}

void LogLoader::loadFile()
{
    // this needs to be called here because we don't want the watcher
    // to be created in a different thread
    if (m_fileWatcher == nullptr)
        startFileWatcher();

    // just load the bottom at first
    QFile file(m_filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
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
            file.seek(m_lastPosition);
        }

        QTextStream in(&file);
        QString content;

        // Read in chunks to keep UI responsive
        while (!in.atEnd() && !QThread::currentThread()->isInterruptionRequested())
        {
            content = in.read(8192); // 8KB chunks
            emit contentLoaded(m_filePath, content);
            QThread::msleep(50);
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
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // we need to start with the end of the file
        qint64 bottomofchunck = m_topPosition;
        qint64 topofchunck = qMax(bottomofchunck - 8192, 0);
        qint64 readsize = bottomofchunck - topofchunck;

        QTextStream in(&file);
        QString content;

        // Read in chunks to keep UI responsive
        if (readsize > 0 && !QThread::currentThread()->isInterruptionRequested())
        {
            file.seek(topofchunck);
            content = in.read(readsize); // 8KB chunks
            emit topContentLoaded(m_filePath, content);
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

    global_Settings.getWindowState("LogViewer", this);

    m_fileWatcher = new QFileSystemWatcher(this);
    m_fileWatcher->addPath(m_folderPath);

    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &LogViewer::onFolderChanged);
    connect((QApplication*)QApplication::instance(), &QApplication::focusChanged, this, &LogViewer::resetFileWatcher); // since the file watcher sometimes quits reset it when the window activation changes
    connect(m_clearlog, &QPushButton::clicked, this, &LogViewer::onClearLog);
    connect(m_close, &QPushButton::clicked, this, &QDialog::close);

    onFolderChanged(m_folderPath);
}

void LogViewer::closeEvent(QCloseEvent *e)
{
    for (QThread* t : m_loadingThreads)
    {
        t->requestInterruption();
    }

    QDialog::closeEvent(e);
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

    global_Settings.setWindowState("LogViewer", this);

    disconnect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &LogViewer::onFolderChanged);
    disconnect((QApplication*)QApplication::instance(), &QApplication::focusChanged, this, &LogViewer::resetFileWatcher); // since the file watcher sometimes quits reset it when the window activation changes
    disconnect(m_clearlog, &QPushButton::clicked, this, &LogViewer::onClearLog);
    disconnect(m_close, &QPushButton::clicked, this, &QDialog::close);

    delete m_fileWatcher;
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

            connect((QApplication*)QApplication::instance(), &QApplication::focusChanged, ll, &LogLoader::resetFileWatcher); // since the file watcher sometimes quits reset it when the window activation changes

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
            m_fileWatcher->removePath(filePath);
            // unless i restart the watcher it doesn't catch the new file
            m_fileWatcher->removePath(m_folderPath);
            m_fileWatcher->addPath(m_folderPath);
            m_fileTabs.remove(filePath);
        }
    }
}

void LogViewer::resetFileWatcher(QWidget* oldWidget, QWidget* newWidget)
{
    QStringList files = m_fileWatcher->files();
    QStringList dirs  = m_fileWatcher->directories();

    m_fileWatcher->removePaths(files);
    m_fileWatcher->removePaths(dirs);
    m_fileWatcher->addPaths(files);
    m_fileWatcher->addPaths(dirs);
}

