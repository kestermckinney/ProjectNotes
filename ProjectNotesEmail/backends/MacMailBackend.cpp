// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MacMailBackend.h"

namespace PN::Comm {

void MacMailBackend::handoff(EmailRequest request, Completion completion)
{
    EmailHandoffResult result;
    result.operationId = request.operationId;
    if (m_cancelled.remove(request.operationId)) {
        result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, OutcomeCertainty::Certain};
    } else {
#if defined(Q_OS_DARWIN)
        result.error = {QStringLiteral("mac-mail-bridge-unavailable"),
                        QStringLiteral("The native macOS Mail bridge is not available in this build."),
                        RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
#else
        result.error = {QStringLiteral("mac-mail-unavailable"),
                        QStringLiteral("macOS Mail is available only on macOS."),
                        RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
#endif
    }
    completion(std::move(result));
}

void MacMailBackend::cancel(const QUuid &operationId)
{
    m_cancelled.insert(operationId);
}

} // namespace PN::Comm
