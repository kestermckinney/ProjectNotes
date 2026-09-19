// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesMeetings/MeetingBackend.h"
#include "ProjectNotesEmail/backends/ThunderbirdLauncher.h"

#include <QSet>

namespace PN::Comm {

// Thunderbird has no command-line switch for a new-event editor, so the
// meeting is written as an .ics file and passed to Thunderbird, which offers
// to import it into one of its calendars.
class ThunderbirdMeetingBackend final : public MeetingBackend {
public:
    ThunderbirdMeetingBackend(QString cacheDirectory, QString executable,
                              ThunderbirdLauncher::ProcessLauncher launcher = {});
    void handoff(MeetingRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    QString m_cacheDirectory;
    ThunderbirdLauncher m_launcher;
    QSet<QUuid> m_cancelled;
};

} // namespace PN::Comm
