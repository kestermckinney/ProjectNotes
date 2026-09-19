// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesMeetings/MeetingBackend.h"

#include <QJsonObject>
#include <QSet>

namespace PN::Comm {
class Office365Service;

// Creates the meeting in the signed-in Microsoft 365 calendar with isDraft set,
// the same request the legacy Python scheduler made, and returns the event's
// Outlook link so the user can review it and send the invitations.
//
// Microsoft documents that creating an event with attendees can send the
// invitations; isDraft is not a documented guarantee against that.  This
// backend follows the plug-in's behavior by the maintainer's decision, and it
// must be verified against a test mailbox (see the email implementation plan).
class GraphMeetingBackend final : public MeetingBackend {
public:
    explicit GraphMeetingBackend(Office365Service *service);
    void handoff(MeetingRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

    static QJsonObject eventJson(const MeetingRequest &request);

private:
    Office365Service *m_service = nullptr;
    QSet<QUuid> m_cancelled;
};

} // namespace PN::Comm
