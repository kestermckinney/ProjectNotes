// Copyright (C) 2025, 2026 Paul McKinney
#ifndef LOGVIEWER_H
#define LOGVIEWER_H

#include <QDialog>
#include <QObject>
#include <QFile>
#include <QPlainTextEdit>
#include <QDir>
#include <QSet>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QThread>
#include <QTimer>
#include <QFileSystemWatcher>

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
    qint64 m_topPosition = -1;
    qint64 m_lastPosition = 0;
    QTimer* m_topLoadTimer = nullptr;
    QFileSystemWatcher* m_fileWatcher = nullptr;
    QString m_filePath;

    bool m_isLoading = false;
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
    void closed();
    void getContents(const QString& filePath);
    void fillTopContents(const QString& filePath);

private:
    QString m_folderPath;
    QTabWidget* m_tabWidget;
    QPushButton* m_clearlog;
    QPushButton* m_close;
    QMap<QString, QPlainTextEdit*> m_fileTabs;
    QMap<QString, QThread*> m_loadingThreads;
    QFileSystemWatcher* m_folderWatcher = nullptr;
    QSet<QString> m_knownLogFiles;

public:
    LogViewer(QWidget* parent = nullptr);
    ~LogViewer();

    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *e) override;
    void hideEvent(QHideEvent *event) override;

    static QString getLogFileLocation();
};

#endif // LOGVIEWER_H
