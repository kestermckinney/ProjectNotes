// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ThunderbirdMeetingBackend.h"
#include "StagedInvite.h"

#include <QDir>

namespace PN::Comm {

ThunderbirdMeetingBackend::ThunderbirdMeetingBackend(QString cacheDirectory, QString executable,
                                                     ThunderbirdLauncher::ProcessLauncher launcher)
    : m_cacheDirectory(std::move(cacheDirectory)), m_launcher(std::move(executable), std::move(launcher))
{
}

void ThunderbirdMeetingBackend::cancel(const QUuid &operationId) { m_cancelled.insert(operationId); }

void ThunderbirdMeetingBackend::handoff(MeetingRequest request, Completion completion)
{
    MeetingHandoffResult result;
    result.operationId = request.operationId;
    const auto fail = [&completion, &result](const QString &code, RetryKind retry) {
        result.error = {code, {}, retry, OutcomeCertainty::Certain};
        completion(result);
    };
    if (m_cancelled.remove(request.operationId)) { fail(QStringLiteral("operation-cancelled"), RetryKind::None); return; }
    if (!m_launcher.available()) { fail(QStringLiteral("thunderbird-unavailable"), RetryKind::ReviewAndRetry); return; }
    const StagedInvite invite = stageMeetingInvite(m_cacheDirectory, request);
    if (!invite.ok()) { result.error = invite.error; completion(result); return; }
    if (!isOperationOwnedFile(invite.path, invite.operationDirectory)) {
        fail(QStringLiteral("thunderbird-path-not-operation-owned"), RetryKind::ReviewAndRetry);
        return;
    }
    result.eventIdentity = invite.path;
    QDir stagingDirectory(invite.operationDirectory);
    const QString readable = stagingDirectory.cdUp() ? stagingDirectory.absolutePath() : QString();
    if (!m_launcher.launch({QDir::toNativeSeparators(invite.path)}, readable)) {
        fail(QStringLiteral("thunderbird-launch-failed"), RetryKind::RetryPresentation);
        return;
    }
    result.certainty = OutcomeCertainty::Uncertain;
    completion(result);
}

} // namespace PN::Comm
