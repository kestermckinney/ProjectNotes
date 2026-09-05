// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DESKTOPAPPCONTROLLER_H
#define DESKTOPAPPCONTROLLER_H

#include <QObject>
#include <QAbstractItemModel>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <initializer_list>
#include <utility>

class QQmlEngine;
class QJSEngine;
class QThread;
class UpdateManager;
class SqliteSyncPro;
struct SyncResult;
class PluginManager;
class Plugin;
class FileFinderService;

// DesktopAppController — the QML bridge for the desktop app.
//
// Mirrors the ProjectNotesMobile AppController pattern (Q_PROPERTY models +
// Q_INVOKABLE CRUD forwarding to global_DBObjects). Grown per phase as screens
// port. Phase 1: shell, projects list, folders. Phase 2: project detail,
// project notes, meeting/note detail (attendees + action items).
//
// It deliberately does NOT own sync yet — the Widgets app keeps its own sync
// lifecycle while the two run side by side. Sync folds in at the parity phase.
class DesktopAppController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel* projectsListModel     READ projectsListModel     NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* projectNotesModel     READ projectNotesModel     NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* meetingAttendeesModel READ meetingAttendeesModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* notesActionItemsModel READ notesActionItemsModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* peopleModel           READ peopleModel           NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* clientsModel          READ clientsModel          NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* allItemsModel         READ allItemsModel         NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* trackerItemDetailModel READ trackerItemDetailModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* trackerItemCommentsModel READ trackerItemCommentsModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* projectTrackerItemsModel READ projectTrackerItemsModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* projectTeamMembersModel READ projectTeamMembersModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* projectLocationsModel READ projectLocationsModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* statusReportItemsModel READ statusReportItemsModel NOTIFY databaseReady)
    Q_PROPERTY(QAbstractItemModel* searchResultsModel    READ searchResultsModel    NOTIFY databaseReady)
    Q_PROPERTY(QObject* fileFinder READ fileFinder CONSTANT)
    Q_PROPERTY(bool databaseOpen READ databaseOpen NOTIFY databaseReady)

    // View options (two-way bindable from the Settings screen).
    Q_PROPERTY(bool showClosedProjects READ showClosedProjects WRITE setShowClosedProjects NOTIFY viewOptionsChanged)
    Q_PROPERTY(bool showInternalItems  READ showInternalItems  WRITE setShowInternalItems  NOTIFY viewOptionsChanged)
    Q_PROPERTY(bool newAndAssignedOnly READ newAndAssignedOnly WRITE setNewAndAssignedOnly NOTIFY viewOptionsChanged)

    // Bumped (coalesced per event-loop turn) whenever the sidebar's per-folder
    // project snapshots go stale — projects proxy reset/filter/data change or a
    // folder membership change. FolderGroup references it so its rows/count
    // bindings re-fetch folderProjects().
    Q_PROPERTY(int sidebarRev READ sidebarRev NOTIFY sidebarRevChanged)

    // Cloud sync — settings are read/written to the same QSettings the Widgets
    // app uses, so both share one configuration.
    Q_PROPERTY(bool    syncEnabled          READ syncEnabled          WRITE setSyncEnabled          NOTIFY syncSettingsChanged)
    Q_PROPERTY(QString syncEmail            READ syncEmail            WRITE setSyncEmail            NOTIFY syncSettingsChanged)
    Q_PROPERTY(QString syncPassword         READ syncPassword         WRITE setSyncPassword         NOTIFY syncSettingsChanged)
    Q_PROPERTY(QString syncEncryptionPhrase READ syncEncryptionPhrase WRITE setSyncEncryptionPhrase NOTIFY syncSettingsChanged)
    Q_PROPERTY(qreal   syncProgress          READ syncProgress NOTIFY syncProgressChanged)  // -1 = idle/hidden, else 0..1
    Q_PROPERTY(int     syncPercent           READ syncPercent  NOTIFY syncProgressChanged)  // 0..100
    Q_PROPERTY(bool    syncHasError          READ syncHasError NOTIFY syncProgressChanged)
    Q_PROPERTY(bool    syncNetworkError      READ syncNetworkError NOTIFY syncProgressChanged)
    Q_PROPERTY(bool    syncActive            READ syncActive   NOTIFY syncProgressChanged)
    Q_PROPERTY(QString syncDetail            READ syncDetail   NOTIFY syncProgressChanged)
    Q_PROPERTY(QString subscriptionStatusText READ subscriptionStatusText NOTIFY subscriptionStatusChanged)
    Q_PROPERTY(QString projectManagerInitials READ projectManagerInitials NOTIFY projectManagerChanged)
    Q_PROPERTY(QString supabaseConnectionInfo READ supabaseConnectionInfo CONSTANT)
    // True once a cloud-sync credential/phrase field has been edited and not yet
    // checked against the host — the Settings screen gates navigation on it (see
    // verifySyncSettings). Cleared by a completed check.
    Q_PROPERTY(bool syncSettingsUnverified READ syncSettingsUnverified NOTIFY syncSettingsUnverifiedChanged)
    Q_PROPERTY(bool syncVerifyInProgress   READ syncVerifyInProgress   NOTIFY syncVerifyInProgressChanged)

    // The size, in logical pixels, of the font the OS draws menus with.
    // Theme.qml's menu type ramp is built on it, so the hand-rolled popup menus
    // (AppMenu, RecordContextMenu, SortMenu, MenuFlyout, the spell-check and
    // zoom menus) match native menus instead of a hardcoded guess. CONSTANT: it
    // is sampled once at startup, so a system font change takes effect on the
    // next launch.
    Q_PROPERTY(int menuFontPixelSize READ menuFontPixelSize CONSTANT)

public:
    explicit DesktopAppController(QObject* parent = nullptr);
    ~DesktopAppController() override;

    static DesktopAppController* create(QQmlEngine* engine, QJSEngine* scriptEngine);

    // Matches the Widgets app's --developer-profile: appends a subdirectory to
    // the data location so a separate dev database is used. Set before the QML
    // engine loads (from main.cpp).
    static void setDeveloperProfile(const QString& profile) { s_developerProfile = profile; }

    // --test-supabase: route cloud sync at the TEST Supabase instance instead of
    // production. Set from main.cpp before the engine loads.
    static void setTestSupabase(bool test) { s_testSupabase = test; }

    // Disable the GitHub update check (used by the test runner so the suite makes
    // no network calls). Enabled by default.
    static void setUpdateChecksEnabled(bool enabled) { s_updateChecksEnabled = enabled; }

    // Absolute app data directory (AppDataLocation + optional developer profile).
    // Matches AppSettings::dataLocation() so both frontends share one location.
    static QString dataLocation();

    // ── Database ─────────────────────────────────────────────────────────────
    Q_INVOKABLE bool openOrCreateDatabase();
    bool databaseOpen() const { return m_databaseOpen; }
    QObject* fileFinder() const;

    // ── Models ───────────────────────────────────────────────────────────────
    QAbstractItemModel* projectsListModel() const;
    QAbstractItemModel* projectNotesModel() const;
    QAbstractItemModel* meetingAttendeesModel() const;
    QAbstractItemModel* notesActionItemsModel() const;
    QAbstractItemModel* peopleModel() const;
    QAbstractItemModel* clientsModel() const;
    QAbstractItemModel* allItemsModel() const;
    QAbstractItemModel* trackerItemDetailModel() const;
    QAbstractItemModel* trackerItemCommentsModel() const;
    QAbstractItemModel* projectTrackerItemsModel() const;
    QAbstractItemModel* projectTeamMembersModel() const;
    QAbstractItemModel* projectLocationsModel() const;
    QAbstractItemModel* statusReportItemsModel() const;
    QAbstractItemModel* searchResultsModel() const;

    // ── Global search ────────────────────────────────────────────────────────
    Q_INVOKABLE void performSearch(const QString& text);

    // ── XML import / export ──────────────────────────────────────────────────
    // Paths may be plain filesystem paths or file:// URLs (as QML FileDialog gives).
    Q_INVOKABLE bool importXmlFile(const QString& fileUrlOrPath);
    Q_INVOKABLE bool exportRecordXml(const QString& tableName, const QString& recordId,
                                     const QString& fileUrlOrPath);

    // ── Preferences (managing company / project manager, by id) ──────────────
    Q_INVOKABLE QString managingCompanyId() const;
    Q_INVOKABLE void    setManagingCompanyId(const QString& clientId);
    Q_INVOKABLE QString projectManagerId() const;
    Q_INVOKABLE void    setProjectManagerId(const QString& personId);
    // Initials derived from the configured project manager's name (e.g. "Paul
    // McKinney" -> "PM"); empty when no project manager has been set.
    QString projectManagerInitials() const;

    // Index of the last-selected tab on the project detail page, remembered per
    // project (Status Report = 0, Tracker = 1, Team = 2, Locations = 3, Notes =
    // 4). Defaults to the Tracker tab when no setting has been saved yet for
    // that project.
    Q_INVOKABLE int     lastProjectDetailTab(const QString& projectId) const;
    Q_INVOKABLE void    setLastProjectDetailTab(const QString& projectId, int index);

    // Draggable header height on the project detail page, persisted per-user
    // (local QSettings, not the synced application_settings table). Returns 0
    // when no preference has been saved yet.
    Q_INVOKABLE int     projectDetailHeaderHeight() const;
    Q_INVOKABLE void    setProjectDetailHeaderHeight(int height);

    // Draggable width on the project sidebar (folder list panel), persisted
    // per-user (same local QSettings store as above). Returns 0 when no
    // preference has been saved yet, so QML falls back to Theme.sidebarWidth.
    Q_INVOKABLE int     projectSidebarWidth() const;
    Q_INVOKABLE void    setProjectSidebarWidth(int width);

    // UI zoom level (Theme.uiScale), persisted per-user (same local QSettings
    // store as above). Stored/returned as a whole percentage (e.g. 130 for
    // 1.30x) to avoid floating-point round-tripping through QSettings; returns
    // 0 when no preference has been saved yet, so QML falls back to 1.0.
    Q_INVOKABLE int     uiZoomPercent() const;
    Q_INVOKABLE void    setUiZoomPercent(int percent);

    // Main window geometry, persisted per-user (same shared QSettings store, but
    // under its own key namespace — this is a separate top-level window from the
    // Widgets app's, which persists its own geometry independently). Returns an
    // empty map (no "valid" key) the first time the app runs, before anything has
    // been saved; Main.qml keeps its built-in defaults in that case. Restored in
    // Component.onCompleted, saved from the window's onClosing handler.
    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void        saveWindowGeometry(int x, int y, int width, int height, bool maximized);

    // ── View options ─────────────────────────────────────────────────────────
    bool showClosedProjects() const;
    void setShowClosedProjects(bool v);
    bool showInternalItems() const;
    void setShowInternalItems(bool v);
    bool newAndAssignedOnly() const;
    void setNewAndAssignedOnly(bool v);

    // ── Quick search ─────────────────────────────────────────────────────────
    Q_INVOKABLE void setQuickSearch(QAbstractItemModel* model, const QString& text);
    // Current quick-search text for a model — lets the top bar's search field
    // resync itself to whichever section it's showing when the rail switches.
    Q_INVOKABLE QString getQuickSearch(QAbstractItemModel* model) const;

    // ── Clipboard ────────────────────────────────────────────────────────────
    // Plain text off the system clipboard, for the note editor's "Paste
    // Unformatted" — mirrors TextEdit::slotPasteUnformated() in the Widgets
    // app, which inserts clipboard->text() directly instead of going through
    // the normal rich-text paste() path.
    Q_INVOKABLE QString clipboardPlainText() const;

    // ── Platform font metrics ────────────────────────────────────────────────
    // Backing reader for the property above: the platform's own menu font
    // (macOS: [NSFont menuFontOfSize:0], 13pt; Windows: the lfMenuFont out of
    // NONCLIENTMETRICS, usually Segoe UI 9pt = 12px at 96dpi), falling back to
    // the general UI font on a platform that doesn't publish a separate one.
    int menuFontPixelSize() const;

    // ── Keyboard shortcuts ───────────────────────────────────────────────────
    // Render a portable Qt key sequence ("Ctrl+N", "Ctrl+,") as the current
    // platform's native display text ("Ctrl+N" on Windows/Linux, "⌘N" on
    // macOS). AppShortcuts.qml holds the one canonical portable sequence per
    // action; this is just the display side — the actual Shortcut items in
    // Main.qml bind the same portable strings directly, and Qt's own platform
    // integration remaps Ctrl<->Cmd for them, so the key shown here always
    // matches the key that actually fires.
    Q_INVOKABLE QString nativeShortcutText(const QString& portableSequence) const;

    // ── Column filter editor (mirrors the Widgets Filter Data dialog) ────────
    // Searchable columns of a list model: [{ field, label, isDate }].
    Q_INVOKABLE QVariantList filterColumns(QAbstractItemModel* model) const;
    // Distinct display values present in a column (for the value checkboxes).
    // Distinct values for a filterable column, as [{value, label}, ...]. `value`
    // is the raw stored value (what filtering matches against); `label` is what
    // to display — for foreign-key columns (e.g. client_id) that resolves the
    // id to its lookup table's display column (e.g. client_name), matching the
    // Widgets filter dialog's delegate-rendered value list.
    Q_INVOKABLE QVariantList columnDistinctValues(QAbstractItemModel* model, const QString& field) const;
    // Apply the editor's per-column selections. Each spec:
    //   { field, values:[...], search:"", rangeStart:"", rangeEnd:"" }.
    Q_INVOKABLE void applyColumnFilters(QAbstractItemModel* model, const QVariantList& specs);
    // Clear all user column filters on a model.
    Q_INVOKABLE void clearColumnFilters(QAbstractItemModel* model);
    // The model's currently-active filter specs, in the same shape
    // applyColumnFilters() consumes — lets a caller preload/merge with
    // whatever's already active instead of blindly overwriting it (used by
    // FilterDialog's preload and by applyQuickFilter's merge below).
    Q_INVOKABLE QVariantList activeColumnFilters(QAbstractItemModel* model);
    // Apply one Quick Filter entry: merges `field`/`values` into whatever
    // column filters are already active (read via activeColumnFilters()) and
    // applies the combined set — so clicking a second Quick Filter stacks
    // with the first instead of replacing it. Re-clicking the same field with
    // the same values removes it instead (toggle), so a checkmarked Quick
    // Filter menu item is idempotent to click again.
    Q_INVOKABLE void applyQuickFilter(QAbstractItemModel* model, const QString& field, const QVariantList& values);
    // True if the model has any active column filter (quick or manual) —
    // drives the Filter button's highlighted state. Read filterRev first in
    // the same QML binding to pick up changes (a plain bool return has no
    // change notification of its own).
    Q_INVOKABLE bool hasActiveColumnFilters(QAbstractItemModel* model) const;
    // Bumped whenever a model's active column filters change (apply/clear/
    // quick filter) — see hasActiveColumnFilters().
    Q_PROPERTY(int filterRev READ filterRev NOTIFY filterRevChanged)
    int filterRev() const { return m_filterRev; }
    // Re-run a list model's query (context-menu "Refresh").
    Q_INVOKABLE void refreshModel(QAbstractItemModel* model);
    // The Q_PROPERTY model behind a rail section — the one canonical mapping
    // FilterDialog, the quick-search resync, and Quick Filter/Sort all share
    // (previously duplicated per-caller). Covers all 9 filterable sections:
    // projects/items/people/clients plus the 5 project sub-tab sections
    // (statusreport/trackeritems/team/locations/notes). Returns nullptr for
    // an unrecognized section.
    Q_INVOKABLE QAbstractItemModel* modelForSection(const QString& section) const;

    // ── Sort ─────────────────────────────────────────────────────────────────
    // Columns a list can be sorted by: [{ field, label }]. Deliberately not
    // filterColumns() — that skips DBNotSearchable columns, which on
    // TrackerItemsModel excludes project_name/project_number, exactly what
    // you'd most want to sort the master Items list by. Skips only the hidden
    // id column and long free-text (DBHtml) columns.
    Q_INVOKABLE QVariantList sortColumns(QAbstractItemModel* model) const;
    // Sort `model` by `field`, persisted the same way column filters are
    // (application_settings, not QSettings) so it follows the user across
    // synced machines. Sorts at the proxy (SortFilterProxyModel::sort()) so
    // foreign-key columns sort by their resolved display text, not the raw id.
    Q_INVOKABLE void applySort(QAbstractItemModel* model, const QString& field, bool descending);
    // Restore natural (the model's base ORDER BY) order.
    Q_INVOKABLE void clearSort(QAbstractItemModel* model);
    // The model's current sort, as {field, descending} ({} if unsorted) — for
    // the Sort menu's checkmarks.
    Q_INVOKABLE QVariantMap activeSort(QAbstractItemModel* model) const;
    // Bumped whenever a model's sort changes (applySort/clearSort) — read
    // first in a QML binding the same way filterRev is, for reactivity.
    Q_PROPERTY(int sortRev READ sortRev NOTIFY sortRevChanged)
    int sortRev() const { return m_sortRev; }

    // The projects in `folderId` that are currently *visible* in the projects
    // list proxy (i.e. that survive the active quick-search / column filter),
    // as [{id, project_number, project_name, project_status}]. "" = every
    // visible project (the "All Projects" group). Served from a snapshot built
    // in one pass over the proxy and cached until sidebarRev bumps — replaces
    // the old per-folder full-model Repeater + O(rows) folderVisibleCount().
    Q_INVOKABLE QVariantList folderProjects(const QString& folderId);
    int sidebarRev() const { return m_sidebarRev; }

    // ── Python plugins (right-click menus, mirrors the Widgets TableView) ─────
    // Menus registered by loaded plugins whose dataexport matches this model's
    // table, as [{ title, submenu, index }]. `index` is an opaque handle into a
    // per-call cache; pass it back to runPluginMenu. Rebuilt on each call, so
    // query it right before showing the menu.
    Q_INVOKABLE QVariantList pluginMenusForModel(QAbstractItemModel* model);
    // As above, but keyed directly by table name — for heterogeneous lists (the
    // global search results) where each row's table isn't the model's own table.
    Q_INVOKABLE QVariantList pluginMenusForTable(const QString& table);
    // Run a plugin menu against a record: exports the record to XML (scoped by
    // the menu's tablefilter) and hands it to the plugin function.
    Q_INVOKABLE void runPluginMenu(QAbstractItemModel* model,
                                   const QString& recordId, int index);
    Q_INVOKABLE void runPluginMenuForTable(const QString& table,
                                           const QString& recordId, int index);
    // The source SQL table backing a (possibly proxied) model — used by the QML
    // record menus to route Export XML for a sub-table row. Empty if unknown.
    Q_INVOKABLE QString tableNameForModel(QAbstractItemModel* model);

    // Global (dataless) plugin menus — Plugins > Settings/Utilities/etc. in the
    // Widgets menu bar (dataexport is empty, so they never appear on a record's
    // right-click menu). Surfaced from the app menu's Plugins group. Same
    // { title, submenu, index }-cache pattern as pluginMenusForTable() above.
    Q_INVOKABLE QVariantList globalPluginMenus();
    // Run one: calls the plugin function directly with its configured
    // parameter, no record/XML involved (mirrors MainWindow::slotPluginMenu).
    Q_INVOKABLE void runGlobalPluginMenu(int index);

    // ── Filters (scope child models to a project / note) ─────────────────────
    Q_INVOKABLE void setProjectFilter(const QString& projectId);
    Q_INVOKABLE void setNoteFilter(const QString& noteId);
    Q_INVOKABLE void refreshProjectNotes();
    Q_INVOKABLE void refreshMeetingAttendees();
    Q_INVOKABLE void refreshNoteActionItems();
    Q_INVOKABLE void refreshAllItems();
    // Activation-time variant: first visit loads, later visits re-query only
    // if the model was marked dirty by a write since the last refresh.
    Q_INVOKABLE void ensureAllItemsLoaded();
    Q_INVOKABLE void refreshTeamMembers();
    Q_INVOKABLE void refreshProjectLocations();
    Q_INVOKABLE void refreshStatusItems();
    Q_INVOKABLE void refreshTrackerComments();

    // ── Row / id lookup helpers ──────────────────────────────────────────────
    Q_INVOKABLE QString projectIdAtRow(int row) const;
    Q_INVOKABLE int     projectRowForId(const QString& projectId) const;
    Q_INVOKABLE QString projectNumberForId(const QString& projectId) const;
    Q_INVOKABLE QString projectNameForId(const QString& projectId) const;
    Q_INVOKABLE QString clientNameForId(const QString& clientId) const;
    Q_INVOKABLE int     clientRowForId(const QString& clientId) const;
    Q_INVOKABLE QString clientIdAtRow(int row) const;
    Q_INVOKABLE int     peopleRowForId(const QString& peopleId) const;
    Q_INVOKABLE QString peopleIdAtRow(int row) const;
    Q_INVOKABLE QString peopleNameForId(const QString& personId) const;

    // ── Picker lists ([{id,name}]) for client / person combos ────────────────
    Q_INVOKABLE QVariantList clientList() const;
    Q_INVOKABLE QVariantList peopleList() const;
    // All projects ([{id,name}], name = "number  project name") — backs the
    // "Move To…" project combo.
    Q_INVOKABLE QVariantList projectList() const;
    // Team members of a project ([{id,name}]) for the Assigned To / Identified By
    // / Primary Contact combos on project-scoped screens (mirrors the Widgets
    // team combos). Any id in `includeIds` that is not a current team member is
    // still appended, so an existing assignment to someone since removed from the
    // team continues to display.
    Q_INVOKABLE QVariantList teamMemberList(const QString& projectId,
                                            const QStringList& includeIds = {}) const;
    // Meeting notes for a project ([{id,title,date}]), newest first — backs the
    // "Move To…" meeting picker for action items.
    Q_INVOKABLE QVariantList notesForProject(const QString& projectId) const;
    // Most recent comments/updates on a tracker item ([{date,by,note}]), newest
    // first — backs the compact preview on each card of the master Items list
    // (ItemsPage).
    Q_INVOKABLE QVariantList recentCommentsForItem(const QString& itemId, int limit) const;

    // ── Static option lists (ComboBox models) ────────────────────────────────
    Q_INVOKABLE QStringList projectStatusOptions() const;
    Q_INVOKABLE QStringList invoicingPeriodOptions() const;
    Q_INVOKABLE QStringList statusReportPeriodOptions() const;
    Q_INVOKABLE QStringList itemTypeOptions() const;
    Q_INVOKABLE QStringList itemPriorityOptions() const;
    Q_INVOKABLE QStringList itemStatusOptions() const;
    Q_INVOKABLE QStringList fileTypeOptions() const;
    Q_INVOKABLE QStringList statusItemCategoryOptions() const;

    // ── Projects CRUD ────────────────────────────────────────────────────────
    // Stages a new project in the model cache and returns its proxy row; the row
    // is only written once saveProject() has a project number and name for it
    // (both are NOT NULL and unique in the schema). Use discardNewProject() to
    // drop it again if the user leaves without filling those in.
    Q_INVOKABLE int         addProject();
    Q_INVOKABLE bool        discardNewProject(int row);
    // Id of the project the last save created from a staged row. The page that
    // typed the fields adopts it from here rather than by row index, which the
    // insert can move when the list re-sorts.
    Q_INVOKABLE QString     lastCreatedProjectId() const { return m_lastCreatedProjectId; }
    // Next free 5-digit project number, offered as the starting value for a new
    // project. Nothing is written by asking.
    Q_INVOKABLE QString     nextProjectNumber() const;
    Q_INVOKABLE bool        deleteProject(int row);
    Q_INVOKABLE QVariantMap getProjectData(int row) const;
    Q_INVOKABLE bool        saveProject(int row,
                                        const QString& projectNumber, const QString& projectName,
                                        const QString& projectStatus, const QString& primaryContactId,
                                        const QString& clientId, const QString& lastStatusDate,
                                        const QString& lastInvoiceDate, const QString& invoicingPeriod,
                                        const QString& statusReportPeriod,
                                        const QString& budget, const QString& actual,
                                        const QString& bcwp, const QString& bcws,
                                        const QString& bac);

    // ── Project notes CRUD ───────────────────────────────────────────────────
    Q_INVOKABLE int         addProjectNote(const QString& projectId);
    Q_INVOKABLE bool        deleteProjectNote(int row);
    Q_INVOKABLE QVariantMap getProjectNoteData(int row) const;
    Q_INVOKABLE QString     projectNoteIdAtRow(int row) const;
    Q_INVOKABLE bool        saveProjectNote(int row, const QString& title, const QString& date,
                                            const QString& note, bool internalItem);
    // Duplicate a note (Widgets parity): copies project + title, resets the date
    // to now, leaves the note body blank, copies attendees, and does not copy
    // action items. Returns the new note's proxy row, or -1 on failure.
    Q_INVOKABLE int         copyProjectNote(const QString& noteId);

    // ── Meeting attendees CRUD ───────────────────────────────────────────────
    Q_INVOKABLE int         addAttendee(const QString& noteId);
    Q_INVOKABLE bool        deleteAttendee(int row);
    Q_INVOKABLE QVariantMap getAttendeeData(int row) const;
    Q_INVOKABLE bool        saveAttendee(int row, const QString& personId);

    // ── Note action items ────────────────────────────────────────────────────
    Q_INVOKABLE int         addNoteActionItem(const QString& noteId, const QString& projectId);
    Q_INVOKABLE bool        deleteNoteActionItem(int row);
    // Full field values for the action item at proxy |row| (for inline editing).
    Q_INVOKABLE QVariantMap getNoteActionItemData(int row) const;
    // Save the editable action-item fields inline on the notes page.
    Q_INVOKABLE bool        saveNoteActionItem(int row, const QString& itemName,
                                const QString& itemType, const QString& priority,
                                const QString& status, const QString& assignedTo,
                                const QString& identifiedBy, const QString& dateIdentified,
                                const QString& dateDue, const QString& description);

    // ── People CRUD ──────────────────────────────────────────────────────────
    Q_INVOKABLE int         addPerson();
    Q_INVOKABLE bool        deletePerson(int row);
    Q_INVOKABLE QVariantMap getPersonData(int row) const;
    Q_INVOKABLE QString     personIdAtRow(int row) const;
    Q_INVOKABLE bool        savePerson(int row, const QString& name, const QString& email,
                                       const QString& officePhone, const QString& cellPhone,
                                       const QString& clientId, const QString& role);
    // Parses vCard(s) dropped onto the People list (see vcardparser.h). Each
    // contact's company is resolved to a client (associated if it already
    // exists, created otherwise) and the person is added/looked up against
    // it. fileUrls are local file:// URLs to read and scan for vCard blocks
    // (e.g. a dropped .vcf); text is raw vCard data already carried by the
    // drop itself (a direct MIME vCard drag, or plain text starting with
    // BEGIN:VCARD) — pass an empty string when there is none. Returns the
    // number of contacts found (0 if none, which also reports errorOccurred).
    Q_INVOKABLE int         addPeopleFromVCardDrop(const QStringList& fileUrls, const QString& text);

    // ── Clients CRUD ─────────────────────────────────────────────────────────
    Q_INVOKABLE int         addClient();
    Q_INVOKABLE bool        deleteClient(int row);
    Q_INVOKABLE QVariantMap getClientData(int row) const;
    Q_INVOKABLE QString     clientIdAtProxyRow(int row) const;
    Q_INVOKABLE bool        saveClient(int row, const QString& clientName);

    // ── Tracker items (risks/issues/action items) ────────────────────────────
    Q_INVOKABLE void        openTrackerItem(const QString& itemId);
    Q_INVOKABLE int         addTrackerItem(const QString& projectId);
    Q_INVOKABLE bool        deleteTrackerItemDetail(int row);
    Q_INVOKABLE QVariantMap getTrackerItemDetailData(int row) const;
    Q_INVOKABLE QString     allItemIdAtRow(int row) const;
    Q_INVOKABLE bool        saveTrackerItemDetail(int row, const QString& itemId,
                                const QString& itemNumber, const QString& itemType,
                                const QString& itemName, const QString& description,
                                const QString& identifiedBy, const QString& assignedTo,
                                const QString& priority, const QString& status,
                                const QString& dateIdentified, const QString& dateDue,
                                bool internalItem);
    Q_INVOKABLE bool        isItemNameUnique(const QString& projectId, const QString& itemId, const QString& itemName) const;
    Q_INVOKABLE bool        isItemNumberUnique(const QString& projectId, const QString& itemId, const QString& itemNumber) const;

    // ── Move a tracker item to a different project and/or meeting ────────────
    // Read-only preview of what moving `itemId` to `newProjectId` would do:
    // { valid, projectName, oldNumber, newNumber, willRenumber, willClearMeeting,
    //   meetingTitle, membersToAdd:[{id,name}] }. `valid` is false when
    // newProjectId is empty/unknown or equals the item's current project. Used
    // for the drag-and-drop move path's confirmation prompt; the Move To…
    // dialog uses willRenumber/membersToAdd for its own inline hints and picks
    // the meeting explicitly rather than relying on willClearMeeting.
    Q_INVOKABLE QVariantMap checkTrackerItemMove(const QString& itemId, const QString& newProjectId) const;
    // Perform the move: reassigns project_id (renumbering if the current
    // item_number collides in the destination, and auto-adding assigned_to/
    // identified_by to the destination project's team) when newProjectId
    // differs from the item's current project, and always sets note_id to
    // newNoteId (empty for "no meeting"). Passing the item's current
    // project/note is a no-op. Used by both the "Move To…" dialog (explicit
    // project + meeting) and the sidebar drag/drop path (explicit project,
    // empty note — meetings are project-scoped and can't follow a cross-
    // project move).
    Q_INVOKABLE bool        moveTrackerItem(const QString& itemId, const QString& newProjectId,
                                            const QString& newNoteId);

    // ── Tracker item comments ────────────────────────────────────────────────
    Q_INVOKABLE int         addComment(const QString& itemId);
    Q_INVOKABLE bool        deleteComment(int row);
    Q_INVOKABLE QVariantMap getCommentData(int row) const;
    Q_INVOKABLE bool        saveComment(int row, const QString& date,
                                        const QString& note, const QString& updatedBy);

    // ── Project team members ─────────────────────────────────────────────────
    Q_INVOKABLE int         addTeamMember(const QString& projectId);
    Q_INVOKABLE bool        deleteTeamMember(int row);
    Q_INVOKABLE QVariantMap getTeamMemberData(int row) const;
    Q_INVOKABLE bool        saveTeamMember(int row, const QString& peopleId,
                                           const QString& role, bool receiveStatusReport);
    // Same vCard parsing as addPeopleFromVCardDrop() (client resolved/created,
    // person added/looked up), plus each contact is added to projectId's team
    // via addPersonToProjectTeam() (silently skipping anyone already on it).
    Q_INVOKABLE int         addTeamMembersFromVCardDrop(const QString& projectId,
                                const QStringList& fileUrls, const QString& text);

    // ── Project locations ────────────────────────────────────────────────────
    Q_INVOKABLE int         addProjectLocation(const QString& projectId);
    Q_INVOKABLE bool        deleteProjectLocation(int row);
    Q_INVOKABLE QVariantMap getProjectLocationData(int row) const;
    Q_INVOKABLE bool        saveProjectLocation(int row, const QString& locationType,
                                                const QString& description, const QString& path);
    // Add a location for a browsed file or a dropped file / web link. The path
    // is set on the model, which auto-detects the file type and description.
    Q_INVOKABLE bool        addProjectLocationFromUrl(const QString& projectId,
                                                      const QString& fileUrlOrPath);
    // Set just the path on an existing location row (used by the per-row browse
    // button); the model re-derives the file type + description.
    Q_INVOKABLE bool        setProjectLocationPath(int row, const QString& fileUrlOrPath);
    // Open a location with the OS default handler (browser, Office deep link, or
    // local file), using desktop services like the Widgets app.
    Q_INVOKABLE void        openProjectLocation(int row);

    // ── Status report items ──────────────────────────────────────────────────
    Q_INVOKABLE int         addStatusItem(const QString& projectId);
    Q_INVOKABLE bool        deleteStatusItem(int row);
    Q_INVOKABLE QVariantMap getStatusItemData(int row) const;
    Q_INVOKABLE bool        saveStatusItem(int row, const QString& category, const QString& description);

    Q_INVOKABLE QString     lastSaveError() const;

    // ── Duplicate a tracker item ─────────────────────────────────────────────
    // Copies the item (new id, "Copy of …" name, next item number in its
    // project) — the Widgets "Copy Item" action. Returns the new item's id, or
    // an empty string on failure.
    Q_INVOKABLE QString copyTrackerItem(const QString& itemId);

    // ── Duplicate any record ─────────────────────────────────────────────────
    // The generic Widgets "Copy" action, for every record menu in the QML shell.
    // Each record type keeps its own copy rules because the work is done by the
    // model's SqlQueryModel::copyRecord() override: a project copies its team, a
    // note copies its attendees and resets the date, a tracker item takes the
    // next item number in its project, and unique text columns come across
    // prefixed "Copy of ".
    //
    // canDuplicate*() answers whether a record menu should offer Duplicate at
    // all. False for read-only models, for tables with no model to copy with,
    // and for the two join tables — project_people and meeting_attendees —
    // whose composite unique key (parent + person) makes a copy a guaranteed
    // clash; the Widgets TableView context menu leaves out those same two.
    Q_INVOKABLE bool    canDuplicateModel(QAbstractItemModel* model);
    Q_INVOKABLE bool    canDuplicateTable(const QString& table);
    // Duplicate |recordId| from the table |model| displays. Returns the new
    // record's id, or an empty string on failure (errorOccurred() carries the
    // reason, typically a unique-value clash from copying the same row twice).
    Q_INVOKABLE QString duplicateRecord(QAbstractItemModel* model, const QString& recordId);
    // Same, addressed by table name — for menus that know only the table their
    // row came from (search hits, detail-page sub-lists).
    Q_INVOKABLE QString duplicateRecordInTable(const QString& table, const QString& recordId);

    // ── Help ▸ maintenance actions (mirror the Widgets Help menu) ────────────
    Q_INVOKABLE QString appVersion() const;   // "6.0.0"
    // Compile-time build timestamp ("Aug  7 2026 14:32:10"), same __DATE__/
    // __TIME__ source as the Widgets AboutDialog's BUILDV.
    Q_INVOKABLE QString buildTimestamp() const;
    // Qt version the running app is actually linked against (qVersion()), not
    // just the header version it was compiled with — matches the Widgets
    // AboutDialog's Qt Version line.
    Q_INVOKABLE QString qtRuntimeVersion() const;
    // Active --developer-profile name, or "" when running under the default
    // profile — matches the Widgets app's window-title "((profile))" prefix.
    Q_INVOKABLE QString developerProfile() const { return s_developerProfile; }
    // Query GitHub for a newer release; answers via updateAvailable() /
    // upToDate() / updateCheckFailed(). The silent variant is for the automatic
    // launch-time check: it stays quiet unless an update is actually available.
    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void checkForUpdatesSilent();
    // Download the platform installer for the last-offered update and run it
    // unattended (silent install + relaunch on Windows/macOS). Progress is
    // reported via updateDownloadStarted()/updateDownloadProgress() so the QML
    // shell can show a themed dialog; on success the app must quit
    // (quitForUpdate() → Qt.quit()).
    Q_INVOKABLE void installUpdate();
    // Abort an in-flight update download (themed Cancel button).
    Q_INVOKABLE void cancelUpdateDownload();
    // Zip every *.log into a timestamped archive on the Desktop and open a
    // pre-addressed support email; reports the outcome via infoOccurred().
    Q_INVOKABLE void sendLogsToSupport();

    // ── Cloud sync ───────────────────────────────────────────────────────────
    Q_INVOKABLE void syncNow();    // configure + trigger an immediate sync cycle
    // Full re-sync: marks every row dirty (re-push) and clears pull watermarks
    // (re-pull everything) — mirrors the Widgets "Sync All" menu action and the
    // Mobile app's "Sync All…" action.
    Q_INVOKABLE void syncAll();
    Q_INVOKABLE void stopSync();   // stop the background sync loop
    // Show the SqliteSyncPro stats window (upload/download chart + per-table
    // byte/row counts) — mirrors the Widgets app's File > Cloud Sync Settings >
    // Sync Stats action. m_syncApi lives on its own worker thread, but this is a
    // plain (non-queued) call, so it runs synchronously on the calling — GUI —
    // thread, which is required since it creates/shows a QWidget window.
    Q_INVOKABLE void showSyncStats();

    // Check the saved sync email/password against the sync host, and confirm the
    // saved encryption phrase can decrypt the account's existing rows. Runs off
    // the GUI thread; the verdict arrives as syncSettingsVerified(). The Settings
    // screen calls this when the user navigates away after editing any cloud-sync
    // field, so a typo is reported while they can still fix it rather than
    // showing up later as a sync that never completes.
    Q_INVOKABLE void verifySyncSettings();

    bool    syncSettingsUnverified() const { return m_syncSettingsUnverified; }
    bool    syncVerifyInProgress()   const { return m_syncVerifyInProgress; }

    bool    syncEnabled() const;
    void    setSyncEnabled(bool v);
    QString syncEmail() const;
    void    setSyncEmail(const QString& v);
    QString syncPassword() const;
    void    setSyncPassword(const QString& v);
    QString syncEncryptionPhrase() const;
    void    setSyncEncryptionPhrase(const QString& v);

    qreal   syncProgress() const { return m_syncProgress; }
    int     syncPercent()  const { return m_syncPercent; }
    bool    syncHasError() const { return m_syncHasError; }
    bool    syncNetworkError() const { return m_syncNetworkError; }
    bool    syncActive()   const { return m_syncSessionActive; }
    QString syncDetail()   const;
    QString subscriptionStatusText() const { return m_subscriptionStatusText; }
    QString supabaseConnectionInfo() const;

signals:
    void databaseReady();
    void errorOccurred(const QString& title, const QString& message);
    // A non-error informational result (e.g. Send Logs to Support outcome).
    void infoOccurred(const QString& title, const QString& message);
    // Update-check results (see checkForUpdates). releaseNotes is the GitHub
    // release body; installUpdate() acts on the last-offered release.
    void updateAvailable(const QString& version, const QString& releaseNotes);
    void upToDate(const QString& version);
    void updateCheckFailed(const QString& error);
    // Download failed (unattended install could not proceed).
    void updateInstallFailed(const QString& error);
    // The download has begun — the shell should show its themed progress dialog.
    void updateDownloadStarted();
    // Download progress; percent is 0..100, or -1 when the size is unknown.
    void updateDownloadProgress(int percent);
    // The installer/relaunch helper is running; the shell must now quit.
    void quitForUpdate();
    void viewOptionsChanged();
    void sidebarRevChanged();
    void filterRevChanged();
    void sortRevChanged();
    void projectManagerChanged();
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

private slots:
    void onSyncRowChanged(const QString& tableName, const QString& id);
    void onSyncComplete(const SyncResult& result);
    void onSyncProgress(const QString& tableName, int processed, int total);
    void onSyncStatusUpdated(int percentComplete, qint64 pendingPush, qint64 pendingPull);

private:
    // Apply an ordered set of column writes to `row`, stopping at the first
    // failure and surfacing it through the themed error dialog. Writing in order
    // and bailing early prevents partial inserts — a failed write on a NOT NULL
    // column followed by a later successful write would otherwise insert a row
    // with that column still null and raise a raw SQL constraint error — and it
    // preserves the first, most specific error rather than a downstream cascade.
    bool applyRowFields(QAbstractItemModel* model, int row,
                        std::initializer_list<std::pair<int, QVariant>> fields);

    // INSERT a project row that addProject() only staged (via
    // SqlQueryModel::insertStagedRow) and add its default project manager.
    bool insertStagedProject(int srcRow, const QVector<QPair<int, QVariant>>& fields);
    QString m_lastCreatedProjectId;   // see lastCreatedProjectId()

    // Helpers for moveTrackerItem / checkTrackerItemMove.
    bool isProjectTeamMember(const QString& projectId, const QString& peopleId) const;
    bool addPersonToProjectTeam(const QString& projectId, const QString& peopleId);

    bool m_databaseOpen = false;
    FileFinderService* m_fileFinder = nullptr;

    // ── Sidebar folder snapshots (see folderProjects/sidebarRev) ─────────────
    QHash<QString, QVariantList> m_folderSnapshot;   // folderId ("" = all) -> rows
    bool m_folderSnapshotValid   = false;
    bool m_sidebarRevPending     = false;   // a coalesced rev bump is queued
    bool m_folderMgrConnected    = false;   // FolderManager signal hooked up
    int  m_sidebarRev            = 0;
    void rebuildFolderSnapshot();
    void invalidateFolderSnapshot();

    // Bumped by applyColumnFilters/clearColumnFilters/applyQuickFilter — see
    // filterRev/hasActiveColumnFilters(). Filter changes are discrete user
    // actions (unlike sidebarRev's coalesced bursts), so no debounce needed.
    int m_filterRev = 0;
    // Bumped by applySort/clearSort — see sortRev/activeSort().
    int m_sortRev = 0;

    // Sync engine (lives on m_syncApiThread; created lazily by configureSyncApi).
    QThread*       m_syncApiThread = nullptr;
    SqliteSyncPro* m_syncApi       = nullptr;
    qreal          m_syncProgress  = -1.0;   // -1 = hidden/idle; else 0..1 (bar fill)
    // True from when a sync becomes active until the database reports 100% synced
    // (or the network drops). Drives bar visibility so it stays up and climbs to
    // 100% instead of only showing during the active transfer of each cycle.
    bool           m_syncSessionActive = false;
    int            m_syncPercent   = 0;      // 0..100 (from syncStatusUpdated)
    qint64         m_syncPendingPush = 0;
    qint64         m_syncPendingPull = 0;
    bool           m_syncHasError  = false;
    bool           m_syncNetworkError = false;
    QString        m_subscriptionStatusText;
    // Set by the sync credential/phrase setters, cleared by a completed
    // verifySyncSettings() — see syncSettingsUnverified.
    bool           m_syncSettingsUnverified = false;
    bool           m_syncVerifyInProgress   = false;

    void    configureSyncApi();
    void    setSyncProgress(qreal progress, bool hasError = false);
    void    setSubscriptionStatusText(const QString& text);
    void    setSyncSettingsUnverified(bool unverified);
    // Turns a verifySyncSettings() status into the user-facing message, clears
    // the pending flag and emits syncSettingsVerified(). GUI thread only.
    void    finishSyncVerification(const QString& status, const QString& detail);
    QString syncSetting(const QString& key) const;
    void    setSyncSetting(const QString& key, const QVariant& value);
    // Sync host for the active environment (--test-supabase picks the TEST
    // instance). Shared by configureSyncApi() and verifySyncSettings() so the
    // engine and the settings check can't drift onto different servers.
    static QString supabaseUrl();
    static QString supabaseAnonKey();

    // ── Software update (delegates to the shared UpdateManager) ───────────────
    UpdateManager* m_updater      = nullptr;
    QString        m_pendingAssetUrl;   // installer asset from the last check
    bool           m_silentCheck  = false;
    void ensureUpdater();

    static DesktopAppController* s_instance;
    static QString s_developerProfile;
    static bool    s_testSupabase;
    static bool    s_updateChecksEnabled;

    // ── Plugins ──────────────────────────────────────────────────────────────
    PluginManager* m_pluginManager = nullptr;
    void ensurePluginManager();   // create the manager once, after the DB is open

    // Per-call handle table for pluginMenusForModel → runPluginMenu.
    struct PluginMenuRef {
        Plugin* plugin = nullptr;
        QString functionname;
        QString tablefilter;
        QString parameter;
    };
    QVector<PluginMenuRef> m_pluginMenuCache;
    QVector<PluginMenuRef> m_globalPluginMenuCache;
};

#endif // DESKTOPAPPCONTROLLER_H
