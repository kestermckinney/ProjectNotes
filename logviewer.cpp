#include "logviewer.h"
#include "pnsettings.h"
#include <QStandardPaths>
#include <QPushButton>

LogViewer::LogViewer(QWidget* t_parent) : QDialog(t_parent)
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

    // Set up file watcher
    m_fileWatcher = new QFileSystemWatcher(this);
    m_fileWatcher->addPath(m_folderPath);

    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &LogViewer::onFileChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &LogViewer::onFolderChanged);

    connect(m_clearlog, &QPushButton::clicked, this, &LogViewer::onClearLog);
    connect(m_close, &QPushButton::clicked, this, &QDialog::close);

    onFolderChanged(m_folderPath);
}

LogViewer::~LogViewer()
{
    global_Settings.setWindowState("LogViewer", this);

    disconnect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &LogViewer::onFileChanged);
    disconnect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &LogViewer::onFolderChanged);

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

void LogViewer::onFileChanged(const QString &filePath)
{
    QFile file(filePath);

    // qDebug() << "checking file: " << filePath;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    if (m_fileTabs.contains(filePath))  //TODO: this probably has the & symble
    {
        // qDebug() << "reloading file: " << filePath;

        QPlainTextEdit *editor = m_fileTabs[filePath];
        editor->setPlainText(content);
        editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->maximum());
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
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                continue;
            }

            QTextStream in(&file);
            QString content = in.readAll();
            file.close();

            QPlainTextEdit *editor = new QPlainTextEdit(this);
            editor->setPlainText(content);
            editor->setReadOnly(true);
            editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->maximum());

            m_tabWidget->addTab(editor, fileInfo.fileName());

            //qDebug() << "adding file as: " << filePath;
            m_fileTabs[filePath] = editor;
            m_fileWatcher->addPath(filePath);
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
        QString filePath = m_folderPath + "/" + fileName;

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
