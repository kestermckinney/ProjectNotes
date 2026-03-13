#ifndef LOGVIEWER_H
#define LOGVIEWER_H

#include <QDialog>
#include <QObject>
#include <QFile>
#include <QPlainTextEdit>
#include <QDir>
#include <QSet>
#include <QDateTime>
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

private slots:
    void onPollTimer();

private:
    qint64 m_topPosition = -1;
    qint64 m_lastPosition = 0;
    QTimer* m_topLoadTimer = nullptr;
    QTimer* m_pollTimer = nullptr;
    qint64  m_pollLastSize = -1;
    QDateTime m_pollLastModified;
    QString m_filePath;
};

class LogViewer : public QDialog
{
    Q_OBJECT

private slots:
    void onFolderChanged(const QString &folderPath);
    void onClearLog();
    void onInsertContent(const QString& filePath, const QString& content);
    void onUpdateContent(const QString& filePath, const QString& content);
    void onPollTimer();

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
    QTimer* m_pollTimer;
    QSet<QString> m_knownLogFiles;

public:
    LogViewer(QWidget* parent = nullptr);
    ~LogViewer();

    void closeEvent(QCloseEvent *e) override;

    static QString getLogFileLocation();
};

#endif // LOGVIEWER_H
