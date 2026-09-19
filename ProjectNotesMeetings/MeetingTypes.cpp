// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MeetingTypes.h"

namespace PN::Comm {
namespace {
bool hasLineBreak(const QString &value) { return value.contains(u'\r') || value.contains(u'\n'); }
bool invalidAddress(const QString &value)
{
    const QString address = value.trimmed();
    return address.isEmpty() || hasLineBreak(address) || address.contains(u',') || address.contains(u';')
        || address.contains(u'"') || address.contains(u' ') || !address.contains(u'@');
}
}

AttendeeRole attendeeRoleFor(RecipientRole role)
{
    return role == RecipientRole::To ? AttendeeRole::Required : AttendeeRole::Optional;
}

QList<MeetingAttendee> meetingAttendees(const QList<EmailAddress> &recipients)
{
    QList<MeetingAttendee> result;
    QSet<QString> seen;
    for (const EmailAddress &recipient : recipients) {
        const QString address = recipient.address.trimmed();
        const QString key = address.toLower();
        if (address.isEmpty() || seen.contains(key))
            continue;
        seen.insert(key);
        result.append({recipient.displayName.trimmed(), address, attendeeRoleFor(recipient.role)});
    }
    return result;
}

ValidationResult validate(const MeetingRequest &request)
{
    ValidationResult result;
    if (request.operationId.isNull())
        result.addError(QStringLiteral("operation-id-required"), QStringLiteral("operationId"));
    if (request.subject.trimmed().isEmpty())
        result.addError(QStringLiteral("meeting-subject-required"), QStringLiteral("subject"));
    if (hasLineBreak(request.subject))
        result.addError(QStringLiteral("meeting-subject-invalid"), QStringLiteral("subject"));
    if (hasLineBreak(request.location))
        result.addError(QStringLiteral("meeting-location-invalid"), QStringLiteral("location"));
    if (!request.start.isValid() || !request.end.isValid())
        result.addError(QStringLiteral("meeting-time-required"), QStringLiteral("start"));
    else if (request.end <= request.start)
        result.addError(QStringLiteral("meeting-end-before-start"), QStringLiteral("end"));
    if (!request.organizerAddress.isEmpty() && invalidAddress(request.organizerAddress))
        result.addError(QStringLiteral("organizer-address-invalid"), QStringLiteral("organizer"));
    for (const MeetingAttendee &attendee : request.attendees)
        if (invalidAddress(attendee.address)) {
            result.addError(QStringLiteral("recipient-address-invalid"), QStringLiteral("attendees"));
            break;
        }
    return result;
}

} // namespace PN::Comm
