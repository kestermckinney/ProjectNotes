// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Native meeting scheduling: the iCalendar writer, preparation, the file and
// Thunderbird and iCloud backends, the Graph event body and the cache cleaner.  Fakes
// only: no calendar app, mail client, network or real account is used.

#include "ProjectNotesEmail/TransientCacheCleaner.h"
#include "ProjectNotesEmail/backends/ThunderbirdLauncher.h"
#include "ProjectNotesMeetings/IcsWriter.h"
#include "ProjectNotesMeetings/MeetingPreparationFactory.h"
#include "ProjectNotesMeetings/MeetingService.h"
#include "ProjectNotesMeetings/backends/GraphMeetingBackend.h"
#include "ProjectNotesMeetings/backends/IcsFileMeetingBackend.h"
#include "ProjectNotesMeetings/backends/ICloudMeetingBackend.h"
#include "ProjectNotesMeetings/backends/ThunderbirdMeetingBackend.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QtTest/QtTest>

using namespace PN::Comm;

class MeetingsTest final : public QObject {
    Q_OBJECT
private slots:
    void escapesIcsText();
    void foldsLongLinesOnUtf8Boundaries();
    void writesEventWithRolesAndOrganizer();
    void omitsOrganizerWhenUnknown();
    void rejectsInvalidMeetings();
    void writesUtcAcrossDaylightSavingChange();
    void proposesNextWholeHour();
    void preparesFromMeetingType();
    void rejectsUnresolvedTemplateFields();
    void buildsRequestFromRecipients();
    void icsBackendStagesAndOpensFile();
    void icsBackendReportsOpenFailure();
    void thunderbirdBackendPassesInviteFile();
    void thunderbirdLauncherGrantsFlatpakAccess();
    void graphEventBodyIsDraftWithCalendarRoles();
    void iCloudCreatesEventWithoutAttendeesOrPresentationUrl();
    void serviceValidatesAndDispatches();
    void cacheCleanerKeepsPluginState();
};

namespace {

MeetingRequest sampleRequest()
{
    MeetingRequest request;
    request.operationId = QUuid::createUuid();
    request.source = {QStringLiteral("db"), 1, QStringLiteral("project"), {}, Workflow::ScheduleMeeting};
    request.subject = QStringLiteral("P100 Kickoff");
    request.plainAgenda = QStringLiteral("Scope; budget, schedule");
    request.htmlAgenda = QStringLiteral("<p>Scope</p>");
    request.location = QStringLiteral("Room 4");
    request.start = QDateTime(QDate(2026, 3, 15), QTime(14, 0), QTimeZone::UTC);
    request.end = request.start.addSecs(3600);
    request.organizerName = QStringLiteral("Pat Manager");
    request.organizerAddress = QStringLiteral("pm@example.com");
    request.attendees = {{QStringLiteral("Ann Required"), QStringLiteral("ann@example.com"), AttendeeRole::Required},
                         {QStringLiteral("Bob Optional"), QStringLiteral("bob@example.com"), AttendeeRole::Optional}};
    return request;
}

QString unfold(const QByteArray &content)
{
    return QString::fromUtf8(content).replace(QStringLiteral("\r\n "), QString());
}

QString makeExecutable(const QTemporaryDir &directory, const QString &name)
{
    const QString path = directory.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return {};
    file.write("#!/bin/sh\n");
    file.close();
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    return path;
}

class ICloudTransport final : public HttpTransport {
public:
    QList<HttpRequest> requests;
    void send(HttpRequest request, Completion completion) override
    {
        requests.append(request);
        HttpResponse response;
        response.operationId = request.operationId;
        if (request.method == "PUT") {
            response.statusCode = 201;
        } else {
            response.statusCode = 207;
            if (requests.size() == 1)
                response.body = "<d:multistatus xmlns:d=\"DAV:\"><d:response><d:propstat><d:prop><d:current-user-principal><d:href>/123/principal/</d:href></d:current-user-principal></d:prop></d:propstat></d:response></d:multistatus>";
            else if (requests.size() == 2)
                response.body = "<d:multistatus xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:caldav\"><d:response><d:propstat><d:prop><c:calendar-home-set><d:href>/123/calendars/</d:href></c:calendar-home-set></d:prop></d:propstat></d:response></d:multistatus>";
            else
                response.body = "<d:multistatus xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:caldav\"><d:response><d:href>/123/calendars/home/</d:href><d:propstat><d:prop><d:displayname>Home</d:displayname><d:resourcetype><d:collection/><c:calendar/></d:resourcetype></d:prop></d:propstat></d:response></d:multistatus>";
        }
        completion(response);
    }
    void cancel(const QUuid &) override {}
};

} // namespace

void MeetingsTest::escapesIcsText()
{
    QCOMPARE(IcsWriter::escapeText(QStringLiteral("a;b,c\\d\ne\r\nf")),
             QStringLiteral("a\\;b\\,c\\\\d\\ne\\nf"));
}

void MeetingsTest::foldsLongLinesOnUtf8Boundaries()
{
    const QString line = QStringLiteral("DESCRIPTION:") + QString(100, QChar(0x00e9));
    const QByteArray folded = IcsWriter::foldLine(line);
    QVERIFY(folded.endsWith("\r\n"));
    const QList<QByteArray> parts = folded.chopped(2).split('\n');
    QVERIFY(parts.size() > 1);
    for (qsizetype index = 0; index < parts.size(); ++index) {
        QByteArray part = parts.at(index);
        if (part.endsWith('\r')) part.chop(1);
        QVERIFY2(part.size() <= 75, part.constData());
        if (index > 0) {
            QVERIFY(part.startsWith(' '));
            part.remove(0, 1);
        }
        // Every physical line is complete UTF-8 on its own.
        QVERIFY(!QString::fromUtf8(part).contains(QChar::ReplacementCharacter));
    }
    QCOMPARE(unfold(folded).trimmed(), line);
}

void MeetingsTest::writesEventWithRolesAndOrganizer()
{
    const MeetingRequest request = sampleRequest();
    ValidationResult validation;
    const QByteArray content = IcsWriter::write(request, QDateTime(QDate(2026, 3, 1), QTime(8, 0), QTimeZone::UTC),
                                                &validation);
    QVERIFY(validation.ok());
    QVERIFY(content.startsWith("BEGIN:VCALENDAR\r\n"));
    QVERIFY(content.endsWith("END:VCALENDAR\r\n"));
    const QString text = unfold(content);
    QVERIFY(text.contains(QStringLiteral("METHOD:PUBLISH\r\n")));
    QVERIFY(text.contains(QStringLiteral("UID:%1@projectnotes").arg(request.operationId.toString(QUuid::WithoutBraces))));
    QVERIFY(text.contains(QStringLiteral("DTSTAMP:20260301T080000Z\r\n")));
    QVERIFY(text.contains(QStringLiteral("DTSTART:20260315T140000Z\r\n")));
    QVERIFY(text.contains(QStringLiteral("DTEND:20260315T150000Z\r\n")));
    QVERIFY(text.contains(QStringLiteral("SUMMARY:P100 Kickoff\r\n")));
    QVERIFY(text.contains(QStringLiteral("DESCRIPTION:Scope\\; budget\\, schedule\r\n")));
    QVERIFY(text.contains(QStringLiteral("LOCATION:Room 4\r\n")));
    QVERIFY(text.contains(QStringLiteral("X-ALT-DESC;FMTTYPE=text/html:<p>Scope</p>\r\n")));
    QVERIFY(text.contains(QStringLiteral("ORGANIZER;CN=\"Pat Manager\":mailto:pm@example.com\r\n")));
    QVERIFY(text.contains(QStringLiteral("ATTENDEE;CN=\"Ann Required\";ROLE=REQ-PARTICIPANT;PARTSTAT=NEEDS-ACTION;RSVP=TRUE:mailto:ann@example.com\r\n")));
    QVERIFY(text.contains(QStringLiteral("ATTENDEE;CN=\"Bob Optional\";ROLE=OPT-PARTICIPANT;PARTSTAT=NEEDS-ACTION;RSVP=TRUE:mailto:bob@example.com\r\n")));
    // No bare LF line endings.
    QCOMPARE(content.count("\n"), content.count("\r\n"));
}

void MeetingsTest::omitsOrganizerWhenUnknown()
{
    MeetingRequest request = sampleRequest();
    request.organizerName.clear();
    request.organizerAddress.clear();
    request.attendees.first().displayName = QStringLiteral("Quote \"Name\"");
    const QString text = unfold(IcsWriter::write(request, {}));
    QVERIFY(!text.contains(QStringLiteral("ORGANIZER")));
    // A DQUOTE cannot appear inside a quoted parameter value.
    QVERIFY(text.contains(QStringLiteral("ATTENDEE;CN=\"Quote Name\";")));
}

void MeetingsTest::rejectsInvalidMeetings()
{
    MeetingRequest request = sampleRequest();
    request.subject = QStringLiteral("Line\nbreak");
    ValidationResult validation;
    QVERIFY(IcsWriter::write(request, {}, &validation).isEmpty());
    QVERIFY(!validation.ok());

    request = sampleRequest();
    request.end = request.start;
    QVERIFY(!validate(request).ok());

    request = sampleRequest();
    request.attendees.first().address = QStringLiteral("ann@example.com\r\nBCC:x@example.com");
    QVERIFY(!validate(request).ok());
}

void MeetingsTest::writesUtcAcrossDaylightSavingChange()
{
    // 8 March 2026 01:30 in New York is the last half hour of standard time.
    const QTimeZone newYork("America/New_York");
    if (!newYork.isValid())
        QSKIP("The America/New_York zone is not available on this system.");
    MeetingRequest request = sampleRequest();
    request.start = QDateTime(QDate(2026, 3, 8), QTime(1, 30), newYork);
    request.end = request.start.addSecs(3600);
    const QString text = unfold(IcsWriter::write(request, {}));
    QVERIFY(text.contains(QStringLiteral("DTSTART:20260308T063000Z\r\n")));
    QVERIFY(text.contains(QStringLiteral("DTEND:20260308T073000Z\r\n")));
}

void MeetingsTest::proposesNextWholeHour()
{
    const QTimeZone utc(QTimeZone::UTC);
    QCOMPARE(MeetingPreparationFactory::nextWholeHour(QDateTime(QDate(2026, 5, 4), QTime(10, 15, 30), utc)),
             QDateTime(QDate(2026, 5, 4), QTime(11, 0), utc));
    QCOMPARE(MeetingPreparationFactory::nextWholeHour(QDateTime(QDate(2026, 5, 4), QTime(10, 0), utc)),
             QDateTime(QDate(2026, 5, 4), QTime(11, 0), utc));
    QCOMPARE(MeetingPreparationFactory::nextWholeHour(QDateTime(QDate(2026, 12, 31), QTime(23, 30), utc)),
             QDateTime(QDate(2027, 1, 1), QTime(0, 0), utc));
}

void MeetingsTest::preparesFromMeetingType()
{
    CommunicationSnapshot snapshot;
    snapshot.projectId = QStringLiteral("project");
    snapshot.projectNumber = QStringLiteral("P100");
    snapshot.projectName = QStringLiteral("Plant & Tower");
    snapshot.projectManagerId = QStringLiteral("pm");
    snapshot.loadedAt = QDateTime(QDate(2026, 5, 4), QTime(9, 0));
    snapshot.contacts = {{QStringLiteral("pm"), QStringLiteral("Pat Manager"), QStringLiteral("pm@example.com")}};

    MeetingTemplate meetingType;
    meetingType.id = QStringLiteral("kickoff");
    meetingType.name = QStringLiteral("Kickoff");
    meetingType.durationMinutes = 90;
    meetingType.subject = QStringLiteral("{{ project.number }} kickoff");
    meetingType.agenda = QStringLiteral("<p>Welcome to {{ project.name }}</p>");

    const QDateTime now(QDate(2026, 5, 4), QTime(10, 20));
    ValidationResult validation;
    const auto prepared = MeetingPreparationFactory::create(snapshot, {QStringLiteral("db"), 2, QStringLiteral("project")},
                                                            meetingType, BackendId::DefaultApp, now, &validation);
    QVERIFY2(prepared, validation.issues.isEmpty() ? "" : qPrintable(validation.issues.constFirst().code));
    QCOMPARE(prepared->source.workflow, Workflow::ScheduleMeeting);
    QCOMPARE(prepared->subject, QStringLiteral("P100 kickoff"));
    // Field values are HTML-escaped inside the rich agenda.
    QVERIFY(prepared->htmlAgenda.contains(QStringLiteral("Plant &amp; Tower")));
    QCOMPARE(prepared->plainAgenda, QStringLiteral("Welcome to Plant & Tower"));
    QCOMPARE(prepared->durationMinutes, 90);
    QCOMPARE(prepared->start.time(), QTime(11, 0));
    QCOMPARE(prepared->organizerAddress, QStringLiteral("pm@example.com"));

    // A meeting type without a subject falls back to the project and type name.
    meetingType.subject.clear();
    const auto fallback = MeetingPreparationFactory::create(snapshot, {}, meetingType, BackendId::DefaultApp, now);
    QVERIFY(fallback);
    QCOMPARE(fallback->subject, QStringLiteral("P100 Plant & Tower - Kickoff"));
}

void MeetingsTest::rejectsUnresolvedTemplateFields()
{
    CommunicationSnapshot snapshot;
    MeetingTemplate meetingType;
    meetingType.id = QStringLiteral("status");
    meetingType.name = QStringLiteral("Status");
    // report.date belongs to the report workflows, not to meetings.
    meetingType.subject = QStringLiteral("Status {{ report.date }}");
    ValidationResult validation;
    QVERIFY(!MeetingPreparationFactory::create(snapshot, {}, meetingType, BackendId::DefaultApp,
                                               QDateTime::currentDateTime(), &validation));
    QVERIFY(!validation.ok());
}

void MeetingsTest::buildsRequestFromRecipients()
{
    MeetingPreparation preparation;
    preparation.operationId = QUuid::createUuid();
    preparation.subject = QStringLiteral(" Review ");
    preparation.start = QDateTime(QDate(2026, 6, 1), QTime(9, 0));
    preparation.durationMinutes = 45;
    preparation.organizerAddress = QStringLiteral("pm@example.com");
    const QList<EmailAddress> recipients = {
        {QStringLiteral("Ann"), QStringLiteral("ann@example.com"), RecipientRole::To},
        {QStringLiteral("Bob"), QStringLiteral("bob@example.com"), RecipientRole::Cc},
        {QStringLiteral("Cy"), QStringLiteral("cy@example.com"), RecipientRole::Bcc},
        {QStringLiteral("Pat"), QStringLiteral("PM@example.com"), RecipientRole::To},
        {QStringLiteral("Ann again"), QStringLiteral("ANN@example.com"), RecipientRole::Cc}};
    const MeetingRequest request = MeetingPreparationFactory::request(preparation, recipients);
    QCOMPARE(request.subject, QStringLiteral("Review"));
    QCOMPARE(request.end, preparation.start.addSecs(45 * 60));
    QCOMPARE(request.attendees.size(), 3);
    QCOMPARE(request.attendees.at(0).role, AttendeeRole::Required);
    QCOMPARE(request.attendees.at(1).role, AttendeeRole::Optional);
    QCOMPARE(request.attendees.at(2).role, AttendeeRole::Optional);
}

void MeetingsTest::icsBackendStagesAndOpensFile()
{
    QTemporaryDir cache;
    QVERIFY(cache.isValid());
    QUrl opened;
    IcsFileMeetingBackend backend(cache.path(), [&opened](const QUrl &url) { opened = url; return true; });
    const MeetingRequest request = sampleRequest();
    MeetingHandoffResult result;
    backend.handoff(request, [&result](MeetingHandoffResult value) { result = value; });
    QVERIFY2(result.error.code.isEmpty(), qPrintable(result.error.code));
    QCOMPARE(result.certainty, OutcomeCertainty::Uncertain);
    QVERIFY(opened.isLocalFile());
    const QString path = opened.toLocalFile();
    QCOMPARE(QFileInfo(path).fileName(), QStringLiteral("invite.ics"));
    QVERIFY(path.startsWith(QDir(cache.path()).filePath(QStringLiteral("communications/staging"))));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.readAll().startsWith("BEGIN:VCALENDAR"));
}

void MeetingsTest::icsBackendReportsOpenFailure()
{
    QTemporaryDir cache;
    IcsFileMeetingBackend backend(cache.path(), [](const QUrl &) { return false; });
    MeetingHandoffResult result;
    backend.handoff(sampleRequest(), [&result](MeetingHandoffResult value) { result = value; });
    QCOMPARE(result.error.code, QStringLiteral("calendar-open-failed"));
    QCOMPARE(result.error.retryKind, RetryKind::RetryPresentation);
}

void MeetingsTest::thunderbirdBackendPassesInviteFile()
{
    QTemporaryDir cache;
    QTemporaryDir tools;
    const QString thunderbird = makeExecutable(tools, QStringLiteral("thunderbird"));
    QString program;
    QStringList arguments;
    ThunderbirdMeetingBackend backend(cache.path(), thunderbird,
                                      [&program, &arguments](const QString &launched, const QStringList &args) {
                                          program = launched;
                                          arguments = args;
                                          return true;
                                      });
    MeetingHandoffResult result;
    backend.handoff(sampleRequest(), [&result](MeetingHandoffResult value) { result = value; });
    QVERIFY2(result.error.code.isEmpty(), qPrintable(result.error.code));
    QCOMPARE(program, thunderbird);
    QCOMPARE(arguments.size(), 1);
    QCOMPARE(QFileInfo(arguments.constFirst()).fileName(), QStringLiteral("invite.ics"));
    QVERIFY(QFileInfo::exists(arguments.constFirst()));

    ThunderbirdMeetingBackend missing(cache.path(), tools.filePath(QStringLiteral("absent")));
    missing.handoff(sampleRequest(), [&result](MeetingHandoffResult value) { result = value; });
    QCOMPARE(result.error.code, QStringLiteral("thunderbird-unavailable"));
}

void MeetingsTest::thunderbirdLauncherGrantsFlatpakAccess()
{
    QTemporaryDir tools;
    const QString flatpak = makeExecutable(tools, QStringLiteral("flatpak"));
    QStringList arguments;
    ThunderbirdLauncher launcher(QStringLiteral("\"%1\" run org.mozilla.Thunderbird").arg(flatpak),
                                 [&arguments](const QString &, const QStringList &args) {
                                     arguments = args;
                                     return true;
                                 });
    QVERIFY(launcher.available());
    QVERIFY(launcher.launch({QStringLiteral("/cache/invite.ics")}, QStringLiteral("/cache/staging")));
    QCOMPARE(arguments, (QStringList{QStringLiteral("run"), QStringLiteral("--filesystem=/cache/staging:ro"),
                                     QStringLiteral("org.mozilla.Thunderbird"), QStringLiteral("/cache/invite.ics")}));
}

void MeetingsTest::graphEventBodyIsDraftWithCalendarRoles()
{
    MeetingRequest request = sampleRequest();
    request.start = QDateTime(QDate(2026, 7, 1), QTime(9, 0), QTimeZone::fromSecondsAheadOfUtc(-4 * 3600));
    request.end = request.start.addSecs(1800);
    const QJsonObject event = GraphMeetingBackend::eventJson(request);
    QCOMPARE(event.value(QStringLiteral("isDraft")).toBool(), true);
    QCOMPARE(event.value(QStringLiteral("subject")).toString(), QStringLiteral("P100 Kickoff"));
    QCOMPARE(event.value(QStringLiteral("start")).toObject().value(QStringLiteral("dateTime")).toString(),
             QStringLiteral("2026-07-01T13:00:00"));
    QCOMPARE(event.value(QStringLiteral("start")).toObject().value(QStringLiteral("timeZone")).toString(),
             QStringLiteral("UTC"));
    QCOMPARE(event.value(QStringLiteral("end")).toObject().value(QStringLiteral("dateTime")).toString(),
             QStringLiteral("2026-07-01T13:30:00"));
    QCOMPARE(event.value(QStringLiteral("body")).toObject().value(QStringLiteral("contentType")).toString(),
             QStringLiteral("HTML"));
    QCOMPARE(event.value(QStringLiteral("location")).toObject().value(QStringLiteral("displayName")).toString(),
             QStringLiteral("Room 4"));
    const QJsonArray attendees = event.value(QStringLiteral("attendees")).toArray();
    QCOMPARE(attendees.size(), 2);
    QCOMPARE(attendees.at(0).toObject().value(QStringLiteral("type")).toString(), QStringLiteral("required"));
    QCOMPARE(attendees.at(1).toObject().value(QStringLiteral("type")).toString(), QStringLiteral("optional"));
    QCOMPARE(attendees.at(1).toObject().value(QStringLiteral("emailAddress")).toObject()
                 .value(QStringLiteral("address")).toString(), QStringLiteral("bob@example.com"));
}

void MeetingsTest::iCloudCreatesEventWithoutAttendeesOrPresentationUrl()
{
    ICloudTransport transport;
    ICloudMeetingBackend backend([] {
        return ICloudCredentials{QStringLiteral("person@icloud.com"), QStringLiteral("app-password")};
    }, nullptr, &transport);
    MeetingHandoffResult result;
    backend.handoff(sampleRequest(), [&result](MeetingHandoffResult value) { result = std::move(value); });
    QVERIFY2(result.error.code.isEmpty(), qPrintable(result.error.code));
    QCOMPARE(result.certainty, OutcomeCertainty::Certain);
    QCOMPARE(result.eventIdentity, QStringLiteral("Home"));
    QVERIFY(result.presentationUrl.isEmpty());
    QVERIFY(result.providerWebLink.isEmpty());
    QCOMPARE(transport.requests.size(), 4);
    const HttpRequest put = transport.requests.constLast();
    QCOMPARE(put.method, QByteArray("PUT"));
    QVERIFY(put.url.host().endsWith(QStringLiteral("icloud.com")));
    QVERIFY(put.body.contains("BEGIN:VEVENT"));
    QVERIFY(!put.body.contains("METHOD:"));
    QVERIFY(!put.body.contains("ORGANIZER"));
    QVERIFY(!put.body.contains("ATTENDEE"));
    QCOMPARE(put.headers.value("If-None-Match"), QByteArray("*"));
}

void MeetingsTest::serviceValidatesAndDispatches()
{
    QTemporaryDir cache;
    int opened = 0;
    IcsFileMeetingBackend backend(cache.path(), [&opened](const QUrl &) { ++opened; return true; });
    MeetingService service;
    MeetingHandoffResult result;

    MeetingRequest request = sampleRequest();
    service.handoff(request, [&result](MeetingHandoffResult value) { result = value; });
    QCOMPARE(result.error.code, QStringLiteral("meeting-backend-unavailable"));

    service.registerBackend(BackendId::DefaultApp, &backend);
    request.subject.clear();
    service.handoff(request, [&result](MeetingHandoffResult value) { result = value; });
    QCOMPARE(result.error.code, QStringLiteral("meeting-subject-required"));
    QCOMPARE(opened, 0);

    service.handoff(sampleRequest(), [&result](MeetingHandoffResult value) { result = value; });
    QVERIFY(result.error.code.isEmpty());
    QCOMPARE(opened, 1);
    QVERIFY(service.activeOperation().isNull());
}

void MeetingsTest::cacheCleanerKeepsPluginState()
{
    QTemporaryDir cache;
    QVERIFY(cache.isValid());
    const QDir root(cache.path());
    const auto touch = [&root](const QString &relative) {
        QVERIFY(root.mkpath(QFileInfo(root.filePath(relative)).path()));
        QFile file(root.filePath(relative));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("x");
    };
    touch(QStringLiteral("communications/staging/op/artifacts/invite.ics"));
    touch(QStringLiteral("20260915P100statusreport.html"));
    touch(QStringLiteral("icloud_contacts_abc.json"));
    touch(QStringLiteral("token_cache.json"));
    touch(QStringLiteral("ifs_sso_token_cache.json"));
    touch(QStringLiteral("projectnotes_saved_state.json"));
    touch(QStringLiteral("qmlcache/keep.html"));

    QVERIFY(TransientCacheCleaner(cache.path()).clean().isEmpty());
    QVERIFY(!root.exists(QStringLiteral("communications/staging")));
    QVERIFY(!root.exists(QStringLiteral("20260915P100statusreport.html")));
    QVERIFY(root.exists(QStringLiteral("icloud_contacts_abc.json")));
    QVERIFY(root.exists(QStringLiteral("token_cache.json")));
    QVERIFY(root.exists(QStringLiteral("ifs_sso_token_cache.json")));
    QVERIFY(root.exists(QStringLiteral("projectnotes_saved_state.json")));
    // Only the top level of the cache is swept for report files.
    QVERIFY(root.exists(QStringLiteral("qmlcache/keep.html")));
    // Cleaning is idempotent.
    QVERIFY(TransientCacheCleaner(cache.path()).clean().isEmpty());
}

QTEST_GUILESS_MAIN(MeetingsTest)
#include "tst_meetings.moc"
