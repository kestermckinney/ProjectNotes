// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesMeetings/MeetingBackend.h"

#include <QSet>

namespace PN::Comm {

// Creates the meeting directly in Calendar.app via Apple Events, instead of
// staging an .ics file for the desktop to open. Deliberately never sets the
// new event's attendees: EventKit and Calendar's own scripting both send
// invitations the moment a syncable event gets attendees, and this backend's
// whole point is a reviewed, not-yet-sent event. request.attendees is instead
// ignored here: the Schedule Meeting dialog's "Copy Attendees" button puts
// them on the clipboard for the reviewer to paste into Calendar's invitees
// field before they click Send.
//
// The first handoff triggers macOS's one-time Automation consent prompt for
// Calendar (and, if Accessibility access has already been granted, for
// System Events). Accessibility access is never requested by this class; if
// it has not been granted the event is still created and shown, just not
// auto-opened for editing.
class MacCalendarBackend final : public MeetingBackend {
public:
    void handoff(MeetingRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    void runScript(MeetingRequest request, Completion completion);

    QSet<QUuid> m_cancelled;
};

} // namespace PN::Comm
