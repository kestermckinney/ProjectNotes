// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesMeetings/MeetingBackend.h"

#include <QSet>

namespace PN::Comm {

// The "MacOS Email and Calendar" meeting backend. It saves the event through
// EventKit and then brings Calendar forward, so it needs only Calendars
// access. It sends no Apple Events: App Review does not grant the sandbox
// exception for them (Guideline 2.4.5(i)), and the direct download behaves
// the same as the App Store build.
//
// It never sets attendees: EventKit sends invitations as soon as a syncable
// event has them. The Schedule Meeting dialog's "Copy
// Attendees" button puts them on the clipboard for the user to paste into the
// event's invitees in Calendar before clicking Send.
class MacCalendarBackend final : public MeetingBackend {
public:
    void handoff(MeetingRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    void saveEvent(MeetingRequest request, Completion completion);

    QSet<QUuid> m_cancelled;
};

} // namespace PN::Comm
