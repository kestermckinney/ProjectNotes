// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailTypes.h"

#include <QDateTime>

namespace PN::Comm {

enum class OperationState { Staged, Cancelled, DraftCreated, HandoffRequested, Unknown };

struct OperationManifest {
    QUuid operationId;
    Workflow workflow = Workflow::SendMeetingNotes;
    QString databaseKey;
    QString projectId;
    BackendId backend = BackendId::Mailto;
    OperationState state = OperationState::Staged;
    QString providerIdentity;
    QString accountKey;
    QDateTime createdAt;
    QList<Artifact> artifacts;
};

struct ArtifactStoreResult {
    Artifact artifact;
    ServiceError error;
    [[nodiscard]] bool ok() const { return error.code.isEmpty(); }
};

class ArtifactStore {
public:
    explicit ArtifactStore(QString profileDirectory);

    // The shared Qt cache location used for transient, app-rendered reports.
    // A developer profile gets an isolated child directory, matching the
    // embedded Python plug-in's cache convention.
    static QString cacheDirectory(const QString &developerProfile = {});
    QString stagingRoot() const;
    bool createOperation(OperationManifest manifest, ServiceError *error = nullptr);
    ArtifactStoreResult stageUserAttachment(const QUuid &operationId, const QString &sourcePath,
                                            const QString &displayName = {});
    ArtifactStoreResult stageGeneratedContent(const QUuid &operationId, const QString &displayName,
                                              const QByteArray &content, const QString &mimeType);
    // Reserve a name in the exact operation-owned artifact directory for an
    // asynchronous producer (such as QtWebEngine's PDF writer).  The file is
    // not registered until registerGeneratedFile verifies and hashes it.
    QString reserveGeneratedPath(const QUuid &operationId, const QString &displayName,
                                 ServiceError *error = nullptr);
    ArtifactStoreResult registerGeneratedFile(const QUuid &operationId, const QString &absolutePath,
                                              const QString &displayName, const QString &mimeType);
    bool publish(const Artifact &artifact, const QString &destinationPath, ServiceError *error = nullptr);
    QList<OperationManifest> loadRecoveryStatus(QList<ServiceError> *diagnostics = nullptr);
    bool cleanup(const QUuid &operationId, ServiceError *error = nullptr);
    // Removes the complete application-owned staging tree at normal shutdown.
    // Published files are outside this tree and are never affected.
    bool cleanupAll(ServiceError *error = nullptr);

private:
    QString operationDirectory(const QUuid &operationId) const;
    bool readManifest(const QUuid &operationId, OperationManifest *manifest, ServiceError *error) const;
    bool writeManifest(const OperationManifest &manifest, ServiceError *error) const;
    bool appendArtifact(const QUuid &operationId, const Artifact &artifact, ServiceError *error);
    ArtifactStoreResult stageFile(const QUuid &operationId, const QString &displayName, const QByteArray *content,
                                  const QString &sourcePath, const QString &mimeType, bool generated);
    static bool validFileName(const QString &name);
    static QByteArray hashFile(const QString &path, ServiceError *error);
    QString m_profileDirectory;
};

} // namespace PN::Comm
