// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "MeetingTypes.h"
#include "ProjectNotesEmail/SnapshotTypes.h"
#include "ProjectNotesIntegrations/MeetingTemplateStore.h"

#include <optional>

namespace PN::Comm {

// The reviewable state of one meeting.  The review dialog edits these fields;
// attendees stay in RecipientSelectionModel until the handoff freezes them.
struct MeetingPreparation {
    QUuid operationId;
    SourceContext source;
    BackendId backend = BackendId::DefaultApp;
    QString accountKey;
    quint64 accountGeneration = 0;
    QString templateId;
    QString templateName;
    QString subject;
    QString htmlAgenda;
    QString plainAgenda;
    QString location;
    QDateTime start;
    int durationMinutes = 60;
    QString organizerName;
    QString organizerAddress;
};

class MeetingPreparationFactory final {
public:
    // Renders the meeting type's subject and agenda against the project
    // snapshot and proposes the next whole local hour as the start time.
    static std::optional<MeetingPreparation> create(const CommunicationSnapshot &snapshot, SourceContext source,
                                                    const MeetingTemplate &meetingType, BackendId backend,
                                                    const QDateTime &now, ValidationResult *validation = nullptr);

    // The next whole hour after `now`, in `now`'s own time spec.  An exact
    // hour moves to the following one so the proposal is never in the past.
    static QDateTime nextWholeHour(const QDateTime &now);

    static MeetingRequest request(const MeetingPreparation &preparation, const QList<EmailAddress> &recipients);
};

} // namespace PN::Comm
