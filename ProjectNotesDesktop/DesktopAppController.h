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
    Q_PROPERTY(bool databaseOpen READ databaseOpen NOTIFY databaseReady)

    // View options (two-way bindable from the Settings screen).
    Q_PROPERTY(bool showClosedProjects READ showClosedProjects WRITE setShowClosedProjects NOTIFY viewOptionsChanged)
    Q_PROPERTY(bool showInternalItems  READ showInternalItems  WRITE setShowInternalItems  NOTIFY viewOptionsChanged)
    Q_PROPERTY(bool newAndAssignedOnly READ newAndAssignedOnly WRITE setNewAndAssignedOnly NOTIFY viewOptionsChanged)

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
    Q_PROPERTY(QString supabaseConnectionInfo READ supabaseConnectionInfo CONSTANT)

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

    // ── View options ─────────────────────────────────────────────────────────
    bool showClosedProjects() const;
    void setShowClosedProjects(bool v);
    bool showInternalItems() const;
    void setShowInternalItems(bool v);
    bool newAndAssignedOnly() const;
    void setNewAndAssignedOnly(bool v);

    // ── Quick search ─────────────────────────────────────────────────────────
    Q_INVOKABLE void setQuickSearch(QAbstractItemModel* model, const QString& text);

    // ── Column filter editor (mirrors the Widgets Filter Data dialog) ────────
    // Searchable columns of a list model: [{ field, label, isDate }].
    Q_INVOKABLE QVariantList filterColumns(QAbstractItemModel* model) const;
    // Distinct display values present in a column (for the value checkboxes).
    Q_INVOKABLE QStringList  columnDistinctValues(QAbstractItemModel* model, const QString& field) const;
    // Apply the editor's per-column selections. Each spec:
    //   { field, values:[...], search:"", rangeStart:"", rangeEnd:"" }.
    Q_INVOKABLE void applyColumnFilters(QAbstractItemModel* model, const QVariantList& specs);
    // Clear all user column filters on a model.
    Q_INVOKABLE void clearColumnFilters(QAbstractItemModel* model);
    // Re-run a list model's query (context-menu "Refresh").
    Q_INVOKABLE void refreshModel(QAbstractItemModel* model);

    // Count the projects in `folderId` that are currently *visible* in `model`
    // (i.e. that survive the active quick-search / column filter). With no
    // filter active this equals the folder's full membership, so sidebar folder
    // badges show the filtered result set only while filtering is on.
    Q_INVOKABLE int folderVisibleCount(QAbstractItemModel* model, const QString& folderId) const;

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

    // ── Filters (scope child models to a project / note) ─────────────────────
    Q_INVOKABLE void setProjectFilter(const QString& projectId);
    Q_INVOKABLE void setNoteFilter(const QString& noteId);
    Q_INVOKABLE void refreshProjectNotes();
    Q_INVOKABLE void refreshMeetingAttendees();
    Q_INVOKABLE void refreshNoteActionItems();
    Q_INVOKABLE void refreshAllItems();
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
    // Team members of a project ([{id,name}]) for the Assigned To / Identified By
    // / Primary Contact combos on project-scoped screens (mirrors the Widgets
    // team combos). Any id in `includeIds` that is not a current team member is
    // still appended, so an existing assignment to someone since removed from the
    // team continues to display.
    Q_INVOKABLE QVariantList teamMemberList(const QString& projectId,
                                            const QStringList& includeIds = {}) const;

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
    Q_INVOKABLE int         addProject();
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

    // ── Help ▸ maintenance actions (mirror the Widgets Help menu) ────────────
    Q_INVOKABLE QString appVersion() const;   // "6.0.0"
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
    Q_INVOKABLE void stopSync();   // stop the background sync loop
    // Show the SqliteSyncPro stats window (upload/download chart + per-table
    // byte/row counts) — mirrors the Widgets app's File > Cloud Sync Settings >
    // Sync Stats action. m_syncApi lives on its own worker thread, but this is a
    // plain (non-queued) call, so it runs synchronously on the calling — GUI —
    // thread, which is required since it creates/shows a QWidget window.
    Q_INVOKABLE void showSyncStats();

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
    bool    syncActive()   const { return m_syncProgress >= 0.0; }
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
    void syncSettingsChanged();
    void syncProgressChanged();
    void subscriptionStatusChanged();
    void subscriptionExpired();

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

    bool m_databaseOpen = false;

    // Sync engine (lives on m_syncApiThread; created lazily by configureSyncApi).
    QThread*       m_syncApiThread = nullptr;
    SqliteSyncPro* m_syncApi       = nullptr;
    qreal          m_syncProgress  = -1.0;   // -1 = hidden/idle
    int            m_syncPercent   = 0;      // 0..100 (from syncStatusUpdated)
    qint64         m_syncPendingPush = 0;
    qint64         m_syncPendingPull = 0;
    bool           m_syncHasError  = false;
    bool           m_syncNetworkError = false;
    QString        m_subscriptionStatusText;

    void    configureSyncApi();
    void    setSyncProgress(qreal progress, bool hasError = false);
    void    setSubscriptionStatusText(const QString& text);
    QString syncSetting(const QString& key) const;
    void    setSyncSetting(const QString& key, const QVariant& value);

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
};

#endif // DESKTOPAPPCONTROLLER_H
