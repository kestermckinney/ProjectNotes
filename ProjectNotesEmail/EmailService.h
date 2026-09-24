#pragma once
#include "EmailBackend.h"
namespace PN::Comm {
class EmailService final {
public:
    using Completion = EmailBackend::Completion;
    void registerBackend(BackendId id, EmailBackend *backend);
    void handoff(EmailRequest request, Completion completion);
    void cancel(const QUuid &operationId);
    QUuid activeOperation() const { return m_activeOperation; }
private:
    QHash<BackendId, EmailBackend *> m_backends;
    QUuid m_activeOperation;
    EmailBackend *m_activeBackend = nullptr;
};
}
