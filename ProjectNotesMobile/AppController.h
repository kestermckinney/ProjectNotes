// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include "MobileSettings.h"
#include "databaseobjects.h"
#include "syncresult.h"

#include <QObject>
#include <QAbstractItemModel>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <memory>

class SqliteSyncPro;
class QQmlEngine;
class QJSEngine;
class QThread;
class SortFilterProxyModel;
class SqlQueryModel;

// AppController — singleton exposed to QML as the application's main C++ object.
// Responsibilities:
//   - Open / create the local SQLite database
//   - Expose all data models to QML views
//   - Manage SqliteSyncPro sync lifecycle
//   - Persist sync credentials via MobileSettings

class AppController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // ── Model properties ────────────────────────────────────────────────────
    Q_PROPERTY(QAbstractItemModel* projectsListModel     READ projectsListModel     NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* projectsModel         READ projectsModel         NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* clientsModel          READ clientsModel          NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* peopleModel           READ peopleModel           NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* trackerItemsModel       READ trackerItemsModel       NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* allItemsModel           READ allItemsModel           NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* projectNotesModel       READ projectNotesModel       NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* meetingAttendeesModel   READ meetingAttendeesModel   NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* trackerItemCommentsModel READ trackerItemCommentsModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* notesActionItemsModel   READ notesActionItemsModel   NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* trackerItemDetailModel  READ trackerItemDetailModel  NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* trackerItemMeetingsModel READ trackerItemMeetingsModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* searchResultsModel      READ searchResultsModel      NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* statusReportItemsModel  READ statusReportItemsModel  NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* projectTeamMembersModel READ projectTeamMembersModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* projectLocationsModel   READ projectLocationsModel   NOTIFY databaseReady)

    // ── Sync settings properties (two-way bindable from QML Settings page) ──
    Q_PROPERTY(bool   syncEnabled         READ syncEnabled         WRITE setSyncEnabled         NOTIFY syncSettingsChanged)
    Q_PROPERTY(int    syncHostType        READ syncHostType        WRITE setSyncHostType        NOTIFY syncSettingsChanged)
    Q_PROPERTY(QString syncEmail          READ syncEmail          WRITE setSyncEmail           NOTIFY syncSettingsChanged)
    Q_PROPERTY(QString syncPassword       READ syncPassword       WRITE setSyncPassword        NOTIFY syncSettingsChanged)
    Q_PROPERTY(QString syncPostgrestUrl   READ syncPostgrestUrl   WRITE setSyncPostgrestUrl    NOTIFY syncSettingsChanged)
    Q_PROPERTY(QString syncSupabaseKey    READ syncSupabaseKey    WRITE setSyncSupabaseKey     NOTIFY syncSettingsChanged)
    Q_PROPERTY(QString syncEncryptionPhrase READ syncEncryptionPhrase WRITE setSyncEncryptionPhrase NOTIFY syncSettingsChanged)

    Q_PROPERTY(qreal   syncProgress READ syncProgress NOTIFY syncProgressChanged)
    Q_PROPERTY(bool    syncHasError READ syncHasError  NOTIFY syncProgressChanged)

    Q_PROPERTY(QString subscriptionStatusText  READ subscriptionStatusText  NOTIFY subscriptionStatusChanged)
    Q_PROPERTY(QString supabaseConnectionInfo  READ supabaseConnectionInfo  CONSTANT)

    // True once a sync credential/phrase field has been edited and not yet
    // checked against the host — the Cloud Sync Settings page gates its back
    // button on it (see verifySyncSettings). Cleared by a completed check.
    Q_PROPERTY(bool syncSettingsUnverified READ syncSettingsUnverified NOTIFY syncSettingsUnverifiedChanged)
    Q_PROPERTY(bool syncVerifyInProgress   READ syncVerifyInProgress   NOTIFY syncVerifyInProgressChanged)

    // ── View options properties ──────────────────────────────────────────────
    Q_PROPERTY(bool showClosedProjects  READ showClosedProjects  WRITE setShowClosedProjects  NOTIFY viewOptionsChanged)
    Q_PROPERTY(bool showInternalItems   READ showInternalItems   WRITE setShowInternalItems   NOTIFY viewOptionsChanged)
    Q_PROPERTY(bool newAndAssignedOnly  READ newAndAssignedOnly  WRITE setNewAndAssignedOnly  NOTIFY viewOptionsChanged)

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    // QML singleton factory — called by the QML engine on first use
    static AppController* create(QQmlEngine* engine, QJSEngine* scriptEngine);

    // ── Invokable actions ────────────────────────────────────────────────────
    Q_INVOKABLE bool openOrCreateDatabase();
    Q_INVOKABLE void startSync();
    Q_INVOKABLE void stopSync();
    Q_INVOKABLE void syncAll();
    // Check the saved sync email/password against the sync host, and confirm the
    // saved encryption phrase can decrypt the account's existing rows. Runs off
    // the GUI thread; the verdict arrives as syncSettingsVerified(). The Cloud
    // Sync Settings page calls this when the user leaves after editing a field,
    // so a typo is reported while they can still fix it rather than showing up
    // later as a sync that never completes. Mirrors the desktop app.
    Q_INVOKABLE void verifySyncSettings();
    Q_INVOKABLE void setProjectFilter(const QString& projectId);

    // ── Preferences helpers — id-based, NOT row-based ────────────────────────
    // Row-based equivalents used to live here (index into clientsModel/
    // peopleModel directly) — replaced because those models are now
    // Sort-able, and PreferencesPage.qml's combos stay alive on the shared
    // StackView same as every other page (see teamMemberList() doc comment).
    // The combos now bind to clientList()/peopleList() snapshots and save by id.
    Q_INVOKABLE QString managingCompanyId() const { return global_DBObjects.getManagingCompany(); }
    Q_INVOKABLE void    setManagingCompanyId(const QString& id) { global_DBObjects.setManagingCompany(id); }
    Q_INVOKABLE QString projectManagerId() const { return global_DBObjects.getProjectManager(); }
    Q_INVOKABLE void    setProjectManagerId(const QString& id) { global_DBObjects.setProjectManager(id); }

    // ── Record editing helpers ───────────────────────────────────────────────
    Q_INVOKABLE bool    savePerson(int row, const QString& name, const QString& email,
                                   const QString& officePhone, const QString& cellPhone,
                                   const QString& clientId, const QString& role);
    Q_INVOKABLE bool    saveClient(int row, const QString& clientName);
    Q_INVOKABLE bool    saveProject(int row, const QString& projectNumber,
                                    const QString& projectName, const QString& projectStatus,
                                    const QString& primaryContactId, const QString& clientId,
                                    const QString& lastStatusDate, const QString& lastInvoiceDate,
                                    const QString& invoicingPeriod,
                                    const QString& statusReportPeriod);
    Q_INVOKABLE bool    saveStatusItem(int row, const QString& category, const QString& description);
    Q_INVOKABLE bool    saveTeamMember(int row, const QString& peopleId, const QString& role, bool receiveStatusReport);
    Q_INVOKABLE bool    saveProjectLocation(int row, const QString& locationType,
                                            const QString& description, const QString& path);
    Q_INVOKABLE bool    saveProjectNote(int row, const QString& title, const QString& date,
                                        const QString& note, bool internalItem);
    Q_INVOKABLE QString clientNameForId(const QString& clientId) const;
    Q_INVOKABLE QString peopleNameForId(const QString& personId) const;
    Q_INVOKABLE QString peopleEmailForId(const QString& personId) const;
    Q_INVOKABLE QString teamMemberEmailList() const;
    Q_INVOKABLE QString attendeeEmailList() const;
    Q_INVOKABLE QString attendeeNameList() const;
    Q_INVOKABLE QString noteActionItemsSummary() const;
    Q_INVOKABLE void    sendMeetingNotesEmail(const QString& commaSeparatedEmails,
                                             const QString& subject,
                                             const QString& body);
    Q_INVOKABLE QString projectNumberForId(const QString& projectId) const;
    Q_INVOKABLE QString projectNameForId(const QString& projectId) const;
    // Stable {id,name} snapshot of a project's team, for the Primary Contact
    // combo — deliberately NOT the live projectTeamMembersModel proxy, which
    // Sort can reorder/reset out from under a ComboBox bound directly to it
    // (QQuickComboBox's currentIndex is a row number, and its internal
    // delegate model isn't safe to keep alive across that model's sort()).
    // `includeIds` keeps any currently-referenced person even if no longer on
    // the team, so an existing assignment still displays (matches desktop).
    Q_INVOKABLE QVariantList teamMemberList(const QString& projectId, const QStringList& includeIds) const;
    // Same rationale as teamMemberList() above, for combos that pick a client
    // or a person (clientsModel/peopleModel are both Sort-able) — a one-time
    // {id,name} read of the live proxy into a plain array the ComboBox binds
    // to instead, so a later sort() elsewhere can't invalidate it.
    Q_INVOKABLE QVariantList clientList() const;
    Q_INVOKABLE QVariantList peopleList() const;
    Q_INVOKABLE QString htmlToPlainText(const QString& html) const;
    Q_INVOKABLE QString lastSaveError() const;

    // ── Add / Delete / Copy (returns new proxy row, or -1 on failure) ────────
    // addProject() stages the row; it is written by the first saveProject() that
    // has a project number and name for it (both required by the schema), and
    // deleteProject() drops it again if the page is left without them.
    Q_INVOKABLE int          addProject();
    // Next free 5-digit project number, offered as a new project's starting
    // value. Nothing is written by asking.
    Q_INVOKABLE QString      nextProjectNumber() const;
    // Id of the project the last save created from a staged row. The page that
    // typed the fields adopts it from here rather than by row index, which the
    // insert can move when the list re-sorts.
    Q_INVOKABLE QString      lastCreatedProjectId() const { return m_lastCreatedProjectId; }
    Q_INVOKABLE bool         deleteProject(int row);
    Q_INVOKABLE int          copyProject(int row);
    Q_INVOKABLE QVariantMap  getProjectData(int row) const;

    Q_INVOKABLE int          addPerson();
    Q_INVOKABLE bool         deletePerson(int row);
    Q_INVOKABLE int          copyPerson(int row);
    Q_INVOKABLE QVariantMap  getPersonData(int row) const;

    Q_INVOKABLE int          addClient();
    Q_INVOKABLE bool         deleteClient(int row);
    Q_INVOKABLE int          copyClient(int row);
    Q_INVOKABLE QVariantMap  getClientData(int row) const;

    Q_INVOKABLE int          addStatusItem(const QString& projectId);
    Q_INVOKABLE bool         deleteStatusItem(int row);
    Q_INVOKABLE int          copyStatusItem(int row);
    Q_INVOKABLE QVariantMap  getStatusItemData(int row) const;

    Q_INVOKABLE int          addTeamMember(const QString& projectId);
    Q_INVOKABLE bool         deleteTeamMember(int row);
    Q_INVOKABLE int          copyTeamMember(int row);
    Q_INVOKABLE QVariantMap  getTeamMemberData(int row) const;

    Q_INVOKABLE int          addProjectLocation(const QString& projectId);
    Q_INVOKABLE bool         deleteProjectLocation(int row);
    Q_INVOKABLE int          copyProjectLocation(int row);
    Q_INVOKABLE QVariantMap  getProjectLocationData(int row) const;

    Q_INVOKABLE int          addProjectNote(const QString& projectId);
    Q_INVOKABLE bool         deleteProjectNote(int row);
    Q_INVOKABLE int          copyProjectNote(int row);
    Q_INVOKABLE QVariantMap  getProjectNoteData(int row) const;

    // ── Static list accessors (for ComboBox models in QML) ──────────────────
    Q_INVOKABLE QStringList projectStatusOptions()      const { return DatabaseObjects::project_status; }
    Q_INVOKABLE QStringList invoicingPeriodOptions()    const { return DatabaseObjects::invoicing_period; }
    Q_INVOKABLE QStringList statusReportPeriodOptions() const { return DatabaseObjects::status_report_period; }
    Q_INVOKABLE QStringList statusItemCategoryOptions() const { return DatabaseObjects::status_item_status; }
    Q_INVOKABLE QStringList fileTypeOptions()           const { return DatabaseObjects::file_types; }
    Q_INVOKABLE QStringList trackerItemTypeOptions()    const { return DatabaseObjects::item_type; }
    Q_INVOKABLE QStringList trackerItemPriorityOptions() const { return DatabaseObjects::item_priority; }
    Q_INVOKABLE QStringList trackerItemStatusOptions()  const { return DatabaseObjects::item_status; }

    // ── Tracker item filter + CRUD ────────────────────────────────────────────
    Q_INVOKABLE void        openTrackerItem(const QString& itemId);
    Q_INVOKABLE int         addTrackerItem(const QString& projectId);
    Q_INVOKABLE bool        deleteTrackerItemDetail(int row);
    Q_INVOKABLE int         copyTrackerItemDetail(int row);
    Q_INVOKABLE QVariantMap getTrackerItemDetailData(int row) const;
    Q_INVOKABLE bool        saveTrackerItemDetail(int row,
                                const QString& itemId,
                                const QString& itemNumber,
                                const QString& itemType,
                                const QString& itemName,
                                const QString& description,
                                const QString& identifiedBy,
                                const QString& assignedTo,
                                const QString& priority,
                                const QString& status,
                                const QString& dateIdentified,
                                const QString& dateDue,
                                const QString& noteId,
                                bool           internalItem);
    Q_INVOKABLE QString     trackerItemIdAtRow(int row) const;
    Q_INVOKABLE int         meetingRowForNoteId(const QString& noteId) const;
    Q_INVOKABLE QString     meetingNoteIdAtRow(int row) const;
    // Stable {id,name} snapshot backing the tracker item's Meeting combo —
    // same rationale as teamMemberList()/clientList()/peopleList() above: the
    // meetings proxy is live/sortable, and QQuickComboBox isn't safe to keep
    // bound to a model across its sort(). id = note id, name = meeting label.
    Q_INVOKABLE QVariantList meetingList() const;
    Q_INVOKABLE bool        isItemNumberUnique(const QString& projectId, const QString& itemId, const QString& itemNumber) const;
    Q_INVOKABLE bool        isItemNameUnique(const QString& projectId, const QString& itemId, const QString& itemName) const;

    // ── Tracker item comments CRUD ────────────────────────────────────────────
    Q_INVOKABLE int         addComment(const QString& itemId);
    Q_INVOKABLE bool        deleteComment(int row);
    Q_INVOKABLE int         copyComment(int row);
    Q_INVOKABLE QVariantMap getCommentData(int row) const;
    Q_INVOKABLE bool        saveComment(int row, const QString& date,
                                        const QString& note, const QString& updatedBy);

    // ── Meeting attendees CRUD ────────────────────────────────────────────────
    Q_INVOKABLE void        setNoteFilter(const QString& noteId, const QString& projectId);
    Q_INVOKABLE int         addAttendee(const QString& noteId);
    Q_INVOKABLE bool        deleteAttendee(int row);
    Q_INVOKABLE QVariantMap getAttendeeData(int row) const;
    Q_INVOKABLE bool        saveAttendee(int row, const QString& personId);

    // ── Note action items CRUD ────────────────────────────────────────────────
    Q_INVOKABLE int         addNoteActionItem(const QString& noteId, const QString& projectId);
    Q_INVOKABLE bool        deleteNoteActionItem(int row);
    Q_INVOKABLE QString     noteActionItemIdAtRow(int row) const;

    // ── Model refresh helpers (call from StackView.onActivated in list pages) ─
    Q_INVOKABLE void        refreshTeamMembers();
    Q_INVOKABLE void        refreshMeetingAttendees();
    Q_INVOKABLE void        refreshTrackerComments();
    Q_INVOKABLE void        refreshTrackerItems();
    Q_INVOKABLE void        refreshAllItems();
    Q_INVOKABLE void        refreshProjectNotes();
    Q_INVOKABLE void        refreshNoteActionItems();
    Q_INVOKABLE void        refreshProjectsList();
    Q_INVOKABLE void        refreshPeople();
    Q_INVOKABLE void        refreshClients();

    // ── Quick search ─────────────────────────────────────────────────────────
    // Sets a client-side text filter on any proxy model.  Any row where at
    // least one column value contains |text| (case-insensitive) is shown;
    // all others are hidden.  Pass an empty string to clear.
    Q_INVOKABLE void        setQuickSearch(QAbstractItemModel* model, const QString& text);

    // ── Column filter editor (mirrors DesktopAppController's Filter Editor) ──
    // Searchable columns of a list model: [{ field, label, isDate }].
    Q_INVOKABLE QVariantList filterColumns(QAbstractItemModel* model) const;
    // Distinct values for a filterable column, as [{value, label}, ...]. `value`
    // is the raw stored value (what filtering matches against); `label` is what
    // to display — for foreign-key columns (e.g. client_id) that resolves the
    // id to its lookup table's display column (e.g. client_name).
    Q_INVOKABLE QVariantList columnDistinctValues(QAbstractItemModel* model, const QString& field) const;
    // Apply the editor's per-column selections. Each spec:
    //   { field, values:[...], search:"", rangeStart:"", rangeEnd:"" }.
    Q_INVOKABLE void applyColumnFilters(QAbstractItemModel* model, const QVariantList& specs);
    // Clear all user column filters on a model.
    Q_INVOKABLE void clearColumnFilters(QAbstractItemModel* model);
    // The model's currently-active filter specs, in the same shape
    // applyColumnFilters() consumes — lets a caller preload/merge with
    // whatever's already active instead of blindly overwriting it.
    Q_INVOKABLE QVariantList activeColumnFilters(QAbstractItemModel* model);
    // Apply one Quick Filter entry: merges `field`/`values` into whatever
    // column filters are already active and applies the combined set — so
    // picking a second Quick Filter stacks with the first instead of
    // replacing it. Re-picking the same field with the same values removes
    // it instead (toggle).
    Q_INVOKABLE void applyQuickFilter(QAbstractItemModel* model, const QString& field, const QVariantList& values);
    // True if the model has any active column filter (quick or manual) —
    // drives the Filter button's highlighted state. Read filterRev first in
    // the same QML binding to pick up changes.
    Q_INVOKABLE bool hasActiveColumnFilters(QAbstractItemModel* model) const;
    // Bumped whenever a model's active column filters change (apply/clear/
    // quick filter) — see hasActiveColumnFilters().
    Q_PROPERTY(int filterRev READ filterRev NOTIFY filterRevChanged)
    int filterRev() const { return m_filterRev; }
    // The Q_PROPERTY model behind a section key — the one canonical mapping
    // FilterSheet/SortSheet/Quick Filter all share. Covers all 9 filterable
    // sections: projects/items/people/clients plus the 5 project sub-tab
    // sections (statusreport/trackeritems/team/locations/notes). Returns
    // nullptr for an unrecognized section.
    Q_INVOKABLE QAbstractItemModel* modelForSection(const QString& section) const;
    // Re-run a list model's query (pull-to-refresh / after a quick filter).
    Q_INVOKABLE void refreshModel(QAbstractItemModel* model);

    // ── Sort ─────────────────────────────────────────────────────────────────
    // Columns a list can be sorted by: [{ field, label }]. Deliberately not
    // filterColumns() — that skips non-searchable columns that are still
    // useful to sort by. Skips only the hidden id column and long free-text
    // (DBHtml) columns.
    Q_INVOKABLE QVariantList sortColumns(QAbstractItemModel* model) const;
    // Sort `model` by `field`, persisted (application_settings) so it follows
    // the user across synced machines. Sorts at the proxy
    // (SortFilterProxyModel::sort()) so foreign-key columns sort by their
    // resolved display text, not the raw id.
    Q_INVOKABLE void applySort(QAbstractItemModel* model, const QString& field, bool descending);
    // Restore natural (the model's base ORDER BY) order.
    Q_INVOKABLE void clearSort(QAbstractItemModel* model);
    // The model's current sort, as {field, descending} ({} if unsorted).
    Q_INVOKABLE QVariantMap activeSort(QAbstractItemModel* model) const;
    // Bumped whenever a model's sort changes (applySort/clearSort) — read
    // first in a QML binding the same way filterRev is, for reactivity.
    Q_PROPERTY(int sortRev READ sortRev NOTIFY sortRevChanged)
    int sortRev() const { return m_sortRev; }
    // Re-resolve a stable record id back to its current row in |model|'s
    // proxy. Mirrors DesktopAppController::projectRowForId()/
    // clientRowForId(). Detail pages call this immediately before every
    // save/delete/copy so a row captured at list-tap time that went stale
    // (a sort()/refresh() while the page was open) can't silently write to
    // the wrong record. Returns -1 if the id no longer exists.
    Q_INVOKABLE int rowForId(QAbstractItemModel* model, const QString& id) const;

    // ── Model accessors ──────────────────────────────────────────────────────
    QAbstractItemModel* projectsListModel()       const;
    QAbstractItemModel* projectsModel()           const;
    QAbstractItemModel* clientsModel()            const;
    QAbstractItemModel* peopleModel()             const;
    QAbstractItemModel* trackerItemsModel()       const;
    QAbstractItemModel* allItemsModel()           const;
    QAbstractItemModel* projectNotesModel()       const;
    QAbstractItemModel* meetingAttendeesModel()   const;
    QAbstractItemModel* searchResultsModel()      const;
    QAbstractItemModel* statusReportItemsModel()      const;
    QAbstractItemModel* projectTeamMembersModel()     const;
    QAbstractItemModel* projectLocationsModel()       const;
    QAbstractItemModel* trackerItemCommentsModel()    const;
    QAbstractItemModel* notesActionItemsModel()       const;
    QAbstractItemModel* trackerItemDetailModel()      const;
    QAbstractItemModel* trackerItemMeetingsModel()    const;

    // ── Sync settings accessors ──────────────────────────────────────────────
    // The four fields the Cloud Sync Settings page edits are implemented in the
    // .cpp: each ignores a no-op write and marks the settings unverified so the
    // page can check them on the way out (see verifySyncSettings).
    bool    syncEnabled()          const { return global_MobileSettings.getSyncEnabled(); }
    void    setSyncEnabled(bool v);

    int     syncHostType()         const { return global_MobileSettings.getSyncHostType(); }
    void    setSyncHostType(int v)       { global_MobileSettings.setSyncHostType(v); emit syncSettingsChanged(); }

    QString syncEmail()            const { return global_MobileSettings.getSyncEmail(); }
    void    setSyncEmail(const QString& v);

    QString syncPassword()         const { return global_MobileSettings.getSyncPassword(); }
    void    setSyncPassword(const QString& v);

    QString syncPostgrestUrl()     const { return global_MobileSettings.getSyncPostgrestUrl(); }
    void    setSyncPostgrestUrl(const QString& v) { global_MobileSettings.setSyncPostgrestUrl(v); emit syncSettingsChanged(); }

    QString syncSupabaseKey()      const { return global_MobileSettings.getSyncSupabaseKey(); }
    void    setSyncSupabaseKey(const QString& v) { global_MobileSettings.setSyncSupabaseKey(v); emit syncSettingsChanged(); }

    QString syncEncryptionPhrase() const { return global_MobileSettings.getSyncEncryptionPhrase(); }
    void    setSyncEncryptionPhrase(const QString& v);

    qreal   syncProgress() const { return m_syncProgress; }
    bool    syncHasError() const { return m_syncHasError; }
    bool    syncSettingsUnverified() const { return m_syncSettingsUnverified; }
    bool    syncVerifyInProgress()   const { return m_syncVerifyInProgress; }

    QString subscriptionStatusText() const { return m_subscriptionStatusText; }
    QString supabaseConnectionInfo() const;

    // ── View options accessors ───────────────────────────────────────────────
    bool showClosedProjects() const { return global_DBObjects.getShowClosedProjects(); }
    void setShowClosedProjects(bool v)
    {
        global_DBObjects.setShowClosedProjects(v);
        global_DBObjects.setGlobalSearches(true);
        emit viewOptionsChanged();
    }

    bool showInternalItems() const { return global_DBObjects.getShowInternalItems(); }
    void setShowInternalItems(bool v)
    {
        global_DBObjects.setShowInternalItems(v);
        global_DBObjects.setGlobalSearches(true);
        emit viewOptionsChanged();
    }

    bool newAndAssignedOnly() const { return !global_DBObjects.getShowResolvedTrackerItems(); }
    void setNewAndAssignedOnly(bool v);  // implemented in .cpp — needs model filter calls

signals:
    void syncSettingsChanged();
    void syncProgressChanged();
    void subscriptionStatusChanged();
    void subscriptionExpired();
    // Verdict from verifySyncSettings(). status is one of:
    //   "ok"          — the host accepted the credentials and the phrase decrypts
    //   "credentials" — the host rejected the email/password
    //   "encryption"  — signed in, but no server row decrypts with this phrase
    //   "offline"     — the host was unreachable, so nothing could be checked
    //   "skipped"     — sync is off or unconfigured; there was nothing to check
    // message is empty for "ok"/"skipped" and user-facing text otherwise.
    void syncSettingsVerified(const QString& status, const QString& message);
    void syncSettingsUnverifiedChanged();
    void syncVerifyInProgressChanged();
    void errorOccurred(const QString& title, const QString& message);
    void databaseReady();
    void viewOptionsChanged();
    void filterRevChanged();
    void sortRevChanged();

private slots:
    void onSyncComplete(const SyncResult& result);
    void onSyncProgress(const QString& tableName, int processed, int total);
    void onSyncStatusUpdated(int percentComplete, qint64 pendingPush, qint64 pendingPull);
    void onSyncSettingsChanged();
    void onSyncRowChanged(const QString& tableName, const QString& id);

private:
    // INSERT a project row that addProject() only staged (via
    // SqlQueryModel::insertStagedRow) and add its default project manager.
    bool insertStagedProject(int srcRow, const QVector<QPair<int, QVariant>>& fields);
    QString m_lastCreatedProjectId;   // see lastCreatedProjectId()

    // m_syncApi lives on m_syncApiThread so its blocking startup work
    // (network auth, WAL pragma, table discovery, persistent DB open)
    // does not stall the UI thread. The thread is long-lived because the
    // persistent DB connection that initialize() opens is bound to it,
    // and syncAll() / shutdown() must run on the same thread.
    QThread*       m_syncApiThread = nullptr;
    SqliteSyncPro* m_syncApi       = nullptr;
    qreal          m_syncProgress  = -1.0;  // -1 = bar hidden
    bool           m_syncHasError  = false;
    QString        m_subscriptionStatusText;
    // Set by the sync credential/phrase setters, cleared by a completed
    // verifySyncSettings() — see syncSettingsUnverified.
    bool           m_syncSettingsUnverified = false;
    bool           m_syncVerifyInProgress   = false;

    // Bumped by applyColumnFilters/clearColumnFilters/applyQuickFilter and
    // applySort/clearSort respectively — see filterRev()/sortRev() above.
    int            m_filterRev     = 0;
    int            m_sortRev       = 0;

    void configureSyncApi();
    void setSyncProgress(qreal progress, bool hasError = false);
    void setSubscriptionStatusText(const QString& text);
    void setSyncSettingsUnverified(bool unverified);
    // Turns a verifySyncSettings() status into the user-facing message, clears
    // the pending flag and emits syncSettingsVerified(). GUI thread only.
    void finishSyncVerification(const QString& status, const QString& detail);
    // Sync host for the active environment (the TEST instance when the build is
    // pointed at it). Shared by configureSyncApi() and verifySyncSettings() so
    // the engine and the settings check can't drift onto different servers.
    static QString supabaseUrl();
    static QString supabaseAnonKey();

    // Applies every filterable section's persisted sort order. Deferred (via
    // QTimer::singleShot) off the end of openOrCreateDatabase() rather than
    // called inline — see that method for why.
    void restoreAllSorts();

    // Delete the record at proxy |row|, surfacing the model's blocked-delete
    // message (child/foreign-key references) via errorOccurred() on failure.
    // Returns true only if the record was actually deleted.
    bool deleteAndReport(SortFilterProxyModel* proxy, SqlQueryModel* source, int row);
};

#endif // APPCONTROLLER_H
