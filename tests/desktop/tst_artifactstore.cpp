// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectNotesEmail/ArtifactStore.h"
#include "ProjectNotesEmail/EmailSettingsStore.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace PN::Comm;

class ArtifactStoreTest final : public QObject {
    Q_OBJECT
private slots:
    void settingsAreProfileAndDatabaseScoped();
    void stagesImmutableAttachmentsAndRejectsTraversal();
    void rejectsOperationIdCollisionsWithoutReplacingRecoveryData();
    void recoversAndCleansOnlyOwnedArtifacts();
    void cleansAllTransientArtifactsAtShutdown();
    void preservesStagingWhenPublishFails();
    void reportsCorruptAndCancelledRecovery();
    void rejectsSymlinksAndFailedStagingRoots();
    void reservesAndRegistersOnlyOperationOwnedGeneratedFiles();
    void publishesGeneratedHtmlToProjectDestination();
};

static OperationManifest operation()
{
    OperationManifest manifest;
    manifest.operationId = QUuid::createUuid();
    manifest.workflow = Workflow::StatusReport;
    manifest.databaseKey = QStringLiteral("db-key");
    manifest.projectId = QStringLiteral("project-id");
    return manifest;
}

void ArtifactStoreTest::settingsAreProfileAndDatabaseScoped()
{
    QTemporaryDir dir;
    QSettings first(dir.filePath("first.ini"), QSettings::IniFormat);
    QSettings second(dir.filePath("second.ini"), QSettings::IniFormat);
    EmailSettingsStore firstStore(first), secondStore(second);
    firstStore.setPreferredBackend(BackendId::Thunderbird);
    firstStore.setThunderbirdPath(QStringLiteral("/opt/thunderbird/thunderbird"));
    QCOMPARE(firstStore.preferredBackend(), BackendId::Thunderbird);
    QCOMPARE(firstStore.thunderbirdPath(), QStringLiteral("/opt/thunderbird/thunderbird"));
    QCOMPARE(secondStore.preferredBackend(), BackendId::Mailto);
    QCOMPARE(secondStore.thunderbirdPath(), QString());
}

void ArtifactStoreTest::stagesImmutableAttachmentsAndRejectsTraversal()
{
    QTemporaryDir dir;
    ArtifactStore store(dir.path());
    const auto manifest = operation();
    QVERIFY(store.createOperation(manifest));
    const QString source = dir.filePath("original.txt");
    { QFile file(source); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("original"); }
    const auto staged = store.stageUserAttachment(manifest.operationId, source);
    QVERIFY(staged.ok());
    { QFile file(source); QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate)); file.write("changed"); }
    QFile stagedFile(staged.artifact.absolutePath); QVERIFY(stagedFile.open(QIODevice::ReadOnly)); QCOMPARE(stagedFile.readAll(), QByteArray("original"));
    const auto duplicate = store.stageUserAttachment(manifest.operationId, source);
    QVERIFY(duplicate.ok());
    QVERIFY(duplicate.artifact.displayName != staged.artifact.displayName);
    QVERIFY(!store.stageGeneratedContent(manifest.operationId, QStringLiteral("../escape.txt"), "x", "text/plain").ok());
}

void ArtifactStoreTest::rejectsOperationIdCollisionsWithoutReplacingRecoveryData()
{
    QTemporaryDir dir;
    ArtifactStore store(dir.path());
    const auto original = operation();
    QVERIFY(store.createOperation(original));
    const auto staged = store.stageGeneratedContent(original.operationId, QStringLiteral("report.html"),
                                                    "original", QStringLiteral("text/html"));
    QVERIFY(staged.ok());

    auto colliding = original;
    colliding.projectId = QStringLiteral("replacement-project");
    ServiceError error;
    QVERIFY(!store.createOperation(colliding, &error));
    QCOMPARE(error.code, QStringLiteral("operation-already-exists"));

    const auto recovered = ArtifactStore(dir.path()).loadRecoveryStatus();
    QCOMPARE(recovered.size(), 1);
    QCOMPARE(recovered.constFirst().projectId, original.projectId);
    QCOMPARE(recovered.constFirst().artifacts.size(), 1);
    QCOMPARE(recovered.constFirst().artifacts.constFirst().absolutePath, staged.artifact.absolutePath);
}

void ArtifactStoreTest::recoversAndCleansOnlyOwnedArtifacts()
{
    QTemporaryDir dir;
    ArtifactStore store(dir.path());
    const auto manifest = operation();
    QVERIFY(store.createOperation(manifest));
    const auto staged = store.stageGeneratedContent(manifest.operationId, QStringLiteral("report.html"), "<p>safe</p>", "text/html");
    QVERIFY(staged.ok());
    ArtifactStore restarted(dir.path());
    QCOMPARE(restarted.loadRecoveryStatus().size(), 1);
    QVERIFY(restarted.cleanup(manifest.operationId));
    QVERIFY(!QFileInfo::exists(staged.artifact.absolutePath));
    ServiceError error;
    QVERIFY(!restarted.cleanup(QUuid::createUuid(), &error));
    QCOMPARE(error.code, QStringLiteral("manifest-not-found"));
}

void ArtifactStoreTest::cleansAllTransientArtifactsAtShutdown()
{
    QTemporaryDir dir;
    ArtifactStore store(dir.path());
    const auto first = operation();
    auto second = operation();
    QVERIFY(store.createOperation(first));
    QVERIFY(store.createOperation(second));
    const auto firstReport = store.stageGeneratedContent(first.operationId, QStringLiteral("first.html"),
                                                          "first", QStringLiteral("text/html"));
    const auto secondReport = store.stageGeneratedContent(second.operationId, QStringLiteral("second.pdf"),
                                                           "second", QStringLiteral("application/pdf"));
    QVERIFY(firstReport.ok());
    QVERIFY(secondReport.ok());
    QVERIFY(store.cleanupAll());
    QVERIFY(!QFileInfo::exists(store.stagingRoot()));
    QVERIFY(!QFileInfo::exists(firstReport.artifact.absolutePath));
    QVERIFY(!QFileInfo::exists(secondReport.artifact.absolutePath));
}

void ArtifactStoreTest::preservesStagingWhenPublishFails()
{
    QTemporaryDir dir;
    ArtifactStore store(dir.path());
    const auto manifest = operation();
    QVERIFY(store.createOperation(manifest));
    const auto staged = store.stageGeneratedContent(manifest.operationId, QStringLiteral("report.html"), "content", "text/html");
    QVERIFY(staged.ok());
    const QString existing = dir.filePath("existing.html");
    { QFile file(existing); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("keep"); }
    ServiceError error;
    QVERIFY(!store.publish(staged.artifact, existing, &error));
    QCOMPARE(error.code, QStringLiteral("destination-exists"));
    QVERIFY(QFileInfo::exists(staged.artifact.absolutePath));

    const QString blockedParent = dir.filePath("not-a-directory");
    { QFile file(blockedParent); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("block"); }
    QVERIFY(!store.publish(staged.artifact, blockedParent + QStringLiteral("/report.html"), &error));
    QCOMPARE(error.code, QStringLiteral("destination-unwritable"));
    QVERIFY(QFileInfo::exists(staged.artifact.absolutePath));

    { QFile file(staged.artifact.absolutePath); QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate)); file.write("tampered"); }
    QVERIFY(!store.publish(staged.artifact, dir.filePath("tampered-report.html"), &error));
    QCOMPARE(error.code, QStringLiteral("publish-source-invalid"));
    QVERIFY(QFileInfo::exists(staged.artifact.absolutePath));
}

void ArtifactStoreTest::reportsCorruptAndCancelledRecovery()
{
    QTemporaryDir dir;
    ArtifactStore store(dir.path());
    auto cancelled = operation();
    cancelled.state = OperationState::Cancelled;
    QVERIFY(store.createOperation(cancelled));

    const QUuid corruptId = QUuid::createUuid();
    const QString corruptDir = QDir(store.stagingRoot()).filePath(corruptId.toString(QUuid::WithoutBraces));
    QVERIFY(QDir().mkpath(corruptDir));
    { QFile file(QDir(corruptDir).filePath("manifest.json")); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("not json"); }
    const QUuid partialId = QUuid::createUuid();
    const QString partialDir = QDir(store.stagingRoot()).filePath(partialId.toString(QUuid::WithoutBraces));
    QVERIFY(QDir().mkpath(partialDir));
    { QFile file(QDir(partialDir).filePath("manifest.json")); QVERIFY(file.open(QIODevice::WriteOnly));
      file.write(QByteArrayLiteral("{\"schemaVersion\":1,\"operationId\":\"")
                 + partialId.toString(QUuid::WithoutBraces).toUtf8() + QByteArrayLiteral("\"}")); }

    QList<ServiceError> diagnostics;
    const auto recovered = store.loadRecoveryStatus(&diagnostics);
    QCOMPARE(recovered.size(), 1);
    QCOMPARE(recovered.constFirst().state, OperationState::Cancelled);
    QCOMPARE(diagnostics.size(), 2);
    QCOMPARE(diagnostics.constFirst().code, QStringLiteral("manifest-corrupt"));
    QVERIFY(QFileInfo::exists(QDir(corruptDir).filePath("manifest.corrupt.json")));
    QVERIFY(QFileInfo::exists(QDir(partialDir).filePath("manifest.corrupt.json")));

    Artifact outside;
    outside.artifactId = QUuid::createUuid();
    outside.absolutePath = dir.filePath("user-original.txt");
    auto hostile = operation();
    hostile.artifacts.append(outside);
    QVERIFY(store.createOperation(hostile));
    ServiceError error;
    QVERIFY(!store.cleanup(hostile.operationId, &error));
    QCOMPARE(error.code, QStringLiteral("cleanup-path-rejected"));
}

void ArtifactStoreTest::rejectsSymlinksAndFailedStagingRoots()
{
    QTemporaryDir dir;
    ArtifactStore store(dir.path());
    const auto manifest = operation();
    QVERIFY(store.createOperation(manifest));
    const QString original = dir.filePath("original.txt");
    { QFile file(original); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("original"); }
    const QString link = dir.filePath("original-link.txt");
    QVERIFY(QFile::link(original, link));
    const auto staged = store.stageUserAttachment(manifest.operationId, link);
    QVERIFY(!staged.ok());
    QCOMPARE(staged.error.code, QStringLiteral("attachment-source-invalid"));

    const QString rootFile = dir.filePath("not-a-directory");
    { QFile file(rootFile); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("x"); }
    ArtifactStore invalid(rootFile);
    ServiceError error;
    QVERIFY(!invalid.createOperation(operation(), &error));
    QCOMPARE(error.code, QStringLiteral("staging-create-failed"));
}

void ArtifactStoreTest::reservesAndRegistersOnlyOperationOwnedGeneratedFiles()
{
    QTemporaryDir dir;
    ArtifactStore store(dir.path());
    const auto manifest = operation();
    QVERIFY(store.createOperation(manifest));
    ServiceError error;
    const QString reserved = store.reserveGeneratedPath(manifest.operationId, QStringLiteral("report.pdf"), &error);
    QVERIFY2(!reserved.isEmpty(), qPrintable(error.code));
    QVERIFY(!QFileInfo::exists(reserved));
    { QFile file(reserved); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("%PDF-synthetic"); }
    const auto registered = store.registerGeneratedFile(manifest.operationId, reserved, QStringLiteral("report.pdf"), QStringLiteral("application/pdf"));
    QVERIFY(registered.ok());
    QVERIFY(registered.artifact.generatedByApp);
    QCOMPARE(registered.artifact.absolutePath, reserved);
    const auto duplicate = store.registerGeneratedFile(manifest.operationId, reserved, QStringLiteral("report.pdf"), QStringLiteral("application/pdf"));
    QVERIFY(!duplicate.ok());
    QCOMPARE(duplicate.error.code, QStringLiteral("artifact-already-registered"));
    const QString lexicalEscape = QDir(QFileInfo(reserved).absolutePath()).filePath(QStringLiteral("../manifest.json"));
    const auto escapedManifest = store.registerGeneratedFile(manifest.operationId, lexicalEscape,
                                                              QStringLiteral("manifest.json"), QStringLiteral("application/json"));
    QVERIFY(!escapedManifest.ok());
    QCOMPARE(escapedManifest.error.code, QStringLiteral("artifact-path-rejected"));
    const auto escaped = store.registerGeneratedFile(manifest.operationId, dir.filePath("outside.pdf"), QStringLiteral("outside.pdf"), QStringLiteral("application/pdf"));
    QVERIFY(!escaped.ok());
    QCOMPARE(escaped.error.code, QStringLiteral("artifact-path-rejected"));
}

void ArtifactStoreTest::publishesGeneratedHtmlToProjectDestination()
{
    QTemporaryDir profile;
    QTemporaryDir project;
    QVERIFY(profile.isValid());
    QVERIFY(project.isValid());
    ArtifactStore store(profile.path());
    const OperationManifest manifest = operation();
    QVERIFY(store.createOperation(manifest));
    const auto staged = store.stageGeneratedContent(manifest.operationId, QStringLiteral("Status Report.html"),
                                                    QByteArrayLiteral("<p>generated</p>"), QStringLiteral("text/html"));
    QVERIFY(staged.ok());
    // The save picker supplies an already-chosen absolute destination, so the
    // store is handed a concrete file path rather than resolving one itself.
    QDir projectDir(project.path());
    QVERIFY(projectDir.mkpath(QStringLiteral("Project Management/Status Reports")));
    const QString destinationPath = projectDir.filePath(
        QStringLiteral("Project Management/Status Reports/") + staged.artifact.displayName);
    ServiceError error;
    QVERIFY(store.publish(staged.artifact, destinationPath, &error));
    QFile published(destinationPath);
    QVERIFY(published.open(QIODevice::ReadOnly));
    QCOMPARE(published.readAll(), QByteArrayLiteral("<p>generated</p>"));
}

QTEST_GUILESS_MAIN(ArtifactStoreTest)
#include "tst_artifactstore.moc"
