// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "FileFinderWorker.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QReadWriteLock>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class FileFinderTest final : public QObject
{
    Q_OBJECT

private slots:
    void reconcilesOnlyActiveProjectsAndAdoptsLegacyRows();
};

void FileFinderTest::reconcilesOnlyActiveProjectsAndAdoptsLegacyRows()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());

    const QString searchRoot = temporary.filePath(QStringLiteral("Projects"));
    const QString activeFolder = searchRoot + QStringLiteral("/1001 - Active Project");
    const QString quotesFolder = activeFolder + QStringLiteral("/Project Management/Quotes");
    const QString quotePath = quotesFolder + QStringLiteral("/Proposal.pdf");
    const QString closedFolder = searchRoot + QStringLiteral("/2002 - Closed Project");
    QVERIFY(QDir().mkpath(quotesFolder));
    QVERIFY(QDir().mkpath(closedFolder));
    QFile quote(quotePath);
    QVERIFY(quote.open(QIODevice::WriteOnly));
    quote.write("test");
    quote.close();

    const QString databasePath = temporary.filePath(QStringLiteral("ProjectNotes.db"));
    const QString setupConnection = QStringLiteral("FileFinderTestSetup");
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                          setupConnection);
        database.setDatabaseName(databasePath);
        QVERIFY2(database.open(), qPrintable(database.lastError().text()));
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE projects (id TEXT PRIMARY KEY, project_number TEXT, "
            "project_status TEXT, deleted INTEGER DEFAULT 0)")));
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE project_locations (id TEXT PRIMARY KEY, project_id TEXT, "
            "location_type TEXT, location_description TEXT, full_path TEXT, "
            "updateddate INTEGER, syncdate INTEGER, deleted INTEGER DEFAULT 0)")));
        QVERIFY(query.exec(QStringLiteral(
            "CREATE UNIQUE INDEX project_location_desc ON project_locations "
            "(project_id, location_description) WHERE deleted = 0")));
        QVERIFY(query.exec(QStringLiteral(
            "INSERT INTO projects VALUES ('active-id', '1001', 'Active', 0)")));
        QVERIFY(query.exec(QStringLiteral(
            "INSERT INTO projects VALUES ('closed-id', '2002', 'Closed', 0)")));

        query.prepare(QStringLiteral(
            "INSERT INTO project_locations VALUES "
            "('legacy-id', 'active-id', 'PDF File', 'Quote', ?, 1, NULL, 0)"));
        query.addBindValue(QFileInfo(quotePath).canonicalFilePath());
        QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
        database.close();
    }
    QSqlDatabase::removeDatabase(setupConnection);

    QReadWriteLock sharedLock;
    FileFinderWorker worker;
    worker.initializeDatabase(databasePath, &sharedLock);
    FileFinderConfiguration configuration;
    configuration.enabled = true;
    configuration.roots = {searchRoot};
    configuration.rules = {{QStringLiteral("Quote"),
                            QStringLiteral(R"(.*Project Management/Quotes.*\.pdf$)")}};
    worker.configure(configuration);

    QSignalSpy scans(&worker, &FileFinderWorker::scanFinished);
    worker.scanNow();
    QCOMPARE(scans.count(), 1);
    FileFinderScanSummary first = qvariant_cast<FileFinderScanSummary>(scans.takeFirst().at(0));
    QCOMPARE(first.projects, 1);
    QCOMPARE(first.files, 1);
    QCOMPARE(first.matched, 1);
    QCOMPARE(first.inserted, 1); // the project folder
    QCOMPARE(first.updated, 1);  // the existing legacy file row was adopted
    QVERIFY(first.error.isEmpty());

    worker.scanNow();
    QCOMPARE(scans.count(), 1);
    FileFinderScanSummary second = qvariant_cast<FileFinderScanSummary>(scans.takeFirst().at(0));
    QCOMPARE(second.inserted, 0);
    QCOMPARE(second.updated, 0);
    QCOMPARE(second.unchanged, 2);

    worker.closeDatabase();
    const QString verifyConnection = QStringLiteral("FileFinderTestVerify");
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                          verifyConnection);
        database.setDatabaseName(databasePath);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral(
            "SELECT id, location_description FROM project_locations "
            "WHERE project_id = 'active-id' ORDER BY location_description")));
        QVERIFY(query.next());
        QCOMPARE(query.value(1).toString(), QStringLiteral("File Finder: Project Folder"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("legacy-id"));
        QVERIFY(query.value(1).toString().startsWith(QStringLiteral("File Finder: Quote :")));
        QVERIFY(!query.next());
        QVERIFY(query.exec(QStringLiteral(
            "SELECT count(*) FROM project_locations WHERE project_id = 'closed-id'")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
        database.close();
    }
    QSqlDatabase::removeDatabase(verifyConnection);
}

QTEST_GUILESS_MAIN(FileFinderTest)
#include "tst_filefinder.moc"
