// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailTypes.h"

#include "SnapshotTypes.h"
#include <QMutex>
#include <QReadWriteLock>
#include <QThreadPool>

#include <functional>

namespace PN::Comm {

struct SnapshotRequest {
    QUuid operationId;
    quint64 previewRevision = 0;
    SourceContext source;
    // Captured by the controller before scheduling the load; repository workers
    // never consult global settings or live QML state.
    QString managingCompanyId;
    QString projectManagerId;
    QDateTime capturedAt;
};

struct SnapshotResult {
    QUuid operationId;
    quint64 previewRevision = 0;
    CommunicationSnapshot snapshot;
    ServiceError error;
};

class SqliteSnapshotLoader final {
public:
    SqliteSnapshotLoader(QString databasePath, QReadWriteLock *lock);
    SnapshotResult load(const SnapshotRequest &request) const;
private: QString m_databasePath; QReadWriteLock *m_lock=nullptr;
};

class CommunicationRepository {
public:
    using Completion = std::function<void(SnapshotResult)>;
    virtual ~CommunicationRepository() = default;
    virtual void loadSnapshot(SnapshotRequest request, Completion completion) = 0;
    virtual void cancel(const QUuid &operationId) = 0;
};

// Owns a private worker pool and opens a fresh read-only SQLite connection for
// each request.  The callback is always delivered on this object's thread.
// A database switch creates a new repository with a new generation; requests
// captured against an older generation fail before querying the new database.
class AsyncSqliteCommunicationRepository final : public QObject, public CommunicationRepository {
public:
    AsyncSqliteCommunicationRepository(QString databasePath, QReadWriteLock *lock,
                                       quint64 databaseGeneration, QObject *parent = nullptr);
    ~AsyncSqliteCommunicationRepository() override;

    void loadSnapshot(SnapshotRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    bool isCancelled(const QUuid &operationId) const;

    QString m_databasePath;
    QReadWriteLock *m_lock = nullptr;
    quint64 m_databaseGeneration = 0;
    mutable QMutex m_mutex;
    QSet<QUuid> m_cancelled;
    QThreadPool m_workers;
};

} // namespace PN::Comm
