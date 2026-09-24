// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailBackend.h"

namespace PN::Comm::Test {

class RecordingEmailBackend final : public EmailBackend {
public:
    void handoff(EmailRequest request, Completion completion) override
    {
        requests.append(request);
        EmailHandoffResult result;
        result.operationId = request.operationId;
        result.certainty = OutcomeCertainty::Certain;
        completion(std::move(result));
    }

    void cancel(const QUuid &operationId) override { cancelled.append(operationId); }

    QList<EmailRequest> requests;
    QList<QUuid> cancelled;
};

} // namespace PN::Comm::Test
