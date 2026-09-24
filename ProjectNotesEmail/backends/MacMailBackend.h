// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailBackend.h"

#include <QSet>

namespace PN::Comm {

// Hands a message to Mail.app as a real, editable HTML compose window via
// NSSharingService, rather than writing an .eml file for the desktop to open.
// NSSharingService's compose-email item has no To/Cc distinction and no
// attachment slot: Cc addresses are folded into the single recipients list it
// exposes, Bcc addresses are dropped (EmailContentBuilder warns about this
// before handoff so it is never a surprise), and attachments are not carried
// over. Callers who need attachments or a true Bcc should pick Default
// Application instead.
class MacMailBackend final : public EmailBackend {
public:
    void handoff(EmailRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    QSet<QUuid> m_cancelled;
};

} // namespace PN::Comm
