// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "MeetingBackend.h"

#include <QHash>

namespace PN::Comm {

// Dispatches a reviewed meeting to the registered backend for its BackendId,
// one operation at a time, mirroring EmailService.
class MeetingService final {
public:
    using Completion = MeetingBackend::Completion;
    void registerBackend(BackendId id, MeetingBackend *backend);
    void handoff(MeetingRequest request, Completion completion);
    void cancel(const QUuid &operationId);
    [[nodiscard]] QUuid activeOperation() const { return m_activeOperation; }

private:
    QHash<BackendId, MeetingBackend *> m_backends;
    QUuid m_activeOperation;
    MeetingBackend *m_activeBackend = nullptr;
};

} // namespace PN::Comm
