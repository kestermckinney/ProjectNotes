// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#include "ProjectNotesEmail/ProjectEmailPreparationFactory.h"
#include "ProjectNotesEmail/LocationAttachmentDownloader.h"
#include "ProjectNotesEmail/TransientCacheCleaner.h"
#include "ProjectNotesEmail/EmlWriter.h"
#include "ProjectNotesEmail/CommunicationsController.h"
#include <QFile>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

using namespace PN::Comm;

class ProjectEmailTest final : public QObject {
    Q_OBJECT
private:
    CommunicationSnapshot snapshot() const {
        CommunicationSnapshot value;
        value.projectId = "project"; value.projectNumber = "P1"; value.projectName = "A & B";
        value.databaseGeneration = 3;
        value.projectManagerId = "manager";
        value.people = {{"manager", "Manager", "manager@example.test", {}, {}, false, false, true},
                        {"person", "<Alice>", "alice@example.test", {}, {}, false, false, true},
                        {"attendee", "Attendee", "attendee@example.test", {}, {}, false, true, false}};
        value.location = CommunicationSnapshot::Location{"location", "Spec & notes", "https://example.test/spec.pdf?x=1&y=2"};
        return value;
    }
    SourceContext source(Workflow workflow) const { return {"db", 3, "project", {}, workflow}; }
private slots:
    void newEmailReviewsRejectSubjectAndBodyEdits() {
        for (const auto workflow : {Workflow::ProjectEmail, Workflow::ShareLocation, Workflow::PersonEmail}) {
            CommunicationsController controller(nullptr);
            EmailPreparation preparation;
            preparation.source.workflow = workflow;
            preparation.document.defaultSubject = workflow == Workflow::PersonEmail ? "" : "Configured subject";
            preparation.document.emailFragment = workflow == Workflow::PersonEmail ? "" : "<p>Configured body</p>";
            const auto subject = preparation.document.defaultSubject;
            const auto body = preparation.document.emailFragment;
            QVERIFY(controller.setPreparation(preparation));
            QVERIFY(!controller.setSubject("Changed subject"));
            QVERIFY(!controller.setHtmlBody("<p>Changed body</p>"));
            QCOMPARE(controller.subject(), subject);
            QCOMPARE(controller.htmlBody(), body);
        }
    }
    void personEmailUsesConfiguredSubjectBodyAndOnlyClickedPerson() {
        auto context = source(Workflow::ProjectEmail); context.personId = "person";
        CommunicationTemplate value{"custom", "Custom", Workflow::ProjectEmail,
            "{{ project.number }}: {{ person.name }}", "<p>Hello {{ person.name }} at {{ person.email }}; {{ project.name }}</p>"};
        const auto review = ProjectEmailPreparationFactory::create(snapshot(), context, BackendId::DefaultApp, value);
        QVERIFY(review.validation.ok());
        QCOMPARE(review.preparation.document.defaultSubject, "P1: <Alice>");
        QVERIFY(review.preparation.document.emailFragment.contains("&lt;Alice&gt;"));
        QVERIFY(review.preparation.document.emailFragment.contains("A &amp; B"));
        QCOMPARE(review.preparation.recipients.size(), 1);
        QCOMPARE(review.preparation.recipients.first().address, "alice@example.test");
        QCOMPARE(review.audience.people.size(), 2);
        const auto editedAudience = applyRecipientOverrides(review.audience,
            {{"person", false, RecipientRole::To}, {"manager", true, RecipientRole::Cc}});
        QCOMPARE(editedAudience.recipients.size(), 1);
        QCOMPARE(editedAudience.recipients.first().address, "manager@example.test");
        QCOMPARE(editedAudience.recipients.first().role, RecipientRole::Cc);
        QVERIFY(EmailContentBuilder::prepare(review.preparation).request.has_value());
        context.personId = "attendee";
        const auto attendee = ProjectEmailPreparationFactory::create(snapshot(), context, BackendId::DefaultApp, value);
        QVERIFY(attendee.validation.ok());
        QCOMPARE(attendee.preparation.recipients.first().address, "attendee@example.test");
        context.personId = "missing";
        QVERIFY(!ProjectEmailPreparationFactory::create(snapshot(), context, BackendId::DefaultApp, value).validation.ok());
        auto invalidPerson = snapshot(); invalidPerson.people[1].email.clear();
        context.personId = "person";
        QVERIFY(!ProjectEmailPreparationFactory::create(invalidPerson, context, BackendId::DefaultApp, value).validation.ok());
    }
    void standalonePersonEmailIsBlankAndNeedsNoProjectOrTemplate() {
        auto value = snapshot(); value.projectId.clear(); value.location.reset();
        auto context = source(Workflow::PersonEmail); context.projectId.clear(); context.personId = "attendee";
        const auto review = ProjectEmailPreparationFactory::create(value, context, BackendId::DefaultApp, {});
        QVERIFY(review.validation.ok());
        QVERIFY(review.preparation.document.defaultSubject.isEmpty());
        QVERIFY(review.preparation.document.emailFragment.isEmpty());
        QCOMPARE(review.preparation.recipients.size(), 1);
        QCOMPARE(review.preparation.recipients.first().address, "attendee@example.test");
        const auto email = EmailContentBuilder::prepare(review.preparation);
        QVERIFY(email.request.has_value());
        ValidationResult emlValidation;
        const auto eml = EmlWriter::write(*email.request, {}, &emlValidation);
        QVERIFY(emlValidation.ok());
        QVERIFY(eml.contains("attendee@example.test"));
        QVERIFY(!eml.contains("Subject:"));
        context.personId.clear();
        QVERIFY(!validate(context).ok());
        context.personId = "person";
        value.people[1].email = "not an address";
        QVERIFY(!ProjectEmailPreparationFactory::create(value, context, BackendId::DefaultApp, {}).validation.ok());
    }
    void projectEmailIncludesTeamAndManagerWithoutAttendees() {
        CommunicationTemplate value{"custom", "Custom", Workflow::ProjectEmail, "{{ project.name }}", "<p>Project email</p>"};
        const auto review = ProjectEmailPreparationFactory::create(snapshot(), source(Workflow::ProjectEmail), BackendId::DefaultApp, value);
        QVERIFY(review.validation.ok());
        QCOMPARE(review.preparation.recipients.size(), 2);
        QCOMPARE(review.preparation.recipients.first().address, "manager@example.test");
    }
    void locationOffersEncodedLinkOrDownloadAndValidatesFiles() {
        auto context = source(Workflow::ShareLocation); context.locationId = "location";
        CommunicationTemplate value{"custom", "Custom", Workflow::ShareLocation,
            "{{ project.number }} - {{ location.description }}", "<p>{{ location.fileName }}</p>{{ content.body }}"};
        auto review = ProjectEmailPreparationFactory::create(snapshot(), context, BackendId::DefaultApp, value);
        QVERIFY(review.validation.ok());
        QVERIFY(review.preparation.document.emailFragment.contains("href=\"https://example.test/spec.pdf?x=1&amp;y=2\""));
        QVERIFY(review.attachmentUrl.isEmpty());
        review = ProjectEmailPreparationFactory::create(snapshot(), context, BackendId::DefaultApp, value, true);
        QVERIFY(review.validation.ok());
        QCOMPARE(review.attachmentUrl.scheme(), "https");
        QVERIFY(!review.preparation.document.emailFragment.contains("href="));
        auto data = snapshot(); data.location->path = "javascript:alert(1)";
        QVERIFY(!ProjectEmailPreparationFactory::create(data, context, BackendId::DefaultApp, value).validation.ok());
        data.location->path = "/nonexistent/file";
        QVERIFY(!ProjectEmailPreparationFactory::create(data, context, BackendId::DefaultApp, value, true).validation.ok());
        QTemporaryDir directory;
        QFile local(directory.filePath("spec.txt")); QVERIFY(local.open(QIODevice::WriteOnly)); local.write("spec"); local.close();
        data.location->path = local.fileName();
        review = ProjectEmailPreparationFactory::create(data, context, BackendId::DefaultApp, value, true);
        QVERIFY(review.validation.ok());
        QCOMPARE(review.attachmentUrl.toLocalFile(), local.fileName());
        data.location->path = directory.path();
        QVERIFY(!ProjectEmailPreparationFactory::create(data, context, BackendId::DefaultApp, value, true).validation.ok());
    }
    void downloadStagesBytesAndExitCleanupRemovesThem_data() {
        QTest::addColumn<bool>("success"); QTest::addColumn<QByteArray>("headers");
        QTest::addColumn<QString>("expectedName"); QTest::addColumn<bool>("redirect");
        QTest::newRow("file") << true << QByteArray() << QString("spec.txt") << false;
        QTest::newRow("http-error") << false << QByteArray() << QString() << false;
        QTest::newRow("response-name") << true << QByteArray("Content-Disposition: attachment; filename=\"../../renamed.txt\"\r\n") << QString("renamed.txt") << false;
        QTest::newRow("unicode-name") << true << QByteArray("Content-Disposition: attachment; filename*=UTF-8''r%C3%A9sum%C3%A9.txt\r\n") << QString::fromUtf8("résumé.txt") << false;
        QTest::newRow("redirect") << true << QByteArray() << QString("spec.txt") << true;
    }
    void downloadStagesBytesAndExitCleanupRemovesThem() {
        QFETCH(bool, success); QFETCH(QByteArray, headers); QFETCH(QString, expectedName); QFETCH(bool, redirect);
        QTemporaryDir directory;
        ArtifactStore store(directory.path()); OperationManifest manifest;
        manifest.operationId = QUuid::createUuid(); manifest.workflow = Workflow::ShareLocation;
        QVERIFY(store.createOperation(manifest));
        QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
        connect(&server, &QTcpServer::newConnection, this, [&] {
            auto socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [socket, success, headers] {
                const auto request = socket->readAll();
                if (request.startsWith("GET /redirect"))
                    socket->write("HTTP/1.1 302 Found\r\nLocation: /spec.txt\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                else if (success)
                    socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n" + headers + "Content-Length: 4\r\nConnection: close\r\n\r\nspec");
                else socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        });
        LocationAttachmentDownloader downloader;
        std::optional<ArtifactStoreResult> result;
        downloader.download(QUrl(QStringLiteral("http://127.0.0.1:%1/%2").arg(server.serverPort()).arg(redirect ? "redirect" : "spec.txt")), directory.path(),
                            manifest.operationId, [&](ArtifactStoreResult value) { result = value; });
        QTRY_VERIFY_WITH_TIMEOUT(result.has_value(), 5000);
        QCOMPARE(result->ok(), success);
        if (success) {
            QFile file(result->artifact.absolutePath); QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), QByteArray("spec")); file.close();
            QCOMPARE(result->artifact.displayName, expectedName);
            QCOMPARE(QFileInfo(result->artifact.absolutePath).fileName(), expectedName);
            QCOMPARE(result->artifact.mimeType, "text/plain");
            QVERIFY(result->artifact.absolutePath.startsWith(store.stagingRoot()));
            QVERIFY(!result->artifact.sha256.isEmpty());
        }
        QCOMPARE(TransientCacheCleaner(directory.path()).clean(), QStringList());
        QVERIFY(!QFileInfo::exists(store.stagingRoot()));
    }
    void cancellationSuppressesCompletion() {
        QTemporaryDir directory; ArtifactStore store(directory.path()); OperationManifest manifest;
        manifest.operationId = QUuid::createUuid(); manifest.workflow = Workflow::ShareLocation;
        QVERIFY(store.createOperation(manifest));
        QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
        QSignalSpy connected(&server, &QTcpServer::newConnection);
        LocationAttachmentDownloader downloader; bool called = false;
        downloader.download(QUrl(QStringLiteral("http://127.0.0.1:%1/file.txt").arg(server.serverPort())), directory.path(),
                            manifest.operationId, [&](ArtifactStoreResult) { called = true; });
        QTRY_COMPARE_WITH_TIMEOUT(connected.count(), 1, 5000);
        downloader.cancel(); QCoreApplication::processEvents();
        QVERIFY(!called);
        const auto recovery = store.loadRecoveryStatus();
        QCOMPARE(recovery.size(), 1); QVERIFY(recovery.first().artifacts.isEmpty());
        QVERIFY(store.cleanupAll());
    }
};
QTEST_GUILESS_MAIN(ProjectEmailTest)
#include "tst_projectemail.moc"
