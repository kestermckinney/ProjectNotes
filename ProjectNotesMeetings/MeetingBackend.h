// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "MeetingTypes.h"

#include <functional>

namespace PN::Comm {

// The calendar counterpart of EmailBackend.  A backend presents one reviewed
// meeting to the user's calendar; it never opens its presentation URL itself.
class MeetingBackend {
public:
    using Completion = std::function<void(MeetingHandoffResult)>;
    virtual ~MeetingBackend() = default;
    virtual void handoff(MeetingRequest request, Completion completion) = 0;
    virtual void cancel(const QUuid &operationId) = 0;
};

} // namespace PN::Comm
