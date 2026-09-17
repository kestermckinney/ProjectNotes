// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailBackend.h"

#include <QSet>

namespace PN::Comm {

// The Objective-C++ Apple Events bridge belongs behind this boundary.  Keeping
// an explicit backend on every host means a saved Mac Mail preference produces
// an actionable platform result rather than falling through to an unregistered
// adapter.  The bridge itself is deliberately deferred until its rich-editable
// composer behavior is proven on signed macOS builds.
class MacMailBackend final : public EmailBackend {
public:
    void handoff(EmailRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    QSet<QUuid> m_cancelled;
};

} // namespace PN::Comm
