// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectNotesEmail/CommunicationRepository.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>

using namespace PN::Comm;

class CommunicationRepositoryTest final : public QObject {
    Q_OBJECT
private slots:
    void loadsTypedSnapshotWithoutUiFilters();
    void preservesInvalidNoteDateAsInvalid();
    void rejectsMissingProject();
    void validatesSelectedNotesAndAttendeeProvenance();
    void scopesMeetingAttendeesToRequestedNotesAndExcludesDeletedPeople();
    void loadsProjectWideMeetingAttendeesAndActions();
    void fingerprintTracksContentAndGeneration();
    void deliversAsyncResultOnOwnerThreadAndRejectsStaleDatabase();
    void waitsForSharedWriterLockBeforeReading();
    void cancellationSuppressesAsyncCallback();
private:
    QTemporaryDir m_dir;
    QReadWriteLock m_lock;
    QString databasePath() const { return m_dir.filePath("snapshot.db"); }
    void createDatabase();
};

void CommunicationRepositoryTest::createDatabase()
{
    QFile::remove(databasePath());
    const QString connection = QStringLiteral("repo-fixture");
    auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
    db.setDatabaseName(databasePath()); QVERIFY(db.open());
    QSqlQuery q(db);
    QVERIFY(q.exec("CREATE TABLE projects(id TEXT,project_number TEXT,project_name TEXT,client_id TEXT,status_report_period TEXT,budget TEXT,actual TEXT,bcwp TEXT,bcws TEXT,bac TEXT,deleted INTEGER)"));
    QVERIFY(q.exec("CREATE TABLE people(id TEXT,name TEXT,email TEXT,client_id TEXT,deleted INTEGER)"));
    QVERIFY(q.exec("CREATE TABLE clients(id TEXT,client_name TEXT,deleted INTEGER)"));
    QVERIFY(q.exec("CREATE TABLE project_people(id TEXT,project_id TEXT,people_id TEXT,receive_status_report INTEGER,deleted INTEGER)"));
    QVERIFY(q.exec("CREATE TABLE project_notes(id TEXT,project_id TEXT,note_title TEXT,note_date INTEGER,note TEXT,internal_item INTEGER,deleted INTEGER)"));
    QVERIFY(q.exec("CREATE TABLE meeting_attendees(id TEXT,note_id TEXT,person_id TEXT,deleted INTEGER)"));
    QVERIFY(q.exec("CREATE TABLE item_tracker(id TEXT,item_number TEXT,item_type TEXT,item_name TEXT,identified_by TEXT,date_identified TEXT,description TEXT,assigned_to TEXT,priority TEXT,status TEXT,date_due TEXT,last_update TEXT,date_resolved TEXT,note_id TEXT,project_id TEXT,internal_item INTEGER,deleted INTEGER)"));
    QVERIFY(q.exec("CREATE TABLE item_tracker_updates(id TEXT,item_id TEXT,update_note TEXT)"));
    QVERIFY(q.exec("CREATE TABLE status_report_items(id TEXT,project_id TEXT,task_category TEXT,task_description TEXT,deleted INTEGER)"));
    QVERIFY(q.exec("CREATE TABLE project_locations(id TEXT,project_id TEXT,location_type TEXT,location_description TEXT,full_path TEXT,deleted INTEGER)"));
    QVERIFY(q.exec("INSERT INTO projects VALUES('p','001','Project','client','Weekly','100','25','30','35','100',0)"));
    QVERIFY(q.exec("INSERT INTO project_locations VALUES('folder-other','p','File Folder','Other Folder','/synthetic/other',0)"));
    QVERIFY(q.exec("INSERT INTO project_locations VALUES('folder-project','p','File Folder','Project Folder','/synthetic/project',0)"));
    QVERIFY(q.exec("INSERT INTO project_locations VALUES('folder-deleted','p','File Folder','Project Folder','/synthetic/deleted',1)"));
    QVERIFY(q.exec("INSERT INTO people VALUES('a','Alice','a@example.test','client',0)"));
    QVERIFY(q.exec("INSERT INTO people VALUES('b','Bob','b@example.test','gone',0)"));
    QVERIFY(q.exec("INSERT INTO clients VALUES('client','Client',0)"));
    QVERIFY(q.exec("INSERT INTO project_people VALUES('1','p','a',1,0)"));
    QVERIFY(q.exec("INSERT INTO project_people VALUES('2','p','b',0,0)"));
    QVERIFY(q.exec("INSERT INTO project_notes VALUES('n','p','Meeting',1758000000,'<p>notes</p>',0,0)"));
    QVERIFY(q.exec("INSERT INTO meeting_attendees VALUES('attendee','n','a',0)"));
    QVERIFY(q.exec("INSERT INTO item_tracker VALUES('item','0001','Tracker','Follow up','a','09/10/2026','Check it','a','High','Assigned','09/20/2026','09/11/2026','','n','p',0,0)"));
    QVERIFY(q.exec("INSERT INTO item_tracker_updates VALUES('update','item','Still open')"));
    QVERIFY(q.exec("INSERT INTO status_report_items VALUES('status','p','In Progress','Build report',0)"));
    db.close(); db = QSqlDatabase(); QSqlDatabase::removeDatabase(connection);
}

void CommunicationRepositoryTest::loadsTypedSnapshotWithoutUiFilters()
{
    createDatabase();
    SqliteSnapshotLoader loader(databasePath(), &m_lock);
    SnapshotRequest request; request.operationId=QUuid::createUuid(); request.previewRevision=3; request.source.projectId="p";
    request.managingCompanyId = QStringLiteral("ours");
    request.projectManagerId = QStringLiteral("a");
    const auto result=loader.load(request);
    QVERIFY(result.error.code.isEmpty()); QCOMPARE(result.previewRevision,quint64(3)); QCOMPARE(result.snapshot.people.size(),2);
    QCOMPARE(result.snapshot.people.at(1).companyName,QString()); QVERIFY(!result.snapshot.fingerprint.isEmpty());
    QCOMPARE(result.snapshot.managingCompanyId, QStringLiteral("ours"));
    QCOMPARE(result.snapshot.projectManagerId, QStringLiteral("a"));
    QCOMPARE(result.snapshot.projectFolderPath, QStringLiteral("/synthetic/project"));
    QCOMPARE(result.snapshot.notes.size(), 1);
    QCOMPARE(result.snapshot.statusReportPeriod, QStringLiteral("Weekly"));
    QCOMPARE(result.snapshot.statusItems.constFirst().description, QStringLiteral("Build report"));
    QCOMPARE(result.snapshot.trackerItems.constFirst().comments, QStringLiteral("Still open"));

    request.operationId = QUuid::createUuid();
    request.source.workflow = Workflow::StatusReport;
    const auto statusResult = loader.load(request);
    QVERIFY(statusResult.error.code.isEmpty());
    QCOMPARE(statusResult.snapshot.currentSelectionPersonIds, QStringList({QStringLiteral("a")}));
}

void CommunicationRepositoryTest::preservesInvalidNoteDateAsInvalid()
{
    createDatabase();
    const QString connection = QStringLiteral("invalid-date-fixture");
    auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
    db.setDatabaseName(databasePath());
    QVERIFY(db.open());
    QSqlQuery query(db);
    QVERIFY(query.exec("UPDATE project_notes SET note_date='not-an-epoch' WHERE id='n'"));
    db.close(); db = QSqlDatabase(); QSqlDatabase::removeDatabase(connection);

    SqliteSnapshotLoader loader(databasePath(), &m_lock);
    SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source.projectId = QStringLiteral("p");
    const auto result = loader.load(request);
    QVERIFY(result.error.code.isEmpty());
    QCOMPARE(result.snapshot.notes.size(), 1);
    QVERIFY(!result.snapshot.notes.constFirst().date.isValid());
}

void CommunicationRepositoryTest::rejectsMissingProject()
{
    createDatabase(); SqliteSnapshotLoader loader(databasePath(), &m_lock); SnapshotRequest request; request.operationId=QUuid::createUuid(); request.source.projectId="missing";
    QCOMPARE(loader.load(request).error.code,QStringLiteral("project-not-found"));
}

void CommunicationRepositoryTest::validatesSelectedNotesAndAttendeeProvenance()
{
    createDatabase();
    SqliteSnapshotLoader loader(databasePath(), &m_lock);
    SnapshotRequest request; request.operationId=QUuid::createUuid(); request.source.projectId="p"; request.source.noteIds={"n"};
    const auto selected = loader.load(request);
    QVERIFY(selected.error.code.isEmpty());
    QCOMPARE(selected.snapshot.currentSelectionIds, QStringList({"n"}));
    QCOMPARE(selected.snapshot.currentSelectionPersonIds, QStringList({"a", "b"}));
    QVERIFY(selected.snapshot.people.at(0).meetingAttendee);
    QCOMPARE(selected.snapshot.notes.constFirst().attendeeIds, QStringList({"a"}));
    QCOMPARE(selected.snapshot.actionItems.constFirst().name, QStringLiteral("Follow up"));
    request.source.noteIds={"not-p"};
    QCOMPARE(loader.load(request).error.code, QStringLiteral("note-not-in-project"));
}

void CommunicationRepositoryTest::scopesMeetingAttendeesToRequestedNotesAndExcludesDeletedPeople()
{
    createDatabase();
    const QString connection = QStringLiteral("selected-attendee-fixture");
    auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
    db.setDatabaseName(databasePath());
    QVERIFY(db.open());
    QSqlQuery query(db);
    QVERIFY(query.exec("INSERT INTO people VALUES('c','Casey','c@example.test','external',0)"));
    QVERIFY(query.exec("INSERT INTO people VALUES('d','Deleted','d@example.test','external',1)"));
    QVERIFY(query.exec("INSERT INTO project_notes VALUES('n2','p','Other meeting',1759000000,'<p>other</p>',0,0)"));
    QVERIFY(query.exec("INSERT INTO meeting_attendees VALUES('attendee2','n2','c',0)"));
    QVERIFY(query.exec("INSERT INTO meeting_attendees VALUES('attendee3','n','d',0)"));
    db.close(); db = QSqlDatabase(); QSqlDatabase::removeDatabase(connection);

    SqliteSnapshotLoader loader(databasePath(), &m_lock);
    SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source.projectId = QStringLiteral("p");
    request.source.noteIds = {QStringLiteral("n")};
    const auto result = loader.load(request);

    QVERIFY(result.error.code.isEmpty());
    QCOMPARE(result.snapshot.currentSelectionIds, QStringList({QStringLiteral("n")}));
    QCOMPARE(result.snapshot.currentSelectionPersonIds, QStringList({QStringLiteral("a"), QStringLiteral("b")}));
    const auto selectedNote = std::find_if(result.snapshot.notes.cbegin(), result.snapshot.notes.cend(),
                                           [](const SnapshotNote &note) { return note.id == QStringLiteral("n"); });
    QVERIFY(selectedNote != result.snapshot.notes.cend());
    QCOMPARE(selectedNote->attendeeIds, QStringList({QStringLiteral("a")}));
    QCOMPARE(result.snapshot.people.size(), 2);
    QVERIFY(std::none_of(result.snapshot.people.cbegin(), result.snapshot.people.cend(),
                         [](const SnapshotPerson &person) { return person.id == QStringLiteral("c") || person.id == QStringLiteral("d"); }));
}

void CommunicationRepositoryTest::loadsProjectWideMeetingAttendeesAndActions()
{
    createDatabase();
    const QString connection = QStringLiteral("project-wide-fixture");
    auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
    db.setDatabaseName(databasePath());
    QVERIFY(db.open());
    QSqlQuery query(db);
    QVERIFY(query.exec("INSERT INTO people VALUES('c','Casey','c@example.test','external',0)"));
    QVERIFY(query.exec("INSERT INTO project_notes VALUES('n2','p','Second meeting',1759000000,'<p>second</p>',0,0)"));
    QVERIFY(query.exec("INSERT INTO meeting_attendees VALUES('attendee2','n2','b',0)"));
    QVERIFY(query.exec("INSERT INTO meeting_attendees VALUES('attendee3','n2','c',0)"));
    QVERIFY(query.exec("INSERT INTO item_tracker VALUES('item2','0002','Action','Second follow up','b','09/11/2026','Check two','b','Medium','New','09/21/2026','09/12/2026','','n2','p',0,0)"));
    db.close(); db = QSqlDatabase(); QSqlDatabase::removeDatabase(connection);

    SqliteSnapshotLoader loader(databasePath(), &m_lock);
    SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source.projectId = QStringLiteral("p");
    request.source.workflow = Workflow::MeetingNotesReport;
    const auto result = loader.load(request);

    QVERIFY(result.error.code.isEmpty());
    QCOMPARE(result.snapshot.currentSelectionIds, QStringList({QStringLiteral("n"), QStringLiteral("n2")}));
    QCOMPARE(result.snapshot.actionItems.size(), 2);
    const auto firstNote = std::find_if(result.snapshot.notes.cbegin(), result.snapshot.notes.cend(),
                                        [](const SnapshotNote &note) { return note.id == QStringLiteral("n"); });
    const auto secondNote = std::find_if(result.snapshot.notes.cbegin(), result.snapshot.notes.cend(),
                                         [](const SnapshotNote &note) { return note.id == QStringLiteral("n2"); });
    QVERIFY(firstNote != result.snapshot.notes.cend());
    QVERIFY(secondNote != result.snapshot.notes.cend());
    QCOMPARE(firstNote->attendeeIds, QStringList({QStringLiteral("a")}));
    QCOMPARE(secondNote->attendeeIds, QStringList({QStringLiteral("b"), QStringLiteral("c")}));
    QCOMPARE(result.snapshot.people.size(), 3);
    QVERIFY(result.snapshot.people.at(0).meetingAttendee);
    QVERIFY(result.snapshot.people.at(1).meetingAttendee);
    QCOMPARE(result.snapshot.people.at(2).name, QStringLiteral("Casey"));
    QVERIFY(result.snapshot.people.at(2).meetingAttendee);
    QVERIFY(!result.snapshot.people.at(2).receivesStatus);
    QVERIFY(!result.snapshot.people.at(2).projectTeamMember);
    QCOMPARE(result.snapshot.currentSelectionPersonIds, QStringList({QStringLiteral("a"), QStringLiteral("b")}));
}

void CommunicationRepositoryTest::fingerprintTracksContentAndGeneration()
{
    createDatabase();
    SqliteSnapshotLoader loader(databasePath(), &m_lock);
    SnapshotRequest request; request.operationId = QUuid::createUuid(); request.source.projectId = "p";
    request.source.databaseGeneration = 1;
    const auto before = loader.load(request); QVERIFY(before.error.code.isEmpty());
    const QString connection = QStringLiteral("fingerprint-change");
    auto db = QSqlDatabase::addDatabase("QSQLITE", connection); db.setDatabaseName(databasePath()); QVERIFY(db.open());
    QSqlQuery update(db); QVERIFY(update.exec("UPDATE project_notes SET note='<p>changed</p>' WHERE id='n'"));
    db.close(); db = QSqlDatabase(); QSqlDatabase::removeDatabase(connection);
    const auto after = loader.load(request); QVERIFY(after.error.code.isEmpty());
    QVERIFY(before.snapshot.fingerprint != after.snapshot.fingerprint);
    request.source.databaseGeneration = 2;
    const auto changedGeneration = loader.load(request); QVERIFY(changedGeneration.error.code.isEmpty());
    QVERIFY(after.snapshot.fingerprint != changedGeneration.snapshot.fingerprint);
}

void CommunicationRepositoryTest::deliversAsyncResultOnOwnerThreadAndRejectsStaleDatabase()
{
    createDatabase();
    AsyncSqliteCommunicationRepository repository(databasePath(), &m_lock, 7);
    SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.previewRevision = 4;
    request.source.projectId = QStringLiteral("p");
    request.source.databaseGeneration = 7;

    bool completed = false;
    repository.loadSnapshot(request, [&](SnapshotResult result) {
        QCOMPARE(QThread::currentThread(), repository.thread());
        QVERIFY(result.error.code.isEmpty());
        QCOMPARE(result.operationId, request.operationId);
        QCOMPARE(result.previewRevision, quint64(4));
        completed = true;
    });
    QTRY_VERIFY(completed);

    request.operationId = QUuid::createUuid();
    request.source.databaseGeneration = 8;
    repository.loadSnapshot(request, [&](SnapshotResult result) {
        QCOMPARE(result.error.code, QStringLiteral("database-generation-changed"));
        completed = true;
    });
}

void CommunicationRepositoryTest::waitsForSharedWriterLockBeforeReading()
{
    createDatabase();
    AsyncSqliteCommunicationRepository repository(databasePath(), &m_lock, 1);
    SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source.projectId = QStringLiteral("p");
    request.source.databaseGeneration = 1;
    bool completed = false;

    m_lock.lockForWrite();
    repository.loadSnapshot(request, [&completed](SnapshotResult result) {
        QVERIFY(result.error.code.isEmpty());
        completed = true;
    });
    QTest::qWait(50);
    QVERIFY(!completed);
    m_lock.unlock();

    QTRY_VERIFY(completed);
}

void CommunicationRepositoryTest::cancellationSuppressesAsyncCallback()
{
    createDatabase();
    AsyncSqliteCommunicationRepository repository(databasePath(), &m_lock, 1);
    SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source.projectId = QStringLiteral("p");
    request.source.databaseGeneration = 1;
    bool completed = false;
    repository.cancel(request.operationId);
    repository.loadSnapshot(request, [&](SnapshotResult) { completed = true; });
    QTest::qWait(50);
    QVERIFY(!completed);
}

QTEST_GUILESS_MAIN(CommunicationRepositoryTest)
#include "tst_communicationrepository.moc"
