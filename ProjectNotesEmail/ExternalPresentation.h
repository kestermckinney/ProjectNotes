// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailTypes.h"

#include <QUrl>

#include <functional>

namespace PN::Comm {

struct PresentationRequest {
    QUuid operationId;
    Artifact artifact;
    QUrl destination;
};

struct PresentationResult {
    QUuid operationId;
    ServiceError error;
};

class ExternalPresentation {
public:
    using Completion = std::function<void(PresentationResult)>;
    virtual ~ExternalPresentation() = default;
    virtual void present(PresentationRequest request, Completion completion) = 0;
};

} // namespace PN::Comm
