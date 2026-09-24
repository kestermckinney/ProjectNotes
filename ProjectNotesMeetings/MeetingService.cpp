// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MeetingService.h"

namespace PN::Comm {

void MeetingService::registerBackend(BackendId id, MeetingBackend *backend)
{
    if (backend) m_backends.insert(id, backend);
    else m_backends.remove(id);
}

void MeetingService::handoff(MeetingRequest request, Completion completion)
{
    const auto fail = [&completion](const QUuid &operationId, const QString &code) {
        MeetingHandoffResult result;
        result.operationId = operationId;
        result.certainty = OutcomeCertainty::Certain;
        result.error = {code, {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        completion(std::move(result));
    };
    if (request.operationId.isNull()) { fail({}, QStringLiteral("operation-id-required")); return; }
    if (!m_activeOperation.isNull()) { fail(request.operationId, QStringLiteral("meeting-operation-busy")); return; }
    const ValidationResult validation = validate(request);
    if (!validation.ok()) { fail(request.operationId, validation.issues.constFirst().code); return; }
    auto *backend = m_backends.value(request.backend);
    if (!backend) { fail(request.operationId, QStringLiteral("meeting-backend-unavailable")); return; }
    m_activeOperation = request.operationId;
    m_activeBackend = backend;
    backend->handoff(std::move(request), [this, operationId = m_activeOperation,
                                          completion = std::move(completion)](MeetingHandoffResult result) mutable {
        if (result.operationId != operationId || m_activeOperation != operationId) return;
        m_activeOperation = {};
        m_activeBackend = nullptr;
        completion(std::move(result));
    });
}

void MeetingService::cancel(const QUuid &operationId)
{
    if (m_activeOperation != operationId || !m_activeBackend) return;
    m_activeBackend->cancel(operationId);
}

} // namespace PN::Comm
