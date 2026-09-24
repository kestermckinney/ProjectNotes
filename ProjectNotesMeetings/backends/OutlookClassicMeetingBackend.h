// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesMeetings/MeetingBackend.h"

#include <QSet>

namespace PN::Comm {

// Builds the meeting in Outlook (classic) through its object model and shows
// it as an unsent meeting window, attendees included; the user sends it from
// there.  An .ics file cannot do this: Outlook imports one as an appointment
// and drops its attendees.  The object model runs on its own COM apartment
// thread, because starting Outlook can take several seconds.  Windows only.
class OutlookClassicMeetingBackend final : public MeetingBackend {
public:
    void handoff(MeetingRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    QSet<QUuid> m_cancelled;
};

} // namespace PN::Comm
