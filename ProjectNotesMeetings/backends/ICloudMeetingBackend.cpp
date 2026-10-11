// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ICloudMeetingBackend.h"

#include "ProjectNotesIntegrations/QtNetworkHttpTransport.h"
#include "ProjectNotesMeetings/IcsWriter.h"

#include <QDomDocument>

namespace PN::Comm {
namespace {

bool cloudUrl(const QUrl &url)
{
    return url.scheme() == QLatin1String("https") && url.userInfo().isEmpty()
        && (url.host() == QLatin1String("icloud.com") || url.host().endsWith(QLatin1String(".icloud.com")));
}

QDomElement child(const QDomNode &node, const QString &name)
{
    for (QDomNode current = node.firstChild(); !current.isNull(); current = current.nextSibling()) {
        const QDomElement element = current.toElement();
        if (element.localName() == name || element.tagName().section(':', -1) == name)
            return element;
    }
    return {};
}

QDomElement descendant(const QDomNode &node, const QString &name)
{
    const QDomElement direct = child(node, name);
    if (!direct.isNull()) return direct;
    for (QDomNode current = node.firstChild(); !current.isNull(); current = current.nextSibling()) {
        const QDomElement found = descendant(current, name);
        if (!found.isNull()) return found;
    }
    return {};
}

QByteArray propfind(const QByteArray &properties)
{
    return "<d:propfind xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:caldav\"><d:prop>"
        + properties + "</d:prop></d:propfind>";
}

QString responseError(const HttpResponse &response)
{
    if (!response.error.displayText.isEmpty()) return response.error.displayText;
    return QStringLiteral("iCloud Calendar returned HTTP %1.").arg(response.statusCode);
}

} // namespace

struct ICloudMeetingBackend::Operation {
    MeetingRequest request;
    ICloudCredentials credentials;
    Completion completion;
    QSet<QUuid> requests;
    bool putStarted = false;
};

ICloudMeetingBackend::ICloudMeetingBackend(ICloudCredentialsProvider credentials, QObject *parent,
                                           HttpTransport *transport)
    : QObject(parent), m_credentials(std::move(credentials)), m_transport(transport)
{
    if (!m_transport) {
        auto *owned = new QtNetworkHttpTransport(this);
        m_transport = owned;
    }
}

ICloudMeetingBackend::~ICloudMeetingBackend()
{
    const auto ids = m_operations.keys();
    for (const QUuid &id : ids)
        cancel(id);
}

void ICloudMeetingBackend::handoff(MeetingRequest request, Completion completion)
{
    MeetingHandoffResult result;
    result.operationId = request.operationId;
    const ICloudCredentials credentials = m_credentials ? m_credentials() : ICloudCredentials{};
    if (!credentials.valid()) {
        result.error = {QStringLiteral("icloud-credentials-required"),
                        QStringLiteral("Enter your Apple Account and save an app-specific password in Settings."),
                        RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        completion(result);
        return;
    }
    if (m_operations.contains(request.operationId)) {
        result.error = {QStringLiteral("icloud-calendar-operation-busy"), {}, RetryKind::ReviewAndRetry,
                        OutcomeCertainty::Certain};
        completion(result);
        return;
    }
    auto *operation = new Operation{std::move(request), credentials, std::move(completion)};
    m_operations.insert(operation->request.operationId, operation);
    discover(operation, 0, QUrl(QStringLiteral("https://caldav.icloud.com/")));
}

void ICloudMeetingBackend::discover(Operation *operation, int step, QUrl url)
{
    const QByteArray body = propfind(step == 0 ? QByteArray("<d:current-user-principal/>")
                                               : step == 1 ? QByteArray("<c:calendar-home-set/>")
                                                           : QByteArray("<d:resourcetype/><d:displayname/><d:current-user-privilege-set/>"));
    dav(operation, "PROPFIND", url, body, step == 2 ? 1 : 0,
        [this, operation, step, url](HttpResponse response) {
        if (!m_operations.contains(operation->request.operationId)) return;
        if (response.statusCode != 200 && response.statusCode != 207) {
            const QString code = response.statusCode == 401 || response.statusCode == 403
                ? QStringLiteral("icloud-calendar-authentication-failed")
                : QStringLiteral("icloud-calendar-discovery-failed");
            fail(operation, code, responseError(response));
            return;
        }
        QDomDocument document;
        if (!document.setContent(response.body, QDomDocument::ParseOption::UseNamespaceProcessing)) {
            fail(operation, QStringLiteral("icloud-calendar-response-invalid"));
            return;
        }
        if (step < 2) {
            const QString propertyName = step == 0 ? QStringLiteral("current-user-principal")
                                                   : QStringLiteral("calendar-home-set");
            const QString href = descendant(descendant(document, propertyName), QStringLiteral("href")).text();
            if (href.isEmpty()) {
                fail(operation, QStringLiteral("icloud-calendar-location-missing"));
                return;
            }
            discover(operation, step + 1, url.resolved(QUrl(href)));
            return;
        }

        QUrl calendar;
        QString calendarName;
        const QDomElement root = document.documentElement();
        for (QDomNode node = root.firstChild(); !node.isNull(); node = node.nextSibling()) {
            if (descendant(node, QStringLiteral("calendar")).isNull()) continue;
            const QDomElement privileges = descendant(node, QStringLiteral("current-user-privilege-set"));
            if (!privileges.isNull() && descendant(privileges, QStringLiteral("write")).isNull()
                && descendant(privileges, QStringLiteral("write-content")).isNull()
                && descendant(privileges, QStringLiteral("all")).isNull())
                continue;
            const QString href = child(node, QStringLiteral("href")).text();
            if (href.isEmpty()) continue;
            calendar = url.resolved(QUrl(href));
            calendarName = descendant(node, QStringLiteral("displayname")).text().trimmed();
            break;
        }
        if (!calendar.isValid()) {
            fail(operation, QStringLiteral("icloud-calendar-not-found"),
                 QStringLiteral("No writable iCloud calendar was found."));
            return;
        }
        createEvent(operation, calendar, calendarName);
    });
}

void ICloudMeetingBackend::createEvent(Operation *operation, QUrl calendarUrl, QString calendarName)
{
    MeetingRequest event = operation->request;
    // iCloud Calendar can send invitations when attendees are written. Keep
    // the server-side item unsent and let the user paste the reviewed list.
    // An organizer without attendees would make Calendar treat this as a
    // meeting organized by that address rather than an event created by the
    // calendar owner, hiding Add Invitees when the addresses differ.
    event.attendees.clear();
    event.organizerName.clear();
    event.organizerAddress.clear();
    ValidationResult validation;
    const QByteArray content = IcsWriter::write(event, QDateTime::currentDateTimeUtc(), &validation,
                                                IcsWriter::OutputKind::CalDavResource);
    if (content.isEmpty()) {
        fail(operation, validation.issues.isEmpty() ? QStringLiteral("icloud-calendar-event-invalid")
                                                    : validation.issues.constFirst().code);
        return;
    }
    if (!calendarUrl.path().endsWith('/')) calendarUrl.setPath(calendarUrl.path() + '/');
    const QUrl eventUrl = calendarUrl.resolved(QUrl(operation->request.operationId.toString(QUuid::WithoutBraces)
                                                    + QStringLiteral(".ics")));
    operation->putStarted = true;
    dav(operation, "PUT", eventUrl, content, 0,
        [this, operation, calendarName](HttpResponse response) {
        if (!m_operations.contains(operation->request.operationId)) return;
        if (response.statusCode != 200 && response.statusCode != 201 && response.statusCode != 204) {
            const bool uncertain = response.statusCode == 0;
            fail(operation, QStringLiteral("icloud-calendar-event-create-failed"), responseError(response), uncertain);
            return;
        }
        MeetingHandoffResult result;
        result.operationId = operation->request.operationId;
        result.certainty = OutcomeCertainty::Certain;
        result.eventIdentity = calendarName.isEmpty() ? QStringLiteral("iCloud Calendar") : calendarName;
        // CalDAV supplies a resource identity but no browser URL for editing it.
        finish(operation, std::move(result));
    });
}

void ICloudMeetingBackend::dav(Operation *operation, QByteArray method, QUrl url, QByteArray body, int depth,
                               DavCompletion completion, int redirects)
{
    if (!cloudUrl(url)) {
        HttpResponse response;
        response.error = {QStringLiteral("icloud-unsafe-url"), QStringLiteral("iCloud returned an unsafe URL.")};
        completion(response);
        return;
    }
    HttpRequest request;
    request.operationId = QUuid::createUuid();
    request.method = method;
    request.url = url;
    request.body = body;
    request.manualRedirects = true;
    request.headers.insert("Authorization", "Basic "
        + (operation->credentials.account + ':' + operation->credentials.password).toUtf8().toBase64());
    request.headers.insert("Depth", QByteArray::number(depth));
    if (method == "PUT") {
        request.headers.insert("Content-Type", "text/calendar; charset=utf-8");
        request.headers.insert("If-None-Match", "*");
    } else {
        request.headers.insert("Content-Type", "application/xml; charset=utf-8");
    }
    operation->requests.insert(request.operationId);
    const QUuid requestId = request.operationId;
    m_transport->send(std::move(request),
        [this, operation, method, url, body, depth, completion = std::move(completion), redirects,
         requestId](HttpResponse response) mutable {
        if (!m_operations.contains(operation->request.operationId)) return;
        operation->requests.remove(requestId);
        if (QList<int>{301, 302, 307, 308}.contains(response.statusCode)) {
            const QByteArray location = response.headers.value("Location", response.headers.value("location"));
            const QUrl target = url.resolved(QUrl(QString::fromUtf8(location)));
            if (redirects < 5 && cloudUrl(target)) {
                dav(operation, method, target, body, depth, std::move(completion), redirects + 1);
                return;
            }
            response.error = {QStringLiteral("icloud-redirect-invalid"),
                              QStringLiteral("iCloud returned an unsafe or excessive redirect.")};
        }
        completion(std::move(response));
    });
}

void ICloudMeetingBackend::cancel(const QUuid &operationId)
{
    Operation *operation = m_operations.value(operationId);
    if (!operation) return;
    for (const QUuid &requestId : std::as_const(operation->requests))
        m_transport->cancel(requestId);
    MeetingHandoffResult result;
    result.operationId = operationId;
    result.certainty = operation->putStarted ? OutcomeCertainty::Uncertain : OutcomeCertainty::Certain;
    result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, result.certainty};
    finish(operation, std::move(result));
}

void ICloudMeetingBackend::fail(Operation *operation, QString code, QString displayText, bool uncertain)
{
    MeetingHandoffResult result;
    result.operationId = operation->request.operationId;
    result.certainty = uncertain ? OutcomeCertainty::Uncertain : OutcomeCertainty::Certain;
    result.error = {std::move(code), std::move(displayText),
                    uncertain ? RetryKind::None : RetryKind::ReviewAndRetry, result.certainty};
    finish(operation, std::move(result));
}

void ICloudMeetingBackend::finish(Operation *operation, MeetingHandoffResult result)
{
    if (!operation || m_operations.take(operation->request.operationId) != operation) return;
    for (const QUuid &requestId : std::as_const(operation->requests))
        m_transport->cancel(requestId);
    auto completion = std::move(operation->completion);
    delete operation;
    completion(std::move(result));
}

} // namespace PN::Comm
