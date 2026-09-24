#pragma once
#include "ProjectNotesEmail/EmailBackend.h"
namespace PN::Comm::Test {
class DeferredEmailBackend final : public EmailBackend {
public:
    void handoff(EmailRequest request, Completion completion) override { requestSeen=request; pending=std::move(completion); }
    void cancel(const QUuid &id) override { cancelled.append(id); }
    void finish(EmailHandoffResult result) { if (pending) { auto completion=std::move(pending); completion(std::move(result)); } }
    EmailRequest requestSeen; Completion pending; QList<QUuid> cancelled;
};
}
