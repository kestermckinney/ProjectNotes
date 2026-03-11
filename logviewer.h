#ifndef LOGVIEWER_H
#define LOGVIEWER_H

#include <QDialog>
#include <QObject>
#include <QFile>
#include <QPlainTextEdit>
#include <QDir>
#include <QFileSystemWatcher>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QThread>
#include <QTimer>

class LogLoader : public QObject
{
    Q_OBJECT

signals:
    void topContentLoaded(const QString& filePath, const QString& content);
    void contentLoaded(const QString& filePath, const QString& content);

public slots:
    void loadFile();
    void timerUpdate();
    void onFileChanged(const QString &filePath);
    void resetFileWatcher(QWidget* oldWidget, QWidget* newWidget);

public:
    LogLoader(const QString& filePath);
    ~LogLoader();

private:
    void startFileWatcher();

    qint64 m_topPosition = -1;
    qint64 m_lastPosition = 0;
    QTimer* m_topLoadTimer = nullptr;
    QString m_filePath;
    QFileSystemWatcher *m_fileWatcher = nullptr;
};

class LogViewer : public QDialog
{
    Q_OBJECT

private slots:
    void onFolderChanged(const QString &folderPath);
    void onClearLog();
    void onInsertContent(const QString& filePath, const QString& content);
    void onUpdateContent(const QString& filePath, const QString& content);
    void resetFileWatcher(QWidget* oldWidget, QWidget* newWidget);

signals:
    void getContents(const QString& filePath);
    void fillTopContents(const QString& filePath);

private:
    QString m_folderPath;
    QTabWidget* m_tabWidget;
    QPushButton* m_clearlog;
    QPushButton* m_close;
    QMap<QString, QPlainTextEdit*> m_fileTabs;
    QMap<QString, QThread*> m_loadingThreads;
    QFileSystemWatcher *m_fileWatcher;
    // QThread* m_loaderThread = nullptr;
    // QThread* m_fillerThread = nullptr;

public:
    LogViewer(QWidget* parent = nullptr);
    ~LogViewer();

    void closeEvent(QCloseEvent *e) override;

    static QString getLogFileLocation();
};

#endif // LOGVIEWER_H
