// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ProjectNotesEmail/EmailBackend.h"
#include <QSet>
namespace PN::Comm {
// A real new-message Inspector initializes the account's configured signature.
class OutlookClassicEmailBackend final : public EmailBackend {
public:
    void handoff(EmailRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;
private:
    QSet<QUuid> m_cancelled;
};
}
