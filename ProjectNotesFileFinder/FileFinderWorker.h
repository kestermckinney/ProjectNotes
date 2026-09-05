// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "FileFinderTypes.h"

#include <QHash>
#include <QObject>
#include <QSqlDatabase>

class QNetworkAccessManager;
class QReadWriteLock;
class QTimer;

class FileFinderWorker final : public QObject
{
    Q_OBJECT
public:
    explicit FileFinderWorker(QObject *parent = nullptr);

public slots:
    void initializeDatabase(const QString &databasePath, QReadWriteLock *databaseLock);
    void configure(const FileFinderConfiguration &configuration);
    void start();
    void stop();
    void scanNow();
    void closeDatabase();

signals:
    void diagnostic(const QString &message);
    void scanStarted();
    void scanFinished(const FileFinderScanSummary &summary);
    void locationsCommitted(int inserted, int updated);

private:
    QList<ActiveProject> activeProjects(QString *error);
    QHash<QString, QString> findLocalProjectFolders(const QList<ActiveProject> &projects);
    QList<DiscoveredLocation> scanLocalFolders(const QList<ActiveProject> &projects,
                                                const QHash<QString, QString> &folders,
                                                int *filesExamined, int *matchedFiles);
    bool commitLocations(const QList<DiscoveredLocation> &locations,
                         FileFinderScanSummary *summary, QString *error);
    void scheduleNextScan();
    static QString locationType(const QString &path);
    static QString normalizedPath(const QString &path);

    QString m_connectionName;
    QSqlDatabase m_database;
    QReadWriteLock *m_databaseLock = nullptr;
    FileFinderConfiguration m_configuration;
    QTimer *m_timer = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    bool m_running = false;
    bool m_scanning = false;
    bool m_rescanPending = false;
};
