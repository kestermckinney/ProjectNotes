// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "EmlFileEmailBackend.h"

#include "ProjectNotesEmail/ArtifactStore.h"
#include "ProjectNotesEmail/EmlWriter.h"

#include <QDesktopServices>

namespace PN::Comm {
namespace {

// The message belongs to the operation that already owns this email's staged
// attachments, so it is removed with the rest of the staging tree.  An email
// with nothing staged has no operation yet and gets one here.
QString stageMessage(const QString &cacheDirectory, const EmailRequest &request, const QByteArray &content,
                     ServiceError *error)
{
    ArtifactStore store(cacheDirectory);
    auto staged = store.stageGeneratedContent(request.operationId, QStringLiteral("message.eml"), content,
                                              QStringLiteral("message/rfc822"));
    if (!staged.ok() && staged.error.code == QLatin1String("manifest-not-found")) {
        OperationManifest manifest;
        manifest.operationId = request.operationId;
        manifest.workflow = request.source.workflow;
        manifest.databaseKey = request.source.databaseKey;
        manifest.projectId = request.source.projectId;
        manifest.backend = request.backend;
        if (!store.createOperation(manifest, error)) return {};
        staged = store.stageGeneratedContent(request.operationId, QStringLiteral("message.eml"), content,
                                             QStringLiteral("message/rfc822"));
    }
    if (!staged.ok()) {
        if (error) *error = staged.error;
        return {};
    }
    return staged.artifact.absolutePath;
}

} // namespace

EmlFileEmailBackend::EmlFileEmailBackend(QString cacheDirectory, FileOpener opener)
    : m_cacheDirectory(std::move(cacheDirectory)), m_opener(std::move(opener))
{
    if (!m_opener) m_opener = [](const QUrl &url) { return QDesktopServices::openUrl(url); };
}

void EmlFileEmailBackend::cancel(const QUuid &operationId) { m_cancelled.insert(operationId); }

void EmlFileEmailBackend::handoff(EmailRequest request, Completion completion)
{
    EmailHandoffResult result; result.operationId = request.operationId;
    if (m_cancelled.remove(request.operationId)) {
        result.error = {"operation-cancelled", {}, RetryKind::None, OutcomeCertainty::Certain};
        completion(result); return;
    }
    if (m_launchAttempted.contains(request.operationId)) {
        result.error = {"eml-launch-already-attempted", {}, RetryKind::None, OutcomeCertainty::Uncertain};
        completion(result); return;
    }
    ValidationResult validation;
    const QByteArray message = EmlWriter::write(request, {}, &validation);
    if (message.isEmpty()) {
        result.error = {validation.issues.isEmpty() ? QStringLiteral("eml-write-failed")
                                                    : validation.issues.constFirst().code,
                        {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        completion(result); return;
    }
    ServiceError staging;
    const QString path = stageMessage(m_cacheDirectory, request, message, &staging);
    if (path.isEmpty()) {
        result.error = {staging.code.isEmpty() ? QStringLiteral("eml-staging-failed") : staging.code, {},
                        RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        completion(result); return;
    }
    m_launchAttempted.insert(request.operationId);
    if (!m_opener(QUrl::fromLocalFile(path))) {
        result.error = {"eml-open-failed", {}, RetryKind::RetryPresentation, OutcomeCertainty::Certain};
        completion(result); return;
    }
    // The mail application may or may not keep the draft; nothing reports back.
    result.certainty = OutcomeCertainty::Uncertain;
    result.draftIdentity = path;
    completion(result);
}

} // namespace PN::Comm
