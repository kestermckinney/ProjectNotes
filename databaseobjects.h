// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DATABASEOBJECTS_H
#define DATABASEOBJECTS_H

#include "projectsmodel.h"
#include "projectslistmodel.h"
#include "clientsmodel.h"
#include "peoplemodel.h"
#include "teamsmodel.h"
#include "statusreportitemsmodel.h"
#include "projectteammembersmodel.h"
#include "projectlocationsmodel.h"
#include "projectnotesmodel.h"
#include "actionitemprojectnotesmodel.h"
#include "actionitemsdetailsmeetingsmodel.h"
#include "meetingattendeesmodel.h"
#include "notesactionitemsmodel.h"
#include "itemdetailteamlistmodel.h"
#include "trackeritemcommentsmodel.h"
#include "trackeritemsmodel.h"
#include "searchresultsmodel.h"
#include "sortfilterproxymodel.h"

#include <QObject>
#include <QSqlDatabase>
#include <QFileInfo>
#include <QMessageBox>
#include <QSqlError>
#include <QStringList>
#include <QDateTime>
#include <QStringListModel>
#include <QDomAttr>
#include <QDomNodeList>
#include <QReadWriteLock>

// common colors
#define QCOLOR_YELLOW QColor(173, 172, 58)
#define QCOLOR_RED QColor(153, 33, 23)
#define QCOLOR_BLUE QColor(0, 15, 128)

// database lock macros (write-exclusive; coordinates with SqliteSyncPro's QReadWriteLock)
#define DB_LOCK db_rwlock.lockForWrite()
#define DB_UNLOCK db_rwlock.unlock()

struct KeyColumnChange
{
    enum OperationType { Insert, Update, Delete};

    QString table;
    QVariant value;
    OperationType operation_type;

    // Equality operator for duplicate checking
    bool operator==(const KeyColumnChange& other) const {
        return table == other.table &&
               value == other.value &&
                operation_type == other.operation_type;
    }
};

class DatabaseObjects : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseObjects(QObject *parent = nullptr);
    bool openDatabase(const QString& databasepath, const QString& connectionname, bool gui = true);
    bool createDatabase(const QString& databasepath);
    void closeDatabase();
    QSqlDatabase& getDb() { return m_sqliteDb; }

    QString execute(const QString& sql);
    void addModel(SqlQueryModel* model);
    void removeModel(SqlQueryModel* model);
    const QList<SqlQueryModel*>& getOpenModels() { return m_openRecordsets; }
    SqlQueryModel* createExportObject(const QString& tableName);

    void backupDatabase(const QString& file);
    bool saveParameter( const QString& parametername, const QString& parametervalue );
    QString loadParameter( const QVariant& parametername );

    void setGlobalSearches( bool refresh );
    QString& getDatabaseFile() { return m_databaseFile; }
    bool isOpen() { return !m_databaseFile.isEmpty(); }
    bool hasGUI() { return m_gui; }

    ClientsModel* clientsmodel() { return m_clientsModel; }
    ClientsModel* unfilteredclientsmodel() { return m_unfilteredclientsModel; }
    PeopleModel* peoplemodel() { return m_peopleModel; }
    PeopleModel* companypeoplemodel() { return m_companyPeopleModel; }
    PeopleModel* unfilteredpeoplemodel() { return m_unfilteredPeopleModel; }
    ProjectsModel* projectinformationmodel() { return m_projectInformationModel; }
    ProjectsListModel* projectslistmodel() { return m_projectsListModel; }
    TeamsModel* teamsmodel() { return m_teamsModel; }
    StatusReportItemsModel* statusreportitemsmodel() { return m_statusReportItemsModel; }
    ProjectTeamMembersModel* projectteammembersmodel() { return m_projectTeamMembersModel; }
    ProjectLocationsModel* projectlocationsmodel() { return m_projectLocationsModel; }
    ProjectNotesModel* projectnotesmodel() { return m_projectNotesModel; }
    ProjectNotesModel* projecteditingnotesmodel() { return m_projectEditingNotesModel; }
    ActionItemProjectNotesModel* actionitemprojectnotesmodel() { return m_actionItemProjectNotesModel; }
    ActionItemsDetailsMeetingsModel* actionitemsdetailsmeetingsmodel() { return m_actionItemsDetailsMeetingsModel; }
    ActionItemsDetailsMeetingsModel* trackeritemsmeetingsmodel() { return m_trackerItemsMeetingsModel; }
    MeetingAttendeesModel* meetingattendeesmodel() { return m_meetingAttendeesModel; }
    NotesActionItemsModel* notesactionitemsmodel() { return m_notesActionItemsModel; }
    TrackerItemCommentsModel* trackeritemscommentsmodel() { return m_trackerItemCommentsModel; }
    TrackerItemsModel* trackeritemsmodel() { return m_projectActionItemsModel; }
    TrackerItemsModel* actionitemsdetailsmodel() { return m_actionItemDetailsModel; }

    SortFilterProxyModel* clientsmodelproxy() { return m_clientsModelProxy; }
    SortFilterProxyModel* unfilteredclientsmodelproxy() { return m_unfilteredclientsModelProxy; }
    SortFilterProxyModel* peoplemodelproxy() { return m_peopleModelProxy; }
    SortFilterProxyModel* companypeoplemodelproxy() { return m_companyPeopleModelProxy; }
    SortFilterProxyModel* unfilteredpeoplemodelproxy() { return m_unfilteredPeopleModelProxy; }
    SortFilterProxyModel* projectinformationmodelproxy() { return m_projectInformationModelProxy; }
    SortFilterProxyModel* projectslistmodelproxy() { return m_projectsListModelProxy; }
    SortFilterProxyModel* teamsmodelproxy() { return m_teamsModelProxy; }
    SortFilterProxyModel* statusreportitemsmodelproxy() { return m_statusReportItemsModelProxy; }
    SortFilterProxyModel* projectteammembersmodelproxy() { return m_projectTeamMembersModelProxy; }
    SortFilterProxyModel* projectlocationsmodelproxy() { return m_projectLocationsModelProxy; }
    SortFilterProxyModel* projectnotesmodelproxy() { return m_projectNotesModelProxy; }
    SortFilterProxyModel* projecteditingnotesmodelproxy() { return m_projectEditingNotesModelProxy; }
    SortFilterProxyModel* actionitemprojectnotesmodelproxy() { return m_actionItemProjectNotesModelProxy; }
    SortFilterProxyModel* actionitemsdetailsmeetingsmodelproxy() { return m_actionItemsDetailsMeetingsModelProxy; }
    SortFilterProxyModel* trackeritemsmeetingsmodelproxy() { return m_trackerItemsMeetingsModelProxy; }
    SortFilterProxyModel* meetingattendeesmodelproxy() { return m_meetingAttendeesModelProxy; }
    SortFilterProxyModel* notesactionitemsmodelproxy() { return m_notesActionItemsModelProxy; }
    SortFilterProxyModel* trackeritemscommentsmodelproxy() { return m_trackerItemCommentsModelProxy; }
    SortFilterProxyModel* trackeritemsmodelproxy() { return m_projectActionItemsModelProxy; }
    SortFilterProxyModel* actionitemsdetailsmodelproxy() { return m_actionItemDetailsModelProxy; }

    SearchResultsModel* searchresultsmodel() { return m_searchResultsModel; }
    SortFilterProxyModel* searchresultsmodelproxy() { return m_searchResultsModelProxy; }

    // keep track of records updated, so you can reload the row in the display
    // Push a new change; skips if exact duplicate already exists
    void pushRowChange(const QString& table, const QVariant& value, const KeyColumnChange::OperationType optype = KeyColumnChange::Update);

    // Pop the last added change; returns true if successful, false if empty
    bool popRowChange(KeyColumnChange& outChange);
    void updateDisplayData();


    // selection values for fields
    static QStringList item_type;
    static QStringList item_status;
    static QStringList item_priority;
    static QStringList project_status;
    static QStringList status_item_status;
    static QStringList invoicing_period;
    static QStringList status_report_period;
    static QStringList file_types;


    SqlQueryModel* findOpenTable(const QString& tablename);

    // global searches
    void setShowResolvedTrackerItems(bool value);
    bool getShowResolvedTrackerItems();
    void setShowClosedProjects(bool value);
    bool getShowClosedProjects();
    void setShowInternalItems(bool value);
    bool getShowInternalItems();
    void setGlobalProjectFilter(const QString& value);
    QString getGlobalProjectFilter();
    void setProjectManager(const QString& value);
    QString getProjectManager();
    void setManagingCompany(const QString& value);
    QString getManagingCompany();

    QDomDocument* createXMLExportDoc(SqlQueryModel* querymodel, const QString& filter = QString());
    QDomDocument* createXMLExportDoc(QList<SqlQueryModel*>* querymodels);

    bool importXMLDoc(const QDomDocument& xmldoc);
    QList<SqlQueryModel*>* getData(const QDomDocument& xmldoc);

    // helper functions
    void addDefaultPMToProject(const QString& projectId);
    void addDefaultPMToMeeting(const QString& noteId);

private:
    QString m_databaseFile;
    QSqlDatabase m_sqliteDb;
    bool m_gui = true;

    ClientsModel* m_clientsModel = nullptr;
    ClientsModel* m_unfilteredclientsModel = nullptr;
    PeopleModel* m_peopleModel = nullptr;
    PeopleModel* m_companyPeopleModel = nullptr;
    PeopleModel* m_unfilteredPeopleModel = nullptr;
    ProjectsListModel* m_projectsListModel = nullptr;
    ProjectsModel* m_projectInformationModel = nullptr;
    TeamsModel* m_teamsModel = nullptr;
    StatusReportItemsModel* m_statusReportItemsModel = nullptr;
    ProjectTeamMembersModel* m_projectTeamMembersModel = nullptr;
    ProjectLocationsModel* m_projectLocationsModel = nullptr;
    ProjectNotesModel* m_projectNotesModel = nullptr;
    ProjectNotesModel* m_projectEditingNotesModel = nullptr;
    ActionItemProjectNotesModel* m_actionItemProjectNotesModel = nullptr;
    ActionItemsDetailsMeetingsModel* m_actionItemsDetailsMeetingsModel = nullptr;
    ActionItemsDetailsMeetingsModel* m_trackerItemsMeetingsModel = nullptr;
    MeetingAttendeesModel* m_meetingAttendeesModel = nullptr;
    NotesActionItemsModel* m_notesActionItemsModel = nullptr;
    TrackerItemCommentsModel* m_trackerItemCommentsModel = nullptr;
    TrackerItemsModel* m_projectActionItemsModel = nullptr;
    TrackerItemsModel* m_actionItemDetailsModel = nullptr;

    SearchResultsModel* m_searchResultsModel = nullptr;

    SortFilterProxyModel* m_clientsModelProxy = nullptr;
    SortFilterProxyModel* m_unfilteredclientsModelProxy = nullptr;
    SortFilterProxyModel* m_peopleModelProxy = nullptr;
    SortFilterProxyModel* m_companyPeopleModelProxy = nullptr;
    SortFilterProxyModel* m_unfilteredPeopleModelProxy = nullptr;
    SortFilterProxyModel* m_projectsListModelProxy = nullptr;
    SortFilterProxyModel* m_projectInformationModelProxy = nullptr;
    SortFilterProxyModel* m_teamsModelProxy = nullptr;
    SortFilterProxyModel* m_statusReportItemsModelProxy = nullptr;
    SortFilterProxyModel* m_projectTeamMembersModelProxy = nullptr;
    SortFilterProxyModel* m_projectLocationsModelProxy = nullptr;
    SortFilterProxyModel* m_projectNotesModelProxy = nullptr;
    SortFilterProxyModel* m_projectEditingNotesModelProxy = nullptr;
    SortFilterProxyModel* m_actionItemProjectNotesModelProxy = nullptr;
    SortFilterProxyModel* m_actionItemsDetailsMeetingsModelProxy = nullptr;
    SortFilterProxyModel* m_trackerItemsMeetingsModelProxy = nullptr;
    SortFilterProxyModel* m_meetingAttendeesModelProxy = nullptr;
    SortFilterProxyModel* m_notesActionItemsModelProxy = nullptr;
    SortFilterProxyModel* m_trackerItemCommentsModelProxy = nullptr;
    SortFilterProxyModel* m_projectActionItemsModelProxy = nullptr;
    SortFilterProxyModel* m_actionItemDetailsModelProxy = nullptr;
    SortFilterProxyModel* m_searchResultsModelProxy = nullptr;

    // xml utility functions
    QList<QDomNode> findTableNodes(const QDomNode& xmlelement, const QString& tablename);
    // list of created models
    QList<SqlQueryModel*> m_openRecordsets;

    // keep track of key column changes to update display
    QList<KeyColumnChange> m_keyColumnChanges;

signals:
    // Emitted by non-GUI instances when a row is changed, so the GUI thread
    // can forward changes to global_DBObjects via a QueuedConnection
    void rowChanged(const QString& table, const QVariant& value, int optype);

};

extern DatabaseObjects global_DBObjects;
extern QReadWriteLock db_rwlock;

#endif // DATABASEOBJECTS_H
