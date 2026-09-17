#include "EmailService.h"
namespace PN::Comm {
void EmailService::registerBackend(BackendId id, EmailBackend *backend) { if (backend) m_backends.insert(id, backend); else m_backends.remove(id); }
void EmailService::handoff(EmailRequest request, Completion completion) {
    const auto fail = [&completion](const QUuid &operationId, const char *code) {
        EmailHandoffResult result;
        result.operationId = operationId;
        result.certainty = OutcomeCertainty::Certain;
        result.error = {QString::fromLatin1(code), {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        completion(std::move(result));
    };
    if (request.operationId.isNull()) { fail({}, "operation-id-required"); return; }
    if (!m_activeOperation.isNull()) { fail(request.operationId, "email-operation-busy"); return; }
    auto *backend = m_backends.value(request.backend);
    if (!backend) { fail(request.operationId, "email-backend-unavailable"); return; }
    m_activeOperation = request.operationId;
    m_activeBackend = backend;
    backend->handoff(request, [this, operationId=request.operationId, completion=std::move(completion)](EmailHandoffResult result) mutable {
        if (result.operationId != operationId || m_activeOperation != operationId) return;
        m_activeOperation = {}; m_activeBackend = nullptr; completion(std::move(result));
    });
}
void EmailService::cancel(const QUuid &operationId) { if (m_activeOperation != operationId || !m_activeBackend) return; m_activeBackend->cancel(operationId); }
}
