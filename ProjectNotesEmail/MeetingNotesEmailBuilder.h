// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ReportTypes.h"
#include "SnapshotTypes.h"

namespace PN::Comm {

struct MeetingActionItem {
    QString name;
    QString assignedTo;
    QString status;
    QString dueDate;
};

struct MeetingNotesBuildInput {
    CommunicationSnapshot snapshot;
    QString noteId;
    QList<MeetingActionItem> actionItems;
};

class MeetingNotesEmailBuilder final {
public:
    static std::optional<ReportDocument> build(const MeetingNotesBuildInput &input,
                                               ValidationResult *validation = nullptr);
};

} // namespace PN::Comm
