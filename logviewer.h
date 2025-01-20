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

class LogViewer : public QDialog
{
    Q_OBJECT

private slots:
    void onFileChanged(const QString &filePath);
    void onFolderChanged(const QString &folderPath);
    void onClearLog();

private:
    QString m_folderPath;
    QTabWidget* m_tabWidget;
    QPushButton* m_clearlog;
    QPushButton* m_close;
    QFileSystemWatcher *m_fileWatcher;
    QMap<QString, QPlainTextEdit*> m_fileTabs;

public:
    LogViewer(QWidget* t_parent = nullptr);
    ~LogViewer();

    static QString getLogFileLocation();
};

#endif // LOGVIEWER_H
