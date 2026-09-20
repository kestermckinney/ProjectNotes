// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MeetingPreparationFactory.h"

#include "ProjectNotesEmail/CommunicationTemplateContext.h"
#include "ProjectNotesIntegrations/TemplateParser.h"

#include <QTextDocument>

namespace PN::Comm {
namespace {
QString plainTextFromHtml(const QString &html)
{
    QTextDocument document;
    document.setHtml(html);
    return document.toPlainText().trimmed();
}

bool isBlankRichText(const QString &html)
{
    return html.trimmed().isEmpty() || plainTextFromHtml(html).isEmpty();
}
}

QDateTime MeetingPreparationFactory::nextWholeHour(const QDateTime &now)
{
    // Adding seconds to a QDateTime handles the day, month and year rollover
    // and any daylight-saving transition in the local zone.
    const QTime time = now.time();
    const int intoHour = time.minute() * 60 + time.second();
    QDateTime result = now.addSecs(3600 - intoHour);
    result.setTime(QTime(result.time().hour(), 0));
    return result;
}

std::optional<MeetingPreparation> MeetingPreparationFactory::create(const CommunicationSnapshot &snapshot,
                                                                    SourceContext source,
                                                                    const MeetingTemplate &meetingType,
                                                                    BackendId backend, const QDateTime &now,
                                                                    ValidationResult *validation)
{
    source.workflow = Workflow::ScheduleMeeting;
    const TemplateContext context = communicationTemplateContext(snapshot, Workflow::ScheduleMeeting);

    MeetingPreparation preparation;
    preparation.operationId = QUuid::createUuid();
    preparation.source = std::move(source);
    preparation.backend = backend;
    preparation.templateId = meetingType.id;
    preparation.templateName = meetingType.name;
    preparation.durationMinutes = meetingType.durationMinutes > 0 ? meetingType.durationMinutes : 60;
    preparation.start = nextWholeHour(now.toLocalTime());

    ValidationResult issues;
    if (meetingType.subject.trimmed().isEmpty()) {
        preparation.subject = snapshot.projectNumber + QLatin1Char(' ') + snapshot.projectName
                              + QStringLiteral(" - ") + meetingType.name;
    } else {
        const TemplateRender subject = renderTemplateFields(meetingType.subject, context);
        issues.issues += subject.validation.issues;
        preparation.subject = subject.text.simplified();
    }
    if (!isBlankRichText(meetingType.agenda)) {
        const TemplateRender agenda = renderTemplateFields(meetingType.agenda, context, TemplateRenderMode::Html);
        issues.issues += agenda.validation.issues;
        preparation.htmlAgenda = agenda.text;
    }
    // A calendar file's DESCRIPTION is plain text by format, so it is derived
    // from the agenda rather than written separately.
    if (!preparation.htmlAgenda.isEmpty())
        preparation.plainAgenda = plainTextFromHtml(preparation.htmlAgenda);

    for (const SnapshotPerson &person : snapshot.people)
        if (!snapshot.projectManagerId.isEmpty() && person.id == snapshot.projectManagerId) {
            preparation.organizerName = person.name;
            preparation.organizerAddress = person.email.trimmed();
        }

    if (!issues.ok()) {
        if (validation) validation->issues += issues.issues;
        return std::nullopt;
    }
    return preparation;
}

MeetingRequest MeetingPreparationFactory::request(const MeetingPreparation &preparation,
                                                  const QList<EmailAddress> &recipients)
{
    MeetingRequest request;
    request.operationId = preparation.operationId;
    request.source = preparation.source;
    request.backend = preparation.backend;
    request.accountKey = preparation.accountKey;
    request.accountGeneration = preparation.accountGeneration;
    request.subject = preparation.subject.trimmed();
    request.htmlAgenda = preparation.htmlAgenda;
    request.plainAgenda = preparation.plainAgenda;
    request.location = preparation.location.trimmed();
    request.start = preparation.start;
    request.end = preparation.start.addSecs(qint64(preparation.durationMinutes) * 60);
    request.organizerName = preparation.organizerName;
    request.organizerAddress = preparation.organizerAddress;
    request.attendees = meetingAttendees(recipients);
    // The organizer is never also listed as an attendee.
    const QString organizer = request.organizerAddress.trimmed().toLower();
    if (!organizer.isEmpty())
        request.attendees.removeIf([&organizer](const MeetingAttendee &attendee) {
            return attendee.address.trimmed().toLower() == organizer;
        });
    return request;
}

} // namespace PN::Comm
