// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "StagedInvite.h"

#include "ProjectNotesEmail/ArtifactStore.h"
#include "ProjectNotesMeetings/IcsWriter.h"

#include <QDir>

namespace PN::Comm {

StagedInvite stageMeetingInvite(const QString &cacheDirectory, const MeetingRequest &request,
                                const QDateTime &stamp)
{
    StagedInvite result;
    ValidationResult validation;
    const QByteArray content = IcsWriter::write(request, stamp, &validation);
    if (content.isEmpty()) {
        result.error = {validation.issues.isEmpty() ? QStringLiteral("meeting-invalid")
                                                    : validation.issues.constFirst().code,
                        {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        return result;
    }
    ArtifactStore store(cacheDirectory);
    OperationManifest manifest;
    manifest.operationId = request.operationId;
    manifest.workflow = Workflow::ScheduleMeeting;
    manifest.databaseKey = request.source.databaseKey;
    manifest.projectId = request.source.projectId;
    manifest.backend = request.backend;
    ServiceError error;
    if (!store.createOperation(manifest, &error)) {
        result.error = {error.code.isEmpty() ? QStringLiteral("meeting-staging-failed") : error.code,
                        {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        return result;
    }
    const auto staged = store.stageGeneratedContent(request.operationId, QStringLiteral("invite.ics"),
                                                    content, QStringLiteral("text/calendar"));
    if (!staged.ok()) {
        result.error = {staged.error.code.isEmpty() ? QStringLiteral("meeting-staging-failed") : staged.error.code,
                        {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        return result;
    }
    result.path = staged.artifact.absolutePath;
    result.operationDirectory = QDir(store.stagingRoot()).filePath(request.operationId.toString(QUuid::WithoutBraces));
    return result;
}

} // namespace PN::Comm
