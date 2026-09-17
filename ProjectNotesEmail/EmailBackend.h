// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailTypes.h"

#include <functional>
#include <QUrl>

namespace PN::Comm {

struct EmailHandoffResult {
    QUuid operationId;
    OutcomeCertainty certainty = OutcomeCertainty::NotStarted;
    QString draftIdentity;
    // Provider presentation metadata only.  Backends never open this URL.
    QUrl presentationUrl;
    ServiceError error;
};

class EmailBackend {
public:
    using Completion = std::function<void(EmailHandoffResult)>;
    virtual ~EmailBackend() = default;
    virtual void handoff(EmailRequest request, Completion completion) = 0;
    virtual void cancel(const QUuid &operationId) = 0;
};

} // namespace PN::Comm
