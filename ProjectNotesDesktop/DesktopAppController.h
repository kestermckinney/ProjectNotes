// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DESKTOPAPPCONTROLLER_H
#define DESKTOPAPPCONTROLLER_H

#include <QObject>
#include <QAbstractItemModel>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class QQmlEngine;
class QJSEngine;
class QThread;
class SqliteSyncPro;
struct SyncResult;

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
                                        const QString& statusReportPeriod);

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

    // ── Status report items ──────────────────────────────────────────────────
    Q_INVOKABLE int         addStatusItem(const QString& projectId);
    Q_INVOKABLE bool        deleteStatusItem(int row);
    Q_INVOKABLE QVariantMap getStatusItemData(int row) const;
    Q_INVOKABLE bool        saveStatusItem(int row, const QString& category, const QString& description);

    Q_INVOKABLE QString     lastSaveError() const;

    // ── Cloud sync ───────────────────────────────────────────────────────────
    Q_INVOKABLE void syncNow();    // configure + trigger an immediate sync cycle
    Q_INVOKABLE void stopSync();   // stop the background sync loop

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

    static DesktopAppController* s_instance;
    static QString s_developerProfile;
    static bool    s_testSupabase;
};

#endif // DESKTOPAPPCONTROLLER_H
