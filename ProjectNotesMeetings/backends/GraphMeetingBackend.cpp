// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "GraphMeetingBackend.h"

#include "ProjectNotesEmail/backends/GraphEmailBackend.h"
#include "ProjectNotesIntegrations/Office365Service.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace PN::Comm {
namespace {
QJsonObject graphTime(const QDateTime &value)
{
    return {{QStringLiteral("dateTime"), value.toUTC().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss"))},
            {QStringLiteral("timeZone"), QStringLiteral("UTC")}};
}

ServiceError graphFailure(const HttpResponse &response)
{
    const QJsonObject error = QJsonDocument::fromJson(response.body).object()
                                  .value(QStringLiteral("error")).toObject();
    QString code = error.value(QStringLiteral("code")).toString();
    const QString message = error.value(QStringLiteral("message")).toString();
    // A token issued before calendar consent was added cannot write events.
    if (response.statusCode == 401 || response.statusCode == 403)
        code = QStringLiteral("graph-calendar-consent-required");
    // A transport failure after the POST left may still have created the event.
    const bool unknown = response.statusCode == 0
        && response.error.code != QLatin1String("office365-not-authenticated")
        && response.error.code != QLatin1String("office365-transport-unavailable");
    const OutcomeCertainty certainty = unknown ? OutcomeCertainty::Uncertain : OutcomeCertainty::Certain;
    return {code.isEmpty() ? (response.error.code.isEmpty() ? QStringLiteral("graph-event-create-failed")
                                                             : response.error.code)
                           : code,
            message.isEmpty() ? response.error.displayText : message,
            unknown ? RetryKind::None : RetryKind::ReviewAndRetry, certainty};
}
}

GraphMeetingBackend::GraphMeetingBackend(Office365Service *service) : m_service(service) {}

void GraphMeetingBackend::cancel(const QUuid &operationId) { m_cancelled.insert(operationId); }

QJsonObject GraphMeetingBackend::eventJson(const MeetingRequest &request)
{
    QJsonArray attendees;
    for (const MeetingAttendee &attendee : request.attendees) {
        attendees.append(QJsonObject{
            {QStringLiteral("emailAddress"), QJsonObject{{QStringLiteral("address"), attendee.address.trimmed()},
                                                         {QStringLiteral("name"), attendee.displayName.trimmed()}}},
            {QStringLiteral("type"), attendee.role == AttendeeRole::Required ? QStringLiteral("required")
                                                                             : QStringLiteral("optional")}});
    }
    const bool html = !request.htmlAgenda.trimmed().isEmpty();
    QJsonObject event{
        {QStringLiteral("subject"), request.subject.trimmed()},
        {QStringLiteral("body"), QJsonObject{{QStringLiteral("contentType"), html ? QStringLiteral("HTML") : QStringLiteral("Text")},
                                             {QStringLiteral("content"), html ? request.htmlAgenda : request.plainAgenda}}},
        {QStringLiteral("start"), graphTime(request.start)},
        {QStringLiteral("end"), graphTime(request.end)},
        {QStringLiteral("attendees"), attendees},
        {QStringLiteral("isDraft"), true},
        {QStringLiteral("allowNewTimeProposals"), true}};
    if (!request.location.trimmed().isEmpty())
        event.insert(QStringLiteral("location"),
                     QJsonObject{{QStringLiteral("displayName"), request.location.trimmed()}});
    return event;
}

void GraphMeetingBackend::handoff(MeetingRequest request, Completion completion)
{
    MeetingHandoffResult result;
    result.operationId = request.operationId;
    const auto fail = [&completion, &result](const QString &code) {
        result.error = {code, {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        completion(result);
    };
    if (!m_service) { fail(QStringLiteral("graph-service-unavailable")); return; }
    if (m_cancelled.remove(request.operationId)) { fail(QStringLiteral("operation-cancelled")); return; }
    if (request.accountGeneration != m_service->account().generation) {
        fail(QStringLiteral("account-generation-changed"));
        return;
    }

    HttpRequest create;
    create.operationId = request.operationId;
    create.method = "POST";
    create.url = QUrl(QStringLiteral("https://graph.microsoft.com/v1.0/me/events"));
    create.headers.insert("Content-Type", "application/json");
    create.body = QJsonDocument(eventJson(request)).toJson(QJsonDocument::Compact);
    m_service->sendGraphRequest(std::move(create), [this, operationId = request.operationId,
                                                    completion = std::move(completion)](HttpResponse response) {
        MeetingHandoffResult result;
        result.operationId = operationId;
        const bool cancelled = m_cancelled.remove(operationId);
        if (!response.error.code.isEmpty() || response.statusCode != 201) {
            result.error = graphFailure(response);
            result.certainty = result.error.certainty;
            completion(result);
            return;
        }
        const QJsonObject event = QJsonDocument::fromJson(response.body).object();
        result.eventIdentity = event.value(QStringLiteral("id")).toString();
        result.certainty = OutcomeCertainty::Certain;
        if (result.eventIdentity.isEmpty()) {
            result.error = {QStringLiteral("graph-event-id-missing"), {}, RetryKind::None, OutcomeCertainty::Uncertain};
            result.certainty = OutcomeCertainty::Uncertain;
            completion(result);
            return;
        }
        // The event exists even if the user cancelled while it was created, so
        // still report it; only skip presenting it.
        const QUrl webLink(event.value(QStringLiteral("webLink")).toString());
        result.providerWebLink = webLink;
        // OWA has no documented link that opens an existing event in its
        // editor, so open the event's own webLink exactly as Graph returned it.
        if (!cancelled && GraphEmailBackend::isAllowedPresentationUrl(webLink))
            result.presentationUrl = webLink;
        completion(result);
    });
}

} // namespace PN::Comm
