// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "IcsFileMeetingBackend.h"
#include "StagedInvite.h"

#include <QDesktopServices>

namespace PN::Comm {

IcsFileMeetingBackend::IcsFileMeetingBackend(QString cacheDirectory, FileOpener opener, QString openFailureCode)
    : m_cacheDirectory(std::move(cacheDirectory)), m_opener(std::move(opener)),
      m_openFailureCode(std::move(openFailureCode))
{
    if (!m_opener) m_opener = [](const QUrl &url) { return QDesktopServices::openUrl(url); };
}

void IcsFileMeetingBackend::cancel(const QUuid &operationId) { m_cancelled.insert(operationId); }

void IcsFileMeetingBackend::handoff(MeetingRequest request, Completion completion)
{
    MeetingHandoffResult result;
    result.operationId = request.operationId;
    if (m_cancelled.remove(request.operationId)) {
        result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, OutcomeCertainty::Certain};
        completion(result);
        return;
    }
    const StagedInvite invite = stageMeetingInvite(m_cacheDirectory, request);
    if (!invite.ok()) { result.error = invite.error; completion(result); return; }
    result.eventIdentity = invite.path;
    if (!m_opener(QUrl::fromLocalFile(invite.path))) {
        result.error = {m_openFailureCode, {}, RetryKind::RetryPresentation,
                        OutcomeCertainty::Certain};
        completion(result);
        return;
    }
    // The calendar app may or may not import the file; nothing reports back.
    result.certainty = OutcomeCertainty::Uncertain;
    completion(result);
}

} // namespace PN::Comm
