// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailTypes.h"

#include <QDateTime>
#include <QUrl>

namespace PN::Comm {

// Calendar attendance, which is not an alias for To/Cc/Bcc.  Saved audiences
// store email roles, so a meeting reads To as Required and Cc or Bcc as
// Optional (see attendeeRoleFor).
enum class AttendeeRole { Required, Optional };

struct MeetingAttendee {
    QString displayName;
    QString address;
    AttendeeRole role = AttendeeRole::Required;
};

// A reviewed meeting ready for a calendar backend.  It is deliberately not an
// EmailRequest: meetings carry a time range and calendar attendance, and their
// backends never compose mail.
struct MeetingRequest {
    QUuid operationId;
    SourceContext source;
    BackendId backend = BackendId::Mailto;
    QString accountKey;
    quint64 accountGeneration = 0;
    QString subject;
    QString htmlAgenda;
    QString plainAgenda;
    QString location;
    // Any time spec; every backend converts to UTC before writing it out.
    QDateTime start;
    QDateTime end;
    // Empty when the organizer is not known (the calendar app fills it in).
    QString organizerName;
    QString organizerAddress;
    QList<MeetingAttendee> attendees;
};

struct MeetingHandoffResult {
    QUuid operationId;
    OutcomeCertainty certainty = OutcomeCertainty::NotStarted;
    // Provider event id (Graph) or the staged .ics path (file backends).
    QString eventIdentity;
    // Provider presentation metadata only.  Backends never open this URL.
    QUrl presentationUrl;
    // The provider's own link to the event, kept for the debug log.
    QUrl providerWebLink;
    ServiceError error;
};

AttendeeRole attendeeRoleFor(RecipientRole role);
QList<MeetingAttendee> meetingAttendees(const QList<EmailAddress> &recipients);
ValidationResult validate(const MeetingRequest &request);

} // namespace PN::Comm

Q_DECLARE_METATYPE(PN::Comm::MeetingRequest)
Q_DECLARE_METATYPE(PN::Comm::MeetingHandoffResult)
