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

class SqliteSyncPro;
class QQmlEngine;
class QJSEngine;

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
    Q_INVOKABLE void setProjectFilter(const QString& projectId);

    // ── Preferences helpers — index lookup and row-based setters ────────────
    Q_INVOKABLE int  managingCompanyIndex() const;
    Q_INVOKABLE void setManagingCompanyByRow(int row);
    Q_INVOKABLE int  projectManagerIndex() const;
    Q_INVOKABLE void setProjectManagerByRow(int row);

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
    Q_INVOKABLE int     clientRowForId(const QString& clientId) const;
    Q_INVOKABLE QString clientIdAtRow(int row) const;
    Q_INVOKABLE QString clientNameForId(const QString& clientId) const;
    Q_INVOKABLE int     peopleRowForId(const QString& peopleId) const;
    Q_INVOKABLE QString peopleIdAtRow(int row) const;
    Q_INVOKABLE QString peopleNameForId(const QString& personId) const;
    Q_INVOKABLE QString peopleEmailForId(const QString& personId) const;
    Q_INVOKABLE int     teamMemberRowForPersonId(const QString& personId) const;
    Q_INVOKABLE QString teamMemberPersonIdAtRow(int row) const;
    Q_INVOKABLE QString teamMemberEmailList() const;
    Q_INVOKABLE QString attendeeEmailList() const;
    Q_INVOKABLE QString projectNumberForId(const QString& projectId) const;
    Q_INVOKABLE QString projectNameForId(const QString& projectId) const;
    Q_INVOKABLE QString htmlToPlainText(const QString& html) const;
    Q_INVOKABLE QString lastSaveError() const;

    // ── Add / Delete / Copy (returns new proxy row, or -1 on failure) ────────
    Q_INVOKABLE int          addProject();
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
                                bool           internalItem);
    Q_INVOKABLE QString     trackerItemIdAtRow(int row) const;
    Q_INVOKABLE bool        isItemNumberUnique(const QString& itemId, const QString& itemNumber) const;

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

    // ── Quick search ─────────────────────────────────────────────────────────
    // Sets a client-side text filter on any proxy model.  Any row where at
    // least one column value contains |text| (case-insensitive) is shown;
    // all others are hidden.  Pass an empty string to clear.
    Q_INVOKABLE void        setQuickSearch(QAbstractItemModel* model, const QString& text);

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

    // ── Sync settings accessors ──────────────────────────────────────────────
    bool    syncEnabled()          const { return global_MobileSettings.getSyncEnabled(); }
    void    setSyncEnabled(bool v)       { global_MobileSettings.setSyncEnabled(v); emit syncSettingsChanged(); }

    int     syncHostType()         const { return global_MobileSettings.getSyncHostType(); }
    void    setSyncHostType(int v)       { global_MobileSettings.setSyncHostType(v); emit syncSettingsChanged(); }

    QString syncEmail()            const { return global_MobileSettings.getSyncEmail(); }
    void    setSyncEmail(const QString& v) { global_MobileSettings.setSyncEmail(v); emit syncSettingsChanged(); }

    QString syncPassword()         const { return global_MobileSettings.getSyncPassword(); }
    void    setSyncPassword(const QString& v) { global_MobileSettings.setSyncPassword(v); emit syncSettingsChanged(); }

    QString syncPostgrestUrl()     const { return global_MobileSettings.getSyncPostgrestUrl(); }
    void    setSyncPostgrestUrl(const QString& v) { global_MobileSettings.setSyncPostgrestUrl(v); emit syncSettingsChanged(); }

    QString syncSupabaseKey()      const { return global_MobileSettings.getSyncSupabaseKey(); }
    void    setSyncSupabaseKey(const QString& v) { global_MobileSettings.setSyncSupabaseKey(v); emit syncSettingsChanged(); }

    QString syncEncryptionPhrase() const { return global_MobileSettings.getSyncEncryptionPhrase(); }
    void    setSyncEncryptionPhrase(const QString& v) { global_MobileSettings.setSyncEncryptionPhrase(v); emit syncSettingsChanged(); }

    qreal   syncProgress() const { return m_syncProgress; }
    bool    syncHasError() const { return m_syncHasError; }

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
    void errorOccurred(const QString& title, const QString& message);
    void databaseReady();
    void viewOptionsChanged();

private slots:
    void onSyncComplete(const SyncResult& result);
    void onSyncProgress(const QString& tableName, int processed, int total);
    void onSyncStatusUpdated(int percentComplete, qint64 pendingPush, qint64 pendingPull);
    void onSyncSettingsChanged();
    void onSyncRowChanged(const QString& tableName, const QString& id);

private:
    SqliteSyncPro* m_syncApi      = nullptr;
    qreal          m_syncProgress = -1.0;  // -1 = bar hidden
    bool           m_syncHasError = false;

    void configureSyncApi();
    void setSyncProgress(qreal progress, bool hasError = false);
};

#endif // APPCONTROLLER_H
