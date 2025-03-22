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

public:
    LogLoader(const QString& filePath);
    ~LogLoader();

private:
    qint64 m_top_position = -1;
    qint64 m_last_position = 0;
    QTimer* m_top_load_timer = nullptr;
    QString m_file_path;
    QFileSystemWatcher *m_fileWatcher;
};

class LogViewer : public QDialog
{
    Q_OBJECT

private slots:
    void onFolderChanged(const QString &folderPath);
    void onClearLog();
    void onInsertContent(const QString& filePath, const QString& content);
    void onUpdateContent(const QString& filePath, const QString& content);

signals:
    void getContents(const QString& filePath);
    void fillTopContents(const QString& filePath);

private:
    QString m_folderPath;
    QTabWidget* m_tabWidget;
    QPushButton* m_clearlog;
    QPushButton* m_close;
    QMap<QString, QPlainTextEdit*> m_fileTabs;
    QMap<QString, QThread*> m_loading_threads;
    QFileSystemWatcher *m_fileWatcher;
    // QThread* m_loader_thread = nullptr;
    // QThread* m_filler_thread = nullptr;

public:
    LogViewer(QWidget* t_parent = nullptr);
    ~LogViewer();

    void closeEvent(QCloseEvent *e) override;

    static QString getLogFileLocation();
};

#endif // LOGVIEWER_H
