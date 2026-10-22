// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
//
// ParityTest — exercises the DesktopAppController bridge end to end to prove the
// QML desktop app can perform every data operation the Widgets app can:
// full CRUD for clients, people, projects, notes, meeting attendees, note action
// items, tracker items (risks/issues/action items) + uniqueness rules, tracker
// comments, team members, project locations, status-report items; plus global
// search, quick-search + column filters, view options, preferences, and XML
// export/import. Sync is intentionally out of scope.

#include <QtTest/QtTest>
#include <QAbstractItemModel>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QSqlError>
#include <QDomDocument>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUuid>

#include "DesktopAppController.h"
#include "databaseobjects.h"
#include "databasestructure.h"
#include "databaseupgrade_v6_4_0.h"
#include "FolderManager.h"
#include "legacywidgetsettings.h"
#include <QSettings>
#include <QScopeGuard>
#include "ProjectNotesEmail/RecipientSelectionModel.h"
#include "ProjectNotesEmail/CommunicationsController.h"
#include "ProjectNotesIntegrations/TemplateEditorModel.h"

namespace {

QString uniq(const QString& prefix)
{
    return prefix + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

// Does any value in the record map stringify to (contain) `needle`?
bool mapHasValue(const QVariantMap& m, const QString& needle)
{
    for (auto it = m.constBegin(); it != m.constEnd(); ++it)
        if (it.value().toString().contains(needle))
            return true;
    return false;
}

// First proxy row whose id column (0) equals `id`, or -1.
int rowForId(QAbstractItemModel* model, const QString& id)
{
    if (!model) return -1;
    for (int r = 0; r < model->rowCount(); ++r)
        if (model->data(model->index(r, 0)).toString() == id)
            return r;
    return -1;
}

// Id (column 0) of the first row whose column `col` equals `value`.
// addCompany()/addContact() only assign the record id when it is first *saved*, so
// the id is looked up by a unique field after the save (the QML app resolves it
// the same way once the row is committed).
QString idForColumnValue(QAbstractItemModel* model, int col, const QString& value)
{
    if (!model) return {};
    for (int r = 0; r < model->rowCount(); ++r)
        if (model->data(model->index(r, col)).toString() == value)
            return model->data(model->index(r, 0)).toString();
    return {};
}

// The data models display and accept dates as MM/DD/YYYY (see DateField.qml).
const char* const kDate1 = "07/31/2026";
const char* const kDate2 = "08/15/2026";

} // namespace

class ParityTest : public QObject
{
    Q_OBJECT

    DesktopAppController* c = nullptr;

    // Shared fixture ids, built up across the ordered test slots.
    QString m_clientId, m_personId, m_projectId, m_projectName, m_projectNumber;
    QString m_noteId, m_itemId;

private slots:
    void initTestCase()
    {
        c = DesktopAppController::create(nullptr, nullptr);
        QVERIFY(c);
        QVERIFY2(c->openOrCreateDatabase(), "database failed to open/create");
        QVERIFY(c->databaseOpen());
    }

    void test_localCleanupKeepsCurrentPreferences()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSettings settings(dir.filePath("settings.ini"), QSettings::IniFormat);
        const QStringList retired{"UserFilter:ClientsModel", "UserFilter:ClientsModel:IsActive",
                                  "MainWindow/Width", "tableViewProjectsColumns",
                                  "tableViewColumnName_tableViewProjectsSortColumn", "ZoomFactor"};
        const QStringList retained{"UserFilter:ShowClosedProjects", "ViewFilter:ShowResolvedTrackerItems",
                                   "UI:QMLMainWindow:Width", "UI:ZoomPercent", "DefaultDictionary",
                                   "Sync/Enabled", "Sync/Password", "UserFilter:CustomPlugin"};
        for (const QString &key : retired + retained) settings.setValue(key, "saved");
        PN::LegacyWidgetSettings::cleanupLocal(settings);
        PN::LegacyWidgetSettings::cleanupLocal(settings);
        for (const QString &key : retired) QVERIFY(!settings.contains(key));
        for (const QString &key : retained) QCOMPARE(settings.value(key).toString(), QString("saved"));
    }

    void test_settingsFailuresReleaseLockAndPreserveTombstones()
    {
        const QString connection = uniq("settings-test-");
        DatabaseObjects settings;
        settings.setGuiDialogsEnabled(false);
        settings.getDb() = QSqlDatabase::addDatabase("QSQLITE", connection);
        settings.getDb().setDatabaseName(":memory:");
        QVERIFY(settings.getDb().open());
        auto cleanup = qScopeGuard([&] {
            settings.getDb().close();
            settings.getDb() = QSqlDatabase();
            QSqlDatabase::removeDatabase(connection);
        });
        QSqlQuery q(settings.getDb());
        QVERIFY(q.exec("PRAGMA foreign_keys = ON"));
        QVERIFY(q.exec("CREATE TABLE allowed(id TEXT PRIMARY KEY)"));
        QVERIFY(q.exec("INSERT INTO allowed VALUES('old'),('new')"));
        QVERIFY(q.exec("CREATE TABLE application_settings(id TEXT PRIMARY KEY, parameter_name TEXT, "
                       "parameter_value TEXT REFERENCES allowed(id) DEFERRABLE INITIALLY DEFERRED, "
                       "updateddate INTEGER, deleted INTEGER DEFAULT 0)"));
        QVERIFY(q.exec("INSERT INTO application_settings VALUES('active','key','old',2,0),"
                       "('older','key','old',1,0),('deleted','key','old',3,1)"));
        QVERIFY(settings.saveParameter("key", "new"));
        QCOMPARE(settings.loadParameter("key"), QString("new"));
        QVERIFY(q.exec("SELECT parameter_value FROM application_settings WHERE id IN ('older','deleted')"));
        while (q.next()) QCOMPARE(q.value(0).toString(), QString("old"));
        q.finish();
        // The SQL statement succeeds, but a deferred constraint rejects commit.
        QVERIFY(!settings.saveParameter("key", "missing-parent"));
        QVERIFY(!settings.lastSaveError().isEmpty());
        QCOMPARE(settings.loadParameter("key"), QString("new"));
        QVERIFY(db_rwlock.tryLockForWrite());
        db_rwlock.unlock();
        QVERIFY(q.exec("DROP TABLE application_settings"));
        QVERIFY(!settings.saveParameter("key", "new"));
        QVERIFY(db_rwlock.tryLockForWrite());
        db_rwlock.unlock();
    }

    void test_upgrade640RetiresOnlyLegacySettings()
    {
        QSqlQuery q(global_DBObjects.getDb());
        QVERIFY(!global_DBObjects.getDb().tables().contains("project_risks"));
        const QStringList retired{"PageState:ProjectsPage:project-id", "UserFilter:ClientsModel",
                                  "UserFilter:ClientsModel:IsActive", "UserFilter:ShowClosedProjects",
                                  "UserFilter:ShowInternalItems", "ViewFilter:ShowResolvedTrackerItems"};
        const QStringList retained{"UI:ColumnFilters:review-test", "UI:SortOrder:review-test",
                                   "UserFilter:CustomPlugin", "UI:LastProjectDetailTab:review-test",
                                   "ReviewTest:CustomSetting"};
        for (const QString &key : retired + retained)
            QVERIFY(global_DBObjects.saveParameter(key, "review-test"));
        // Empty legacy tables can be dropped; unrelated extension tables remain.
        QVERIFY(q.exec("CREATE TABLE project_risks(id TEXT)"));
        QVERIFY(q.exec("CREATE TABLE review_extension_data(id TEXT)"));
        QVERIFY(q.exec("UPDATE application_version SET current_version='6.3.0'"));
        DatabaseStructure structure;
        QVERIFY(structure.UpgradeDatabase());
        QVERIFY(!global_DBObjects.getDb().tables().contains("project_risks"));
        QVERIFY(global_DBObjects.getDb().tables().contains("review_extension_data"));
        for (const QString &key : retired) QVERIFY(global_DBObjects.loadParameter(key).isEmpty());
        for (const QString &key : retained) QCOMPARE(global_DBObjects.loadParameter(key), QString("review-test"));
        QVERIFY(q.exec("SELECT current_version FROM application_version"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QString("6.5.0"));
        q.finish();
        QVERIFY(q.exec("SELECT deleted, syncdate, updateddate FROM application_settings "
                       "WHERE parameter_name='PageState:ProjectsPage:project-id'"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
        QVERIFY(q.value(1).isNull());
        QVERIFY(!q.value(2).isNull());
        q.finish();
        QVERIFY(structure.UpgradeDatabase());
        // A later sync from an old client is cleaned up on the next open too.
        QVERIFY(global_DBObjects.saveParameter(retired.first(), "old-client"));
        QVERIFY(structure.UpgradeDatabase());
        QVERIFY(global_DBObjects.loadParameter(retired.first()).isEmpty());
        QVERIFY(q.exec("CREATE TABLE project_risks(id TEXT)"));
        QVERIFY(q.exec("INSERT INTO project_risks VALUES('historical-risk')"));
        QVERIFY(db_UpgradeStep_v6_4_0());
        QVERIFY(!global_DBObjects.getDb().tables().contains("project_risks"));
        QVERIFY(q.exec("DROP TABLE review_extension_data"));
        for (const QString &key : retired + retained) {
            q.prepare("DELETE FROM application_settings WHERE parameter_name=?");
            q.addBindValue(key);
            QVERIFY(q.exec());
        }
    }

    void test_upgrade640RollsBackOnFailure()
    {
        QSqlQuery q(global_DBObjects.getDb());
        QVERIFY(global_DBObjects.saveParameter("PageState:rollback-test:record", "legacy"));
        QVERIFY(q.exec("UPDATE application_version SET current_version='6.3.0'"));
        QVERIFY(q.exec("CREATE TRIGGER review_cleanup_failure BEFORE UPDATE ON application_settings "
                       "WHEN OLD.parameter_name='PageState:rollback-test:record' "
                       "BEGIN SELECT RAISE(ABORT, 'cleanup failed'); END"));
        auto cleanup = qScopeGuard([&] {
            QSqlQuery restore(global_DBObjects.getDb());
            restore.exec("DROP TRIGGER IF EXISTS review_cleanup_failure");
            restore.exec("DELETE FROM application_settings WHERE parameter_name='PageState:rollback-test:record'");
            restore.exec("UPDATE application_version SET current_version='6.5.0'");
        });
        DatabaseStructure structure;
        QVERIFY(!structure.UpgradeDatabase());
        QCOMPARE(global_DBObjects.loadParameter("PageState:rollback-test:record"), QString("legacy"));
        QVERIFY(q.exec("SELECT current_version FROM application_version"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QString("6.3.0"));
        q.finish();
        QVERIFY(db_rwlock.tryLockForWrite());
        db_rwlock.unlock();
    }

    void test_upgrade650MovesProjectFoldersIntoTables()
    {
        QSqlQuery q(global_DBObjects.getDb());
        const QString tag = uniq("fold");
        const QStringList projects{tag + "-p1", tag + "-p2", tag + "-p3", tag + "-p4"};
        for (const QString& id : projects) {
            q.prepare("INSERT INTO projects(id, project_number, project_name, deleted) VALUES(?, ?, ?, 0)");
            q.addBindValue(id);
            q.addBindValue(id);
            q.addBindValue("Folder migration " + id);
            QVERIFY(q.exec());
        }
        const QString fa = tag + "-fa", fb = tag + "-fb", fc = tag + "-fc", gone = tag + "-gone";
        // A folder deleted since the old document was written.
        q.prepare("INSERT INTO project_folders(id, folder_name, deleted) VALUES(?, ?, 1)");
        q.addBindValue(gone);
        q.addBindValue(tag + " Old");
        QVERIFY(q.exec());
        auto cleanup = qScopeGuard([&] {
            QSqlQuery r(global_DBObjects.getDb());
            r.exec("DELETE FROM projects WHERE id LIKE '" + tag + "%'");
            r.exec("DELETE FROM project_folders WHERE id LIKE '" + tag + "%'");
            r.exec("DELETE FROM application_settings WHERE parameter_name='project_folders'");
        });

        auto folder = [&](const QString& id, const QString& name, const QString& icon, const QString& color) {
            return QJsonObject{{"id", id}, {"name", tag + " " + name}, {"icon", icon}, {"color", color}};
        };
        const QJsonObject document{
            {"folders", QJsonArray{folder(fa, "Favorites", "star", "#111111"), folder(fb, "Clients", "work", "#222222"),
                                   folder(fc, "favorites", "bolt", "#333333"), folder(gone, "Old", "folder", "#444444")}},
            {"memberships", QJsonObject{{projects[0], QJsonArray{fb, fa}}, {projects[1], QJsonArray{gone, fa}},
                                        {projects[2], QJsonArray{gone}}, {projects[3], QJsonArray{fc}}}},
            {"allCollapsed", true}};
        const QString json = QString::fromUtf8(QJsonDocument(document).toJson(QJsonDocument::Compact));
        QVERIFY(global_DBObjects.saveParameter("project_folders", json));

        DatabaseStructure structure;
        QVERIFY(structure.UpgradeDatabase());

        auto folderOf = [&](const QString& projectId) {
            QSqlQuery f(global_DBObjects.getDb());
            f.prepare("SELECT folder_id FROM projects WHERE id = ?");
            f.addBindValue(projectId);
            return f.exec() && f.next() ? f.value(0) : QVariant();
        };
        // One folder per project: the first one it listed that still exists.
        QCOMPARE(folderOf(projects[0]).toString(), fb);
        QCOMPARE(folderOf(projects[1]).toString(), fa);
        QVERIFY(folderOf(projects[2]).isNull());
        // A duplicate name merges into the folder that already has it.
        QCOMPARE(folderOf(projects[3]).toString(), fa);

        QVERIFY(q.exec("SELECT id, folder_name, icon, color, sort_order, deleted, updateddate FROM project_folders "
                       "WHERE id LIKE '" + tag + "%' ORDER BY id"));
        QMap<QString, QVariantList> rows;
        while (q.next())
            rows.insert(q.value(0).toString(), {q.value(1), q.value(2), q.value(3), q.value(4), q.value(5), q.value(6)});
        q.finish();
        QCOMPARE(rows.size(), 3);
        QVERIFY(!rows.contains(fc));
        QCOMPARE(rows.value(fa).at(0).toString(), tag + " Favorites");
        QCOMPARE(rows.value(fa).at(1).toString(), QString("star"));
        QCOMPARE(rows.value(fa).at(2).toString(), QString("#111111"));
        QCOMPARE(rows.value(fa).at(3).toInt(), 0);
        QCOMPARE(rows.value(fb).at(3).toInt(), 1);
        QVERIFY(!rows.value(fa).at(5).isNull());
        QCOMPARE(rows.value(gone).at(4).toInt(), 1);   // stays deleted

        // The setting is retired as a synced soft delete.
        QVERIFY(global_DBObjects.loadParameter("project_folders").isEmpty());
        QVERIFY(q.exec("SELECT COUNT(*) FROM application_settings WHERE parameter_name='project_folders' AND deleted=1"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
        q.finish();

        // Running again, or an old client pushing the document back, changes nothing.
        QVERIFY(structure.UpgradeDatabase());
        QVERIFY(global_DBObjects.saveParameter("project_folders", json));
        QVERIFY(structure.UpgradeDatabase());
        QCOMPARE(folderOf(projects[0]).toString(), fb);
        QVERIFY(folderOf(projects[2]).isNull());
        QVERIFY(q.exec("SELECT COUNT(*) FROM project_folders WHERE id LIKE '" + tag + "%' AND deleted = 0"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 2);
        q.finish();
        QVERIFY(global_DBObjects.loadParameter("project_folders").isEmpty());
    }

    // ── Static option lists back the ComboBoxes on every form ─────────────────
    void test_00_optionLists()
    {
        QVERIFY(!c->projectStatusOptions().isEmpty());
        QVERIFY(!c->invoicingPeriodOptions().isEmpty());
        QVERIFY(!c->statusReportPeriodOptions().isEmpty());
        QVERIFY(!c->itemTypeOptions().isEmpty());
        QVERIFY(!c->itemPriorityOptions().isEmpty());
        QVERIFY(!c->itemStatusOptions().isEmpty());
        QVERIFY(!c->fileTypeOptions().isEmpty());
        QVERIFY(!c->statusItemCategoryOptions().isEmpty());
        QCOMPARE(c->meetingTemplateChoices().size(), 12);
        QCOMPARE(c->meetingTemplateEditorModel()->property("templates").toList().size(), 12);
    }

    void test_01_clientCrud()
    {
        const int before = c->companiesModel()->rowCount();
        const int row = c->addCompany();
        QVERIFY(row >= 0);

        const QString name = uniq("Client_");
        QVERIFY2(c->saveCompany(row, name), qPrintable(c->lastSaveError()));
        QCOMPARE(c->companiesModel()->rowCount(), before + 1);

        const QString id = idForColumnValue(c->companiesModel(), 1, name);  // col 1 = client_name
        QVERIFY(!id.isEmpty());
        QCOMPARE(c->companyNameForId(id), name);

        m_clientId = id;
    }

    void test_02_personCrud()
    {
        const int before = c->contactsModel()->rowCount();
        const int row = c->addContact();
        QVERIFY(row >= 0);

        const QString name = uniq("Person_");
        QVERIFY2(c->saveContact(row, name, "p@example.com", "555-1000", "555-2000",
                               m_clientId, "Engineer"),
                 qPrintable(c->lastSaveError()));
        QCOMPARE(c->contactsModel()->rowCount(), before + 1);

        const QString id = idForColumnValue(c->contactsModel(), 1, name);  // col 1 = name
        QVERIFY(!id.isEmpty());
        QCOMPARE(c->contactNameForId(id), name);

        m_personId = id;
    }

    void test_03_projectCrud()
    {
        const int before = c->projectsListModel()->rowCount();
        const int row = c->addProject();
        QVERIFY(row >= 0);
        // addProject() only stages the row in the model cache; the record — and
        // with it the id — is written by the first save, once the NOT NULL
        // number/name are filled in. Same contract as addCompany()/addContact().
        QVERIFY2(c->projectIdAtRow(row).isEmpty(), "a staged project must not have an id yet");

        const QString num  = uniq("PN");
        const QString name = uniq("Project_");
        QVERIFY2(c->saveProject(row, num, name,
                                c->projectStatusOptions().first(),
                                m_personId, m_clientId, "07/01/2026", "07/15/2026",
                                c->invoicingPeriodOptions().first(),
                                c->statusReportPeriodOptions().first(),
                                "10000", "4200", "3900", "4100", "10000"),
                 qPrintable(c->lastSaveError()));
        // The page that typed the fields adopts the new id from here (the insert
        // can move the row within the sorted list) — see _adoptSavedRecord().
        const QString id = c->lastCreatedProjectId();
        QVERIFY(!id.isEmpty());
        QCOMPARE(c->projectsListModel()->rowCount(), before + 1);
        QCOMPARE(c->projectNameForId(id), name);
        QCOMPARE(c->projectNumberForId(id), num);

        const int r = c->projectRowForId(id);
        QVERIFY(r >= 0);
        const QVariantMap data = c->getProjectData(r);
        QVERIFY(!data.isEmpty());
        QVERIFY(mapHasValue(data, name));

        m_projectId = id; m_projectName = name; m_projectNumber = num;
    }

    void test_03b_projectFolders()
    {
        FolderManager folders;
        folders.reload();
        const QString tag = uniq("Folder ");
        const QString a = folders.addFolder(tag + " A", "star", "#111111");
        const QString b = folders.addFolder(tag + " B");
        QVERIFY(!a.isEmpty());
        QVERIFY(!b.isEmpty());
        auto cleanup = qScopeGuard([&] {
            folders.removeFolder(a);
            folders.removeFolder(b);
        });

        // Names are required and unique, ignoring case.
        QVERIFY(folders.addFolder("  ").isEmpty());
        QVERIFY(folders.addFolder(tag.toLower() + " a").isEmpty());
        QVERIFY(!folders.folderNameError(tag + " B", a).isEmpty());
        QVERIFY(folders.folderNameError(tag + " A", a).isEmpty());
        QVERIFY(!folders.renameFolder(a, tag + " b"));
        QVERIFY(folders.renameFolder(a, tag + " Renamed"));
        QCOMPARE(folders.folderName(a), tag + " Renamed");
        QVERIFY(folders.setFolderIcon(a, "bolt"));
        QVERIFY(folders.setFolderColor(a, "#abcdef"));
        QVERIFY(folders.moveFolder(b, 0));
        QSqlQuery q(global_DBObjects.getDb());
        QVERIFY(q.exec("SELECT folder_name, icon, color FROM project_folders WHERE id = '" + a + "'"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), tag + " Renamed");
        QCOMPARE(q.value(1).toString(), QString("bolt"));
        QCOMPARE(q.value(2).toString(), QString("#abcdef"));
        q.finish();
        int indexA = -1, indexB = -1;
        const QVariantList list = folders.folders();
        for (int i = 0; i < list.size(); ++i) {
            if (list.at(i).toMap().value("id") == a) indexA = i;
            if (list.at(i).toMap().value("id") == b) indexB = i;
        }
        QVERIFY(indexB >= 0 && indexB < indexA);

        // Collapse state stays out of the database.
        QVERIFY(folders.setFolderCollapsed(a, true));
        QVERIFY(q.exec("SELECT COUNT(*) FROM application_settings WHERE parameter_name LIKE '%" + a + "%'"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 0);
        q.finish();
        QVERIFY(folders.setFolderCollapsed(a, false));

        // One folder per project; a deleted folder releases its projects.
        QVERIFY(c->addProject() >= 0);
        QAbstractItemModel* projects = c->projectsListModel();
        QString projectId;
        for (int r = 0; r < projects->rowCount() && projectId.isEmpty(); ++r)
            projectId = projects->data(projects->index(r, 0)).toString();
        QVERIFY(!projectId.isEmpty());
        QVERIFY(folders.setProjectFolder(projectId, a));
        QCOMPARE(folders.folderForProject(projectId), a);
        QVERIFY(folders.setProjectFolder(projectId, b));
        QCOMPARE(folders.folderForProject(projectId), b);
        QVERIFY(!folders.setProjectFolder(projectId, "no-such-folder"));
        QCOMPARE(folders.folders().at(0).toMap().value("count").toInt(), 1);

        // The Folder filter lists every folder plus Not Categorized.
        const QVariantList values = c->columnDistinctValues(projects, "folder_id");
        QVERIFY(values.size() >= 3);
        QCOMPARE(values.first().toMap().value("value").toString(), QString());
        QStringList valueIds;
        for (const QVariant& v : values) valueIds << v.toMap().value("value").toString();
        QVERIFY(valueIds.contains(a));
        QVERIFY(valueIds.contains(b));

        // Filtering the list to a folder keeps every sidebar group whole.
        const int allBefore = c->folderProjects("").size();
        c->filterProjectsByFolder(b);
        QCOMPARE(c->projectFolderFilter(), QStringList{b});
        QCOMPARE(projects->rowCount(), 1);
        QCOMPARE(projects->data(projects->index(0, 0)).toString(), projectId);
        QCOMPARE(c->folderProjects("").size(), allBefore);
        QCOMPARE(c->folderProjects(b).size(), 1);
        c->filterProjectsByFolder("");
        QCOMPARE(c->projectFolderFilter(), QStringList{QString()});
        for (int r = 0; r < projects->rowCount(); ++r)
            QVERIFY(projects->data(projects->index(r, 0)).toString() != projectId);
        QCOMPARE(c->folderProjects("").size(), allBefore);
        c->filterProjectsByFolder("");   // the same folder again toggles it off
        QVERIFY(c->projectFolderFilter().isEmpty());
        QVERIFY(!c->hasActiveColumnFilters(projects));

        QVERIFY(folders.removeFolder(b));
        QVERIFY(folders.folderForProject(projectId).isEmpty());
        QVERIFY(q.exec("SELECT folder_id FROM projects WHERE id = '" + projectId + "'"));
        QVERIFY(q.next());
        QVERIFY(q.value(0).isNull());
        q.finish();
        QVERIFY(q.exec("SELECT deleted FROM project_folders WHERE id = '" + b + "'"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
        q.finish();
    }

    void test_03c_primaryProjectFolder()
    {
        const QString projectId = m_projectId;
        QVERIFY(!projectId.isEmpty());
        const QString tag = uniq("loc");
        const QString main = tag + "-main", other = tag + "-other", link = tag + "-link";
        QSqlQuery q(global_DBObjects.getDb());
        const QList<QStringList> rows{{main, "File Folder", "Project Folder", "/projects/main"},
                                      {other, "File Folder", "Contracts", "/projects/contracts"},
                                      {link, "Web Link", "Project Folder", "https://example.test/team"}};
        for (const QStringList& row : rows) {
            q.prepare("INSERT INTO project_locations(id, project_id, location_type, location_description, full_path, deleted) "
                      "VALUES(?, ?, ?, ?, ?, 0)");
            q.addBindValue(row[0]);
            q.addBindValue(projectId);
            q.addBindValue(row[1]);
            q.addBindValue(row[2]);
            q.addBindValue(row[3]);
            QVERIFY(q.exec());
        }
        auto cleanup = qScopeGuard([&] {
            QSqlQuery r(global_DBObjects.getDb());
            r.exec("DELETE FROM project_locations WHERE id LIKE '" + tag + "%'");
            r.exec("UPDATE projects SET primary_location_id = NULL, primary_location_manual = 0 WHERE id = '" + projectId + "'");
        });
        auto manualFlag = [&]() {
            QSqlQuery f(global_DBObjects.getDb());
            f.prepare("SELECT primary_location_manual FROM projects WHERE id = ?");
            f.addBindValue(projectId);
            return f.exec() && f.next() ? f.value(0).toInt() : -1;
        };

        // The upgrade that adds the columns backfills the "Project Folder" row once.
        for (const char* sql : {"DROP TRIGGER IF EXISTS trg_project_locations_primary_deleted",
                                "DROP TRIGGER IF EXISTS trg_project_locations_primary_removed",
                                "ALTER TABLE projects DROP COLUMN primary_location_manual",
                                "ALTER TABLE projects DROP COLUMN primary_location_id"})
            QVERIFY2(q.exec(sql), qPrintable(q.lastError().text()));
        DatabaseStructure structure;
        QVERIFY(structure.UpgradeDatabase());
        global_DBObjects.projectinformationmodel()->refresh();
        QCOMPARE(c->primaryProjectLocationId(projectId), main);
        QVERIFY(!c->projectFolderChosenByUser(projectId));

        // Only this project's live folder locations can be chosen.
        QVERIFY(!c->setPrimaryProjectLocation(projectId, link));
        QVERIFY(!c->setPrimaryProjectLocation(projectId, "no-such-location"));
        QSignalSpy revs(c, &DesktopAppController::projectFolderRevChanged);
        QVERIFY(c->setPrimaryProjectLocation(projectId, other));
        QVERIFY(revs.count() >= 1);
        QCOMPARE(c->primaryProjectLocationId(projectId), other);
        QVERIFY(c->projectFolderChosenByUser(projectId));

        // Clearing leaves the project without a folder, and it stays that way.
        QVERIFY(c->setPrimaryProjectLocation(projectId, ""));
        QVERIFY(c->primaryProjectLocationId(projectId).isEmpty());
        QCOMPARE(manualFlag(), 1);
        QVERIFY(structure.UpgradeDatabase());
        QVERIFY(c->primaryProjectLocationId(projectId).isEmpty());

        QVERIFY(c->letFileFinderChooseProjectFolder(projectId));
        QCOMPARE(manualFlag(), 0);
        QVERIFY(c->primaryProjectLocationId(projectId).isEmpty());

        // Deleting the primary location clears it but keeps the user's flag.
        QVERIFY(c->setPrimaryProjectLocation(projectId, main));
        QVERIFY(q.exec("UPDATE project_locations SET deleted = 1 WHERE id = '" + main + "'"));
        QVERIFY(q.exec("SELECT primary_location_id, primary_location_manual FROM projects WHERE id = '" + projectId + "'"));
        QVERIFY(q.next());
        QVERIFY(q.value(0).isNull());
        QCOMPARE(q.value(1).toInt(), 1);
        q.finish();
        QVERIFY(c->setPrimaryProjectLocation(projectId, other));
        QVERIFY(q.exec("DELETE FROM project_locations WHERE id = '" + other + "'"));
        QVERIFY(c->primaryProjectLocationId(projectId).isEmpty());
    }

    void test_04_pickerLists()
    {
        // Client / person pickers used by the project + person forms.
        QVERIFY(c->companyList().size()  >= 1);
        QVERIFY(c->contactList().size()  >= 1);
        // Team-member picker must include an explicitly-included id even before
        // anyone is added to the team.
        const QVariantList tm = c->teamMemberList(m_projectId, { m_personId });
        bool found = false;
        for (const QVariant& v : tm)
            if (v.toMap().value("id").toString() == m_personId) found = true;
        QVERIFY(found);
    }

    void test_05_projectNotesCrud()
    {
        c->setProjectFilter(m_projectId);
        const int before = c->projectNotesModel()->rowCount();
        const int row = c->addProjectNote(m_projectId);
        QVERIFY(row >= 0);
        const QString noteId = c->projectNoteIdAtRow(row);
        QVERIFY(!noteId.isEmpty());

        const QString title = uniq("Note_");
        QVERIFY2(c->saveProjectNote(row, title, kDate1,
                                    "Meeting minutes body.", false),
                 qPrintable(c->lastSaveError()));
        QCOMPARE(c->projectNotesModel()->rowCount(), before + 1);

        const int r = rowForId(c->projectNotesModel(), noteId);
        QVERIFY(r >= 0);
        QVERIFY(mapHasValue(c->getProjectNoteData(r), title));

        m_noteId = noteId;
    }

    void test_06_meetingAttendees()
    {
        c->setNoteFilter(m_noteId);
        const int before = c->meetingAttendeesModel()->rowCount();
        const int row = c->addAttendee(m_noteId);
        QVERIFY(row >= 0);
        QVERIFY2(c->saveAttendee(row, m_personId), qPrintable(c->lastSaveError()));
        QCOMPARE(c->meetingAttendeesModel()->rowCount(), before + 1);

        // Duplicate attendee must be rejected (same person on the same note).
        const int dupRow = c->addAttendee(m_noteId);
        QVERIFY(dupRow >= 0);
        QVERIFY(!c->saveAttendee(dupRow, m_personId));
        QVERIFY(!c->lastSaveError().isEmpty());
    }

    void test_07_noteActionItems()
    {
        c->setNoteFilter(m_noteId);
        const int before = c->notesActionItemsModel()->rowCount();
        const int row = c->addNoteActionItem(m_noteId, m_projectId);
        QVERIFY(row >= 0);

        const QString name = uniq("AI_");
        QVERIFY2(c->saveNoteActionItem(row, name,
                    c->itemTypeOptions().first(),
                    c->itemPriorityOptions().first(),
                    c->itemStatusOptions().first(),
                    m_personId, m_personId, kDate1, kDate2,
                    "Follow up on the deliverable.", kDate1, QString()),
                 qPrintable(c->lastSaveError()));
        QCOMPARE(c->notesActionItemsModel()->rowCount(), before + 1);
    }

    void test_08_trackerItemCrudAndUniqueness()
    {
        const int addedRow = c->addTrackerItem(m_projectId);  // detail filtered → row 0
        QCOMPARE(addedRow, 0);
        auto* detail = c->trackerItemDetailModel();
        QVERIFY(detail->rowCount() >= 1);
        const QString itemId = detail->data(detail->index(0, 0)).toString();
        QVERIFY(!itemId.isEmpty());

        const QString name = uniq("Risk_");
        const QString number = uniq("R");
        QVERIFY2(c->saveTrackerItemDetail(0, itemId, number,
                    c->itemTypeOptions().first(), name,
                    "A tracked risk.", m_personId, m_personId,
                    c->itemPriorityOptions().first(),
                    c->itemStatusOptions().first(),
                    kDate1, "08/31/2026", false, QString(), QString()),
                 qPrintable(c->lastSaveError()));

        // Uniqueness helpers used by the QML form validators.
        QVERIFY(!c->isItemNameUnique(m_projectId, "", name));         // taken by another item
        QVERIFY( c->isItemNameUnique(m_projectId, itemId, name));     // same item excluded
        QVERIFY(!c->isItemNumberUnique(m_projectId, "", number));
        QVERIFY( c->isItemNumberUnique(m_projectId, itemId, number));

        // The item must appear in the cross-project "all items" list.
        c->refreshAllItems();
        QVERIFY(rowForId(c->allItemsModel(), itemId) >= 0);

        m_itemId = itemId;
    }

    void test_09_trackerComments()
    {
        c->openTrackerItem(m_itemId);
        const int before = c->trackerItemCommentsModel()->rowCount();
        const int row = c->addComment(m_itemId);
        QVERIFY(row >= 0);
        QVERIFY2(c->saveComment(row, kDate1, "Progress update.", m_personId),
                 qPrintable(c->lastSaveError()));
        QCOMPARE(c->trackerItemCommentsModel()->rowCount(), before + 1);
    }

    void test_10_teamMembers()
    {
        // A fresh person, so the duplicate-team-member guard doesn't trip on the
        // project's default PM.
        const int prow = c->addContact();
        QVERIFY(prow >= 0);
        const QString memberName = uniq("Member_");
        QVERIFY2(c->saveContact(prow, memberName, "m@example.com", "", "",
                               m_clientId, "Analyst"),
                 qPrintable(c->lastSaveError()));
        const QString memberId = idForColumnValue(c->contactsModel(), 1, memberName);
        QVERIFY(!memberId.isEmpty());

        c->setProjectFilter(m_projectId);
        const int before = c->projectTeamMembersModel()->rowCount();
        const int row = c->addTeamMember(m_projectId);
        QVERIFY(row >= 0);
        QVERIFY2(c->saveTeamMember(row, memberId, "Analyst", true),
                 qPrintable(c->lastSaveError()));
        QCOMPARE(c->projectTeamMembersModel()->rowCount(), before + 1);
    }

    void test_10b_sendNotesLoadsRecipientsWithoutEmailBackend()
    {
        const QString previousManager = c->projectManagerId();
        c->setProjectManagerId(QString());
        QVERIFY(c->prepareMeetingNotesReview(m_projectId, m_noteId));
        QObject *review = c->communicationsController();
        QTRY_VERIFY_WITH_TIMEOUT(review->property("stageName").toString() != QStringLiteral("editing"), 5000);
        QVERIFY2(review->property("stageName").toString() == QStringLiteral("reviewing"),
                 qPrintable(review->property("diagnostic").toString()));
        auto *recipients = qobject_cast<PN::Comm::RecipientSelectionModel *>(c->recipientSelectionModel());
        QVERIFY(recipients);
        QVERIFY(recipients->rowCount() >= 2);
        QCOMPARE(recipients->selectedRecipientCount(), 1);
        QCOMPARE(recipients->toRecipientCount(), 1);
        QCOMPARE(recipients->ccRecipientCount(), 0);
        c->setProjectManagerId(previousManager);
    }

    void test_11_projectLocations()
    {
        c->setProjectFilter(m_projectId);
        const int before = c->projectLocationsModel()->rowCount();
        const int row = c->addProjectLocation(m_projectId);
        QVERIFY(row >= 0);
        QVERIFY2(c->saveProjectLocation(row, c->fileTypeOptions().first(),
                                        "Spec document", "/tmp/spec.txt"),
                 qPrintable(c->lastSaveError()));
        QCOMPARE(c->projectLocationsModel()->rowCount(), before + 1);
    }

    void test_11b_projectEmailAndLocationSharing()
    {
        auto *review = qobject_cast<PN::Comm::CommunicationsController *>(c->communicationsController());
        auto *recipients = qobject_cast<PN::Comm::RecipientSelectionModel *>(c->recipientSelectionModel());
        auto *templates = qobject_cast<PN::Comm::TemplateEditorModel *>(c->templateEditorModel());
        QVERIFY(review); QVERIFY(recipients); QVERIFY(templates);
        templates->setWorkflow("project-email");
        templates->setDraftSubject("Hello {{ contact.name }} - {{ project.number }}");
        templates->setDraftRichBody("<p>To {{ contact.email }} about {{ project.name }}</p>");
        QVERIFY(templates->save());
        QVERIFY(c->prepareProjectEmailReview(m_projectId, m_personId));
        QTRY_COMPARE_WITH_TIMEOUT(review->stageName(), "reviewing", 5000);
        QCOMPARE(recipients->selectedRecipientCount(), 1);
        QCOMPARE(review->preparation().recipients.first().address, "p@example.com");
        QVERIFY(review->subject().contains(m_projectNumber));
        QVERIFY(review->htmlBody().contains("p@example.com"));
        QVERIFY(recipients->rowCount() > 1);
        QVERIFY(c->saveReviewAudiencePreset("Project person audience"));
        QString projectPresetId;
        for (const auto &preset : c->reviewAudiencePresets())
            if (preset.toMap().value("name").toString() == "Project person audience")
                projectPresetId = preset.toMap().value("id").toString();
        QVERIFY(!projectPresetId.isEmpty());
        QVERIFY(c->setReviewAudiencePresetDefault(projectPresetId));
        recipients->selectAll();
        QVERIFY(recipients->selectedRecipientCount() > 1);
        QVERIFY(c->applyReviewAudiencePreset(projectPresetId));
        QCOMPARE(recipients->selectedRecipientCount(), 1);
        QCOMPARE(recipients->resolve().recipients.first().address, "p@example.com");
        // A same-workflow template edit must be used on the next person draft.
        templates->setDraftSubject("Follow up {{ project.number }}");
        templates->setDraftRichBody("<p>Configured body for {{ contact.email }}</p>");
        QVERIFY(templates->save());
        QVERIFY(c->prepareProjectEmailReview(m_projectId, m_personId));
        QTRY_COMPARE_WITH_TIMEOUT(review->stageName(), "reviewing", 5000);
        QCOMPARE(review->subject(), "Follow up " + m_projectNumber);
        QCOMPARE(review->htmlBody(), "<p>Configured body for p@example.com</p>");
        QCOMPARE(recipients->selectedRecipientCount(), 1);
        QVERIFY(templates->resetSelected());
        QVERIFY(c->prepareProjectEmailReview(m_projectId));
        QTRY_COMPARE_WITH_TIMEOUT(review->stageName(), "reviewing", 5000);
        // Team drafts load their project-email default; saved meeting guests
        // must remain recipients even without a clicked-person source.
        QCOMPARE(recipients->selectedRecipientCount(), 1);
        QCOMPARE(recipients->resolve().recipients.first().address, "p@example.com");
        QVERIFY(c->applyReviewAudiencePreset(projectPresetId));
        QVERIFY(c->prepareProjectEmailReview(m_projectId, m_personId));
        QTRY_COMPARE_WITH_TIMEOUT(review->stageName(), "reviewing", 5000);
        QVERIFY(review->subject().contains(m_projectNumber));
        QVERIFY(!review->subject().contains("Follow up"));
        QTemporaryDir directory;
        QFile original(directory.filePath("attachment.txt"));
        QVERIFY(original.open(QIODevice::WriteOnly)); original.write("original bytes"); original.close();
        c->setProjectFilter(m_projectId);
        const int row = c->addProjectLocation(m_projectId);
        QVERIFY(c->saveProjectLocation(row, c->fileTypeOptions().first(), "Shared attachment", original.fileName()));
        const QString locationId = idForColumnValue(c->projectLocationsModel(), 3, "Shared attachment");
        QVERIFY(!locationId.isEmpty());
        QVERIFY(c->prepareLocationEmailReview(m_projectId, locationId, true));
        QTRY_COMPARE_WITH_TIMEOUT(review->stageName(), "reviewing", 5000);
        QCOMPARE(review->preparation().attachments.size(), 1);
        const auto artifact = review->preparation().attachments.first();
        QVERIFY(artifact.absolutePath != original.fileName());
        QVERIFY(original.open(QIODevice::WriteOnly)); original.write("changed bytes"); original.close();
        QFile cached(artifact.absolutePath); QVERIFY(cached.open(QIODevice::ReadOnly));
        QCOMPARE(cached.readAll(), QByteArray("original bytes")); cached.close();
        QVERIFY(c->saveReviewAudiencePreset("Location audience"));
        const auto presets = c->reviewAudiencePresets(); QVERIFY(!presets.isEmpty());
        const QString presetId = presets.first().toMap().value("id").toString();
        QVERIFY(c->setReviewAudiencePresetDefault(presetId));
        QVERIFY(c->prepareLocationEmailReview(m_projectId, locationId, true));
        QTRY_COMPARE_WITH_TIMEOUT(review->stageName(), "reviewing", 5000);
        QVERIFY(c->applyReviewAudiencePreset(presetId));
        // Changing the saved location after preview must block the handoff,
        // before any mail application or provider is invoked.
        const int savedRow = rowForId(c->projectLocationsModel(), locationId);
        QVERIFY(c->saveProjectLocation(savedRow, c->fileTypeOptions().first(), "Changed attachment", original.fileName()));
        QVERIFY(c->handoffPreparedReview());
        QTRY_COMPARE_WITH_TIMEOUT(review->stageName(), "failed", 5000);
        QVERIFY(review->diagnostic().contains("changed"));
    }

    void test_11c_standalonePersonEmailHasNoProjectTemplate()
    {
        const int row = c->addContact();
        QVERIFY(row >= 0);
        QVERIFY(c->saveContact(row, "Standalone email person", "standalone@example.test", "", "", m_clientId, ""));
        const QString contactId = idForColumnValue(c->contactsModel(), 1, "Standalone email person");
        QVERIFY(!contactId.isEmpty());
        QVERIFY(c->prepareContactEmailReview(contactId));
        auto *review = qobject_cast<PN::Comm::CommunicationsController *>(c->communicationsController());
        auto *recipients = qobject_cast<PN::Comm::RecipientSelectionModel *>(c->recipientSelectionModel());
        QTRY_COMPARE_WITH_TIMEOUT(review->stageName(), "reviewing", 5000);
        QCOMPARE(review->preparation().source.workflow, PN::Comm::Workflow::ContactEmail);
        QVERIFY(review->preparation().source.projectId.isEmpty());
        QVERIFY(review->subject().isEmpty());
        QVERIFY(review->htmlBody().isEmpty());
        QCOMPARE(recipients->selectedRecipientCount(), 1);
        QCOMPARE(review->preparation().recipients.first().address, "standalone@example.test");
        QVERIFY(!review->setSubject("A plain email"));
        QVERIFY(!review->setHtmlBody("<p>New message</p>"));
        QVERIFY(review->subject().isEmpty());
        QVERIFY(review->htmlBody().isEmpty());
        QVERIFY(PN::Comm::EmailContentBuilder::prepare(review->preparation()).request.has_value());
        QVERIFY(!c->saveReviewAudiencePreset("Not a project audience"));
        QVERIFY(!c->prepareProjectEmailReview({}));
        QVERIFY(!c->prepareContactEmailReview({}));
    }

    void test_11d_contactCardExportAndEmail()
    {
        const int row = c->addContact();
        QVERIFY(row >= 0);
        QVERIFY(c->saveContact(row, "Card; Person", "card@example.test", "555-0100", "555-0199", m_clientId, "Lead, QA"));
        const QString contactId = idForColumnValue(c->contactsModel(), 1, "Card; Person");
        QVERIFY(!contactId.isEmpty());
        QCOMPARE(c->contactVCardFileName(contactId), "Card; Person.vcf");
        QVERIFY(c->contactVCardFileName("missing").isEmpty());

        QTemporaryDir dir;
        const QString path = dir.filePath("card.vcf");
        QVERIFY(c->exportContactVCard(contactId, QUrl::fromLocalFile(path).toString()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray card = file.readAll();
        QVERIFY(card.startsWith("BEGIN:VCARD\r\nVERSION:3.0\r\n"));
        QVERIFY(card.contains("FN:Card\\; Person\r\n"));
        QVERIFY(card.contains("EMAIL;TYPE=WORK:card@example.test\r\n"));
        QVERIFY2(card.contains("TEL;TYPE=WORK:5550100\r\n"), card.constData());
        QVERIFY(card.contains("TEL;TYPE=CELL:5550199\r\n"));
        QVERIFY(card.contains("TITLE:Lead\\, QA\r\n"));
        QVERIFY(card.contains("ORG:"));
        QVERIFY(card.endsWith("END:VCARD\r\n"));
        QVERIFY(!c->exportContactVCard("missing", path + ".2"));

        // Email Contact skips the review dialog: the draft goes straight to
        // the email integration with the card attached and no recipients.
        QSignalSpy errors(c, &DesktopAppController::errorOccurred);
        QVERIFY(c->emailContactCard(contactId));
        auto *review = qobject_cast<PN::Comm::CommunicationsController *>(c->communicationsController());
        QTRY_VERIFY_WITH_TIMEOUT(review->stageName() == "completed" || review->stageName() == "failed", 5000);
        QCOMPARE(review->preparation().source.workflow, PN::Comm::Workflow::ContactEmail);
        QVERIFY(review->preparation().recipients.isEmpty());
        QVERIFY(review->preparation().addressLater);
        QCOMPARE(review->subject(), "Contact: Card; Person");
        QCOMPARE(review->attachmentNames(), QStringList{"Card; Person.vcf"});
        QCOMPARE(review->preparation().attachments.first().mimeType, "text/vcard");
        // The handoff reached the backend. Offscreen test runs can't open the
        // .eml in a mail app, which is the only acceptable failure here.
        if (review->stageName() == "failed") {
            QCOMPARE(errors.count(), 1);
            QVERIFY2(review->lastResult().certainty != PN::Comm::OutcomeCertainty::NotStarted
                         || review->lastResult().error.code.contains("open"),
                     qPrintable(review->diagnostic()));
        } else {
            QCOMPARE(errors.count(), 0);
        }
        QVERIFY(!c->emailContactCard({}));
    }

    void test_11e_vCardDragAndDropImport()
    {
        // An exported card dropped on the Contacts list creates the contact
        // with every field the card carries.
        const int row = c->addContact();
        QVERIFY(row >= 0);
        QVERIFY(c->saveContact(row, "Drop Source", "drop@example.test", "5550111", "5550122", m_clientId, "Engineer"));
        const QString sourceId = idForColumnValue(c->contactsModel(), 1, "Drop Source");
        QTemporaryDir dir;
        const QString path = dir.filePath("Drop Source.vcf");
        QVERIFY(c->exportContactVCard(sourceId, QUrl::fromLocalFile(path).toString()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QString card = QString::fromUtf8(file.readAll());
        card.replace("Drop Source", "Dropped, Person");
        card.replace("drop@example.test", "dropped@example.test");
        QCOMPARE(c->addContactsFromVCardDrop({}, card), 1);
        QSqlQuery q(global_DBObjects.getDb());
        q.prepare("SELECT id, email, office_phone, cell_phone, client_id, role FROM people WHERE name = ? AND deleted = 0");
        q.addBindValue("Dropped, Person");
        QVERIFY(q.exec() && q.next());
        const QString droppedId = q.value(0).toString();
        QCOMPARE(q.value(1).toString(), "dropped@example.test");
        QCOMPARE(q.value(2).toString(), "5550111");
        QCOMPARE(q.value(3).toString(), "5550122");
        QCOMPARE(q.value(4).toString(), m_clientId);
        QCOMPARE(q.value(5).toString(), "Engineer");

        // The exported file dropped on a project's Team list adds the
        // existing contact to the team instead of creating a duplicate.
        QCOMPARE(c->addTeamMembersFromVCardDrop(m_projectId, {QUrl::fromLocalFile(path).toString()}, {}), 1);
        q.prepare("SELECT COUNT(*) FROM people WHERE name = 'Drop Source' AND deleted = 0");
        QVERIFY(q.exec() && q.next());
        QCOMPARE(q.value(0).toInt(), 1);
        q.prepare("SELECT COUNT(*) FROM project_people WHERE project_id = ? AND people_id = ? AND deleted = 0");
        q.addBindValue(m_projectId);
        q.addBindValue(sourceId);
        QVERIFY(q.exec() && q.next());
        QCOMPARE(q.value(0).toInt(), 1);
        QVERIFY(!droppedId.isEmpty());
    }

    void test_11f_vCardDragOutData()
    {
        const QString contactId = idForColumnValue(c->contactsModel(), 1, "Drop Source");
        QVERIFY(!contactId.isEmpty());
        const QVariantMap data = c->contactVCardDragData(contactId);
        QCOMPARE(data.keys(), (QStringList{"text/uri-list", "text/vcard", "text/x-vcard"}));
        const QUrl url(data.value("text/uri-list").toString());
        QVERIFY(url.isLocalFile());
        QCOMPARE(QFileInfo(url.toLocalFile()).fileName(), "Drop Source.vcf");
        QFile file(url.toLocalFile());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray card = file.readAll();
        file.close();
        QCOMPARE(QString::fromUtf8(card), data.value("text/vcard").toString());
        QCOMPARE(data.value("text/x-vcard"), data.value("text/vcard"));
        // A second drag of the same contact gets its own file, so a drop
        // target still reading the first one is never disturbed.
        const QUrl second(c->contactVCardDragData(contactId).value("text/uri-list").toString());
        QVERIFY(second != url);
        QVERIFY(QFile::exists(url.toLocalFile()));
        QVERIFY(c->contactVCardDragData("missing").isEmpty());
        // Dragged-out cards live in the transient cache, which is cleared on quit.
        QVERIFY(url.toLocalFile().contains("/vcard-drag/"));
    }

    void test_12_statusItems()
    {
        c->setProjectFilter(m_projectId);
        const int before = c->statusReportItemsModel()->rowCount();
        const int row = c->addStatusItem(m_projectId);
        QVERIFY(row >= 0);
        QVERIFY2(c->saveStatusItem(row, c->statusItemCategoryOptions().first(),
                                   "On track for milestone."),
                 qPrintable(c->lastSaveError()));
        QCOMPARE(c->statusReportItemsModel()->rowCount(), before + 1);
    }

    void test_13_globalSearch()
    {
        c->performSearch(m_projectName);
        // The seeded project should surface in the global search results.
        QVERIFY(c->searchResultsModel()->rowCount() >= 1);
    }

    void test_14_quickSearchAndColumnFilters()
    {
        auto* pm = c->projectsListModel();
        const int total = pm->rowCount();
        QVERIFY(total >= 1);

        c->setQuickSearch(pm, m_projectNumber);
        const int narrowed = pm->rowCount();
        QVERIFY(narrowed >= 1);
        QVERIFY(narrowed <= total);

        c->setQuickSearch(pm, "");
        QCOMPARE(pm->rowCount(), total);

        // Column filter editor plumbing (mirrors the Widgets Filter Data dialog).
        const QVariantList cols = c->filterColumns(pm);
        QVERIFY(!cols.isEmpty());
        const QString field = cols.first().toMap().value("field").toString();
        QVERIFY(!field.isEmpty());
        c->columnDistinctValues(pm, field);           // must not crash
        c->applyColumnFilters(pm, {});                // no-op spec
        c->clearColumnFilters(pm);
        c->refreshModel(pm);
        QCOMPARE(pm->rowCount(), total);
    }

    // Quick Filter (stacking/toggle/highlight plumbing), the computed overdue
    // columns it depends on, and the Sort menu plumbing — see DesktopAppController's
    // activeColumnFilters/applyQuickFilter/hasActiveColumnFilters/modelForSection
    // and sortColumns/applySort/clearSort/activeSort.
    void test_14b_quickFilterAndSort()
    {
        auto* pm = c->projectsListModel();
        const int total = pm->rowCount();
        QVERIFY(total >= 1);

        // status_overdue/invoicing_overdue are computed "Yes"/"No" columns
        // added specifically so the Quick Filter's overdue shortcuts can go
        // through the same column-filter pipeline as everything else.
        const QVariantList cols = c->filterColumns(pm);
        bool hasStatusOverdue = false, hasInvoicingOverdue = false;
        for (const QVariant& cv : cols) {
            const QString field = cv.toMap().value("field").toString();
            if (field == "status_overdue")    hasStatusOverdue = true;
            if (field == "invoicing_overdue") hasInvoicingOverdue = true;
        }
        QVERIFY(hasStatusOverdue);
        QVERIFY(hasInvoicingOverdue);

        // modelForSection is the canonical section->model map Filter/Sort/
        // quick search all share.
        QCOMPARE(c->modelForSection("projects"), static_cast<QAbstractItemModel*>(pm));
        QVERIFY(c->modelForSection("not-a-real-section") == nullptr);

        // applyQuickFilter stacks: two different fields both survive one
        // after the other instead of the second replacing the first.
        QVERIFY(!c->hasActiveColumnFilters(pm));
        const int filterRevBefore = c->filterRev();
        c->applyQuickFilter(pm, "client_id", { m_clientId });
        QVERIFY(c->filterRev() > filterRevBefore);
        c->applyQuickFilter(pm, "project_status", { c->projectStatusOptions().first() });
        QVariantList active = c->activeColumnFilters(pm);
        QCOMPARE(active.size(), 2);
        QVERIFY(c->hasActiveColumnFilters(pm));

        // Re-applying the same field/values toggles it off instead of piling
        // up a duplicate — makes a checkmarked Quick Filter row idempotent.
        c->applyQuickFilter(pm, "project_status", { c->projectStatusOptions().first() });
        active = c->activeColumnFilters(pm);
        QCOMPARE(active.size(), 1);
        QCOMPARE(active.first().toMap().value("field").toString(), QStringLiteral("client_id"));

        c->clearColumnFilters(pm);
        QVERIFY(!c->hasActiveColumnFilters(pm));
        c->refreshModel(pm);
        QCOMPARE(pm->rowCount(), total);

        // Sort: pick a column, verify it round-trips through activeSort, and
        // that sorting alone never changes which rows are present.
        const QVariantList sortCols = c->sortColumns(pm);
        QVERIFY(!sortCols.isEmpty());
        const int sortRevBefore = c->sortRev();
        c->applySort(pm, "project_name", true);
        QVERIFY(c->sortRev() > sortRevBefore);
        QVariantMap sort = c->activeSort(pm);
        QCOMPARE(sort.value("field").toString(), QStringLiteral("project_name"));
        QCOMPARE(sort.value("descending").toBool(), true);
        QCOMPARE(pm->rowCount(), total);

        c->clearSort(pm);
        sort = c->activeSort(pm);
        QVERIFY(sort.value("field").toString().isEmpty());
    }

    void test_15_viewOptions()
    {
        const bool a = c->showClosedProjects();
        c->setShowClosedProjects(!a);
        QCOMPARE(c->showClosedProjects(), !a);
        c->setShowClosedProjects(a);
        QCOMPARE(c->showClosedProjects(), a);

        const bool b = c->showInternalItems();
        c->setShowInternalItems(!b);
        QCOMPARE(c->showInternalItems(), !b);
        c->setShowInternalItems(b);

        const bool d = c->newAndAssignedOnly();
        c->setNewAndAssignedOnly(!d);
        QCOMPARE(c->newAndAssignedOnly(), !d);
        c->setNewAndAssignedOnly(d);

        // Show Project Folders defaults to on, and round-trips like the rest.
        QVERIFY(c->showProjectSidebar());
        c->setShowProjectSidebar(false);
        QCOMPARE(c->showProjectSidebar(), false);
        c->setShowProjectSidebar(true);
        QCOMPARE(c->showProjectSidebar(), true);
    }

    // Regression: toggling a view option has to re-query the models it filters.
    // The QML pages bind straight to those models (there is no page re-open like
    // the Widgets app does), so marking them dirty alone left a closed project
    // on screen after Show Closed Projects was unchecked, and vice versa.
    void test_15a_showClosedProjectsRefreshesList()
    {
        const bool orig = c->showClosedProjects();

        c->setShowClosedProjects(false);

        const int row = c->addProject();
        QVERIFY(row >= 0);
        const QString name = uniq("ClosedProject_");
        QVERIFY2(c->saveProject(row, uniq("PN"), name,
                                "Closed", m_personId, m_clientId,
                                "07/01/2026", "07/15/2026",
                                c->invoicingPeriodOptions().first(),
                                c->statusReportPeriodOptions().first(),
                                "10000", "4200", "3900", "4100", "10000"),
                 qPrintable(c->lastSaveError()));
        const QString closedId = c->lastCreatedProjectId();
        QVERIFY(!closedId.isEmpty());

        c->setShowClosedProjects(true);
        QVERIFY2(rowForId(c->projectsListModel(), closedId) >= 0,
                 "closed project missing from the list after enabling Show Closed Projects");

        c->setShowClosedProjects(false);
        QVERIFY2(rowForId(c->projectsListModel(), closedId) < 0,
                 "closed project still listed after unchecking Show Closed Projects");

        // Clean up the fixture (only visible again with closed projects shown).
        c->setShowClosedProjects(true);
        const int r = c->projectRowForId(closedId);
        QVERIFY(r >= 0);
        QVERIFY2(c->deleteProject(r), qPrintable(c->lastSaveError()));

        c->setShowClosedProjects(orig);
    }

    void test_16_preferences()
    {
        c->setManagingCompanyId(m_clientId);
        QCOMPARE(c->managingCompanyId(), m_clientId);
        c->setProjectManagerId(m_personId);
        QCOMPARE(c->projectManagerId(), m_personId);
    }

    void test_17_xmlExportImport()
    {
        const QString audienceId = uniq("audience_");
        const QString audienceName = uniq("Audience_");
        QSqlQuery insert(global_DBObjects.getDb());
        insert.prepare("INSERT INTO project_email_audiences "
                       "(id, project_id, workflow, audience_name, people_source, company_filter, "
                       "selected_person_ids_json, default_workflows_json) "
                       "VALUES (?, ?, 'project', ?, 'chosen-people', 'all', ?, '[\"status\"]')");
        insert.addBindValue(audienceId);
        insert.addBindValue(m_projectId);
        insert.addBindValue(audienceName);
        insert.addBindValue(QString("[\"%1\"]").arg(m_personId));
        QVERIFY2(insert.exec(), qPrintable(insert.lastError().text()));

        const QString table = c->tableNameForModel(c->projectsListModel());
        QVERIFY2(!table.isEmpty(), "could not resolve source table for the projects model");

        const QString path = QDir::temp().filePath(uniq("pn_export_") + ".xml");
        QVERIFY2(c->exportRecordXml(table, m_projectId, path),
                 qPrintable(c->lastSaveError()));
        QFileInfo fi(path);
        QVERIFY(fi.exists());
        QVERIFY(fi.size() > 0);

        QFile exported(path);
        QVERIFY(exported.open(QFile::ReadOnly));
        QDomDocument xml;
        QVERIFY(xml.setContent(&exported));
        exported.close();
        const auto audiences = xml.elementsByTagName("table");
        bool foundAudience = false;
        for (int i = 0; i < audiences.count(); ++i)
            foundAudience |= audiences.at(i).toElement().attribute("name") == "project_email_audiences";
        QVERIFY2(foundAudience, "project XML omitted audience groups");

        QSqlQuery remove(global_DBObjects.getDb());
        remove.prepare("DELETE FROM project_email_audiences WHERE id = ?");
        remove.addBindValue(audienceId);
        QVERIFY2(remove.exec(), qPrintable(remove.lastError().text()));

        // Re-importing the exported record must succeed (round-trip).
        QVERIFY2(c->importXmlFile(path), "re-import of exported project failed");
        QSqlQuery restored(global_DBObjects.getDb());
        restored.prepare("SELECT project_id, audience_name, selected_person_ids_json, default_workflows_json "
                         "FROM project_email_audiences WHERE id = ? AND deleted = 0");
        restored.addBindValue(audienceId);
        QVERIFY2(restored.exec(), qPrintable(restored.lastError().text()));
        QVERIFY(restored.next());
        QCOMPARE(restored.value(0).toString(), m_projectId);
        QCOMPARE(restored.value(1).toString(), audienceName);
        QCOMPARE(restored.value(2).toString(), QString("[\"%1\"]").arg(m_personId));
        QCOMPARE(restored.value(3).toString(), QString("[\"status\"]"));
        QFile::remove(path);
    }

    void test_18_deleteProject()
    {
        // A staged row that was never written is dropped by discardNewProject();
        // deleteProject() is for stored records (its UPDATE would match nothing).
        const int stagedRow = c->addProject();
        QVERIFY(stagedRow >= 0);
        const int staged = c->projectsListModel()->rowCount();
        QVERIFY(c->discardNewProject(stagedRow));
        QCOMPARE(c->projectsListModel()->rowCount(), staged - 1);

        // A saved project — including the default PM team row the save added —
        // goes through deleteProject().
        const int row = c->addProject();
        QVERIFY(row >= 0);
        QVERIFY2(c->saveProject(row, uniq("PN"), uniq("Doomed_"),
                                c->projectStatusOptions().first(),
                                m_personId, m_clientId, "07/01/2026", "07/15/2026",
                                c->invoicingPeriodOptions().first(),
                                c->statusReportPeriodOptions().first(),
                                "0", "0", "0", "0", "0"),
                 qPrintable(c->lastSaveError()));
        const QString id = c->lastCreatedProjectId();
        QVERIFY(!id.isEmpty());

        const int r = c->projectRowForId(id);
        QVERIFY(r >= 0);
        const int cnt = c->projectsListModel()->rowCount();
        QVERIFY2(c->deleteProject(r), qPrintable(c->lastSaveError()));
        QCOMPARE(c->projectsListModel()->rowCount(), cnt - 1);
        QCOMPARE(c->projectRowForId(id), -1);
    }

    // Regression: with no default Project Manager configured, adding a project
    // must NOT create a project_people row (an empty people_id row is junk that
    // breaks XML export/import).
    void test_19_noDefaultPmNoTeamRow()
    {
        c->setProjectManagerId("");   // clear the default PM
        QVERIFY(c->projectManagerId().isEmpty());

        // The project has to be written for the default-PM step to run at all —
        // it happens right after the staged row is INSERTed (insertStagedProject).
        const int row = c->addProject();
        QVERIFY(row >= 0);
        QVERIFY2(c->saveProject(row, uniq("PN"), uniq("NoPmProject_"),
                                c->projectStatusOptions().first(),
                                "", m_clientId, "07/01/2026", "07/15/2026",
                                c->invoicingPeriodOptions().first(),
                                c->statusReportPeriodOptions().first(),
                                "0", "0", "0", "0", "0"),
                 qPrintable(c->lastSaveError()));
        const QString pid = c->lastCreatedProjectId();
        QVERIFY(!pid.isEmpty());

        c->setProjectFilter(pid);
        QCOMPARE(c->projectTeamMembersModel()->rowCount(), 0);
    }

    // Copy Item (Widgets parity): duplicates a tracker item with a new id and a
    // "Copy of …" name, and the copy shows up in the cross-project list.
    void test_20_copyTrackerItem()
    {
        QVERIFY(!m_itemId.isEmpty());
        const QString newId = c->copyTrackerItem(m_itemId);
        QVERIFY2(!newId.isEmpty(), qPrintable(c->lastSaveError()));
        QVERIFY(newId != m_itemId);

        c->refreshAllItems();
        QVERIFY(rowForId(c->allItemsModel(), newId) >= 0);

        c->openTrackerItem(newId);
        QVERIFY(mapHasValue(c->getTrackerItemDetailData(0), "Copy of"));
    }

    // Move a tracker item to a different project: no-op on the item's own
    // project, kept item_number when free in the destination, renumbered on a
    // collision, assigned_to/identified_by auto-added to the destination
    // project's team when missing, and a linked meeting (note_id) cleared.
    // Mirrors Widgets' ItemDetailsDelegate::verifyProjectNumber(), but for the
    // QML bridge's checkTrackerItemMove()/moveTrackerItem() (passing an empty
    // note id, matching the sidebar drag/drop path's unconditional clear).
    void test_20b_moveTrackerItemToProject()
    {
        // A second project to move items into.
        const int destRow = c->addProject();
        QVERIFY(destRow >= 0);
        QVERIFY2(c->saveProject(destRow, uniq("PN"), uniq("DestProject_"),
                                c->projectStatusOptions().first(),
                                m_personId, m_clientId, "07/01/2026", "07/15/2026",
                                c->invoicingPeriodOptions().first(),
                                c->statusReportPeriodOptions().first(),
                                "0", "0", "0", "0", "0"),
                 qPrintable(c->lastSaveError()));
        const QString destProjectId = c->lastCreatedProjectId();
        QVERIFY(!destProjectId.isEmpty());

        // A person who is not (yet) a member of destProjectId, to exercise the
        // auto-add path.
        const int prow = c->addContact();
        QVERIFY(prow >= 0);
        const QString moverName = uniq("Mover_");
        QVERIFY2(c->saveContact(prow, moverName, "mv@example.com", "", "",
                               m_clientId, "Analyst"),
                 qPrintable(c->lastSaveError()));
        const QString moverId = idForColumnValue(c->contactsModel(), 1, moverName);
        QVERIFY(!moverId.isEmpty());

        // ── No-op: "moving" to the item's own project is not a valid move ──────
        {
            QCOMPARE(c->addTrackerItem(m_projectId), 0);
            const QString itemId = c->trackerItemDetailModel()->data(
                c->trackerItemDetailModel()->index(0, 0)).toString();
            QVERIFY2(c->saveTrackerItemDetail(0, itemId, uniq("N"),
                        c->itemTypeOptions().first(), uniq("SameProj_"), "",
                        "", "", c->itemPriorityOptions().first(),
                        c->itemStatusOptions().first(), kDate1, kDate2, false,
                        QString(), QString()),
                     qPrintable(c->lastSaveError()));
            QVERIFY(!c->checkTrackerItemMove(itemId, m_projectId).value("valid").toBool());
        }

        // ── Number free in the destination: kept as-is, mover auto-added ───────
        QString freeItemId, freeNumber;
        {
            QCOMPARE(c->addTrackerItem(m_projectId), 0);
            freeItemId = c->trackerItemDetailModel()->data(
                c->trackerItemDetailModel()->index(0, 0)).toString();
            freeNumber = uniq("F");
            QVERIFY2(c->saveTrackerItemDetail(0, freeItemId, freeNumber,
                        c->itemTypeOptions().first(), uniq("FreeNum_"), "",
                        moverId, moverId, c->itemPriorityOptions().first(),
                        c->itemStatusOptions().first(), kDate1, kDate2, false,
                        QString(), QString()),
                     qPrintable(c->lastSaveError()));

            const QVariantMap chk = c->checkTrackerItemMove(freeItemId, destProjectId);
            QVERIFY(chk.value("valid").toBool());
            QCOMPARE(chk.value("oldNumber").toString(), freeNumber);
            QCOMPARE(chk.value("newNumber").toString(), freeNumber);
            QVERIFY(!chk.value("willRenumber").toBool());
            QVERIFY(!chk.value("willClearMeeting").toBool());
            // moverId is both assigned_to and identified_by → de-duplicated to one entry.
            const QVariantList members = chk.value("membersToAdd").toList();
            QCOMPARE(members.size(), 1);
            QCOMPARE(members.first().toMap().value("id").toString(), moverId);

            QVERIFY2(c->moveTrackerItem(freeItemId, destProjectId, ""),
                     qPrintable(c->lastSaveError()));

            c->openTrackerItem(freeItemId);
            const QVariantMap after = c->getTrackerItemDetailData(0);
            QCOMPARE(after.value("project_id").toString(), destProjectId);
            QCOMPARE(after.value("item_number").toString(), freeNumber);

            const QVariantList destTeam = c->teamMemberList(destProjectId);
            bool found = false;
            for (const QVariant& v : destTeam)
                if (v.toMap().value("id").toString() == moverId) found = true;
            QVERIFY(found);
        }

        // ── Number collides in the destination: renumbered ─────────────────────
        {
            QCOMPARE(c->addTrackerItem(m_projectId), 0);
            const QString itemId = c->trackerItemDetailModel()->data(
                c->trackerItemDetailModel()->index(0, 0)).toString();
            // Reuses freeNumber, which the destination project now already holds.
            QVERIFY2(c->saveTrackerItemDetail(0, itemId, freeNumber,
                        c->itemTypeOptions().first(), uniq("Collide_"), "",
                        "", "", c->itemPriorityOptions().first(),
                        c->itemStatusOptions().first(), kDate1, kDate2, false,
                        QString(), QString()),
                     qPrintable(c->lastSaveError()));

            const QVariantMap chk = c->checkTrackerItemMove(itemId, destProjectId);
            QVERIFY(chk.value("valid").toBool());
            QVERIFY(chk.value("willRenumber").toBool());
            const QString newNumber = chk.value("newNumber").toString();
            QVERIFY(!newNumber.isEmpty());
            QVERIFY(newNumber != freeNumber);

            QVERIFY2(c->moveTrackerItem(itemId, destProjectId, ""),
                     qPrintable(c->lastSaveError()));
            c->openTrackerItem(itemId);
            QCOMPARE(c->getTrackerItemDetailData(0).value("item_number").toString(), newNumber);
        }

        // ── Linked meeting is cleared on move ───────────────────────────────────
        {
            const int aiRow = c->addNoteActionItem(m_noteId, m_projectId);
            QVERIFY(aiRow >= 0);
            const QString itemId = c->notesActionItemsModel()->data(
                c->notesActionItemsModel()->index(aiRow, 0)).toString();
            QVERIFY(!itemId.isEmpty());

            const QVariantMap chk = c->checkTrackerItemMove(itemId, destProjectId);
            QVERIFY(chk.value("valid").toBool());
            QVERIFY(chk.value("willClearMeeting").toBool());
            QVERIFY(!chk.value("meetingTitle").toString().isEmpty());

            QVERIFY2(c->moveTrackerItem(itemId, destProjectId, ""),
                     qPrintable(c->lastSaveError()));
            c->openTrackerItem(itemId);
            const QVariantMap after = c->getTrackerItemDetailData(0);
            QCOMPARE(after.value("project_id").toString(), destProjectId);
            QVERIFY(after.value("note_id").toString().isEmpty());
        }
    }

    void test_21_appVersion()
    {
        QCOMPARE(c->appVersion(), QStringLiteral("6.5.0"));
    }

    // installUpdate with no prior successful check must fail gracefully (no crash,
    // reports the failure) rather than attempting a download.
    void test_21b_installUpdateWithoutCheck()
    {
        QSignalSpy fail(c, &DesktopAppController::updateInstallFailed);
        c->installUpdate();
        QCOMPARE(fail.count(), 1);
    }

    // Send Logs to Support builds a zip of the log files and reports the result.
    // We assert it completes with a result signal, then remove any bundle it
    // created so the test leaves nothing behind on the Desktop / temp dir.
    void test_22_sendLogsToSupport()
    {
        const QStringList dirs = {
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
            QDir::tempPath()
        };
        // The archive and the .eml draft opened for it both land here.
        const QStringList patterns = { QStringLiteral("ProjectNotes-logs-*.zip"),
                                       QStringLiteral("ProjectNotes-logs-*.eml") };
        QMap<QString, QStringList> before;
        for (const QString& d : dirs)
            before[d] = QDir(d).entryList(patterns, QDir::Files);

        QSignalSpy info(c, &DesktopAppController::infoOccurred);
        QSignalSpy err(c,  &DesktopAppController::errorOccurred);
        c->sendLogsToSupport();
        QVERIFY2(info.count() + err.count() >= 1, "no result signal from sendLogsToSupport");

        // Clean up any bundle this test created.
        for (const QString& d : dirs)
            for (const QString& f : QDir(d).entryList(patterns, QDir::Files))
                if (!before[d].contains(f))
                    QFile::remove(QDir(d).filePath(f));
    }
};

int runParityTests(int argc, char** argv)
{
    ParityTest t;
    return QTest::qExec(&t, argc, argv);
}

#include "tst_parity.moc"
