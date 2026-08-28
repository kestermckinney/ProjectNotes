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
#include <QStandardPaths>
#include <QUuid>

#include "DesktopAppController.h"

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
// addClient()/addPerson() only assign the record id when it is first *saved*, so
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
    }

    void test_01_clientCrud()
    {
        const int before = c->clientsModel()->rowCount();
        const int row = c->addClient();
        QVERIFY(row >= 0);

        const QString name = uniq("Client_");
        QVERIFY2(c->saveClient(row, name), qPrintable(c->lastSaveError()));
        QCOMPARE(c->clientsModel()->rowCount(), before + 1);

        const QString id = idForColumnValue(c->clientsModel(), 1, name);  // col 1 = client_name
        QVERIFY(!id.isEmpty());
        QCOMPARE(c->clientNameForId(id), name);

        m_clientId = id;
    }

    void test_02_personCrud()
    {
        const int before = c->peopleModel()->rowCount();
        const int row = c->addPerson();
        QVERIFY(row >= 0);

        const QString name = uniq("Person_");
        QVERIFY2(c->savePerson(row, name, "p@example.com", "555-1000", "555-2000",
                               m_clientId, "Engineer"),
                 qPrintable(c->lastSaveError()));
        QCOMPARE(c->peopleModel()->rowCount(), before + 1);

        const QString id = idForColumnValue(c->peopleModel(), 1, name);  // col 1 = name
        QVERIFY(!id.isEmpty());
        QCOMPARE(c->peopleNameForId(id), name);

        m_personId = id;
    }

    void test_03_projectCrud()
    {
        const int before = c->projectsListModel()->rowCount();
        const int row = c->addProject();
        QVERIFY(row >= 0);
        // addProject() only stages the row in the model cache; the record — and
        // with it the id — is written by the first save, once the NOT NULL
        // number/name are filled in. Same contract as addClient()/addPerson().
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

    void test_04_pickerLists()
    {
        // Client / person pickers used by the project + person forms.
        QVERIFY(c->clientList().size()  >= 1);
        QVERIFY(c->peopleList().size()  >= 1);
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
                    "Follow up on the deliverable."),
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
                    kDate1, "08/31/2026", false),
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
        const int prow = c->addPerson();
        QVERIFY(prow >= 0);
        const QString memberName = uniq("Member_");
        QVERIFY2(c->savePerson(prow, memberName, "m@example.com", "", "",
                               m_clientId, "Analyst"),
                 qPrintable(c->lastSaveError()));
        const QString memberId = idForColumnValue(c->peopleModel(), 1, memberName);
        QVERIFY(!memberId.isEmpty());

        c->setProjectFilter(m_projectId);
        const int before = c->projectTeamMembersModel()->rowCount();
        const int row = c->addTeamMember(m_projectId);
        QVERIFY(row >= 0);
        QVERIFY2(c->saveTeamMember(row, memberId, "Analyst", true),
                 qPrintable(c->lastSaveError()));
        QCOMPARE(c->projectTeamMembersModel()->rowCount(), before + 1);
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
        const QString table = c->tableNameForModel(c->projectsListModel());
        QVERIFY2(!table.isEmpty(), "could not resolve source table for the projects model");

        const QString path = QDir::temp().filePath(uniq("pn_export_") + ".xml");
        QVERIFY2(c->exportRecordXml(table, m_projectId, path),
                 qPrintable(c->lastSaveError()));
        QFileInfo fi(path);
        QVERIFY(fi.exists());
        QVERIFY(fi.size() > 0);

        // Re-importing the exported record must succeed (round-trip).
        QVERIFY2(c->importXmlFile(path), "re-import of exported project failed");
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
        const int prow = c->addPerson();
        QVERIFY(prow >= 0);
        const QString moverName = uniq("Mover_");
        QVERIFY2(c->savePerson(prow, moverName, "mv@example.com", "", "",
                               m_clientId, "Analyst"),
                 qPrintable(c->lastSaveError()));
        const QString moverId = idForColumnValue(c->peopleModel(), 1, moverName);
        QVERIFY(!moverId.isEmpty());

        // ── No-op: "moving" to the item's own project is not a valid move ──────
        {
            QCOMPARE(c->addTrackerItem(m_projectId), 0);
            const QString itemId = c->trackerItemDetailModel()->data(
                c->trackerItemDetailModel()->index(0, 0)).toString();
            QVERIFY2(c->saveTrackerItemDetail(0, itemId, uniq("N"),
                        c->itemTypeOptions().first(), uniq("SameProj_"), "",
                        "", "", c->itemPriorityOptions().first(),
                        c->itemStatusOptions().first(), kDate1, kDate2, false),
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
                        c->itemStatusOptions().first(), kDate1, kDate2, false),
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
                        c->itemStatusOptions().first(), kDate1, kDate2, false),
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
        QCOMPARE(c->appVersion(), QStringLiteral("6.1.0"));
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
        const QString pattern = QStringLiteral("ProjectNotes-logs-*.zip");
        QMap<QString, QStringList> before;
        for (const QString& d : dirs)
            before[d] = QDir(d).entryList({ pattern }, QDir::Files);

        QSignalSpy info(c, &DesktopAppController::infoOccurred);
        QSignalSpy err(c,  &DesktopAppController::errorOccurred);
        c->sendLogsToSupport();
        QVERIFY2(info.count() + err.count() >= 1, "no result signal from sendLogsToSupport");

        // Clean up any bundle this test created.
        for (const QString& d : dirs)
            for (const QString& f : QDir(d).entryList({ pattern }, QDir::Files))
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
