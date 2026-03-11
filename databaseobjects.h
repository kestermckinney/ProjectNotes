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
#include <QMutex>

// common colors
#define QCOLOR_YELLOW QColor(173, 172, 58)
#define QCOLOR_RED QColor(153, 33, 23)
#define QCOLOR_BLUE QColor(0, 15, 128)

// setup a locking macro
#define DB_LOCK db_mutex.lock()
#define DB_UNLOCK db_mutex.unlock()

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
    bool createDatabase(QString& databasepath);
    void closeDatabase();
    QSqlDatabase& getDb() { return m_sqliteDb; }

    QString execute(const QString& sql);
    void addModel(SqlQueryModel* model);
    void removeModel(SqlQueryModel* model);
    QList<SqlQueryModel*> getOpenModels() {  return m_openRecordsets; }
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
    void addColumnChanges(const DatabaseObjects& source);
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
    void setGlobalClientFilter(QString value);
    QString getGlobalClientFilter();
    void setGlobalProjectFilter(QString value);
    QString getGlobalProjectFilter();
    void setProjectManager(QString value);
    QString getProjectManager();
    void setManagingCompany(QString value);
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

    ClientsModel* m_clientsModel;
    ClientsModel* m_unfilteredclientsModel;
    PeopleModel* m_peopleModel;
    PeopleModel* m_companyPeopleModel;
    PeopleModel* m_unfilteredPeopleModel;
    ProjectsListModel* m_projectsListModel;
    ProjectsModel* m_projectInformationModel;
    TeamsModel* m_teamsModel;
    StatusReportItemsModel* m_statusReportItemsModel;
    ProjectTeamMembersModel* m_projectTeamMembersModel;
    ProjectLocationsModel* m_projectLocationsModel;
    ProjectNotesModel* m_projectNotesModel;
    ProjectNotesModel* m_projectEditingNotesModel;
    ActionItemProjectNotesModel* m_actionItemProjectNotesModel;
    ActionItemsDetailsMeetingsModel* m_actionItemsDetailsMeetingsModel;
    ActionItemsDetailsMeetingsModel* m_trackerItemsMeetingsModel;
    MeetingAttendeesModel* m_meetingAttendeesModel;
    NotesActionItemsModel* m_notesActionItemsModel;
    TrackerItemCommentsModel* m_trackerItemCommentsModel;
    TrackerItemsModel* m_projectActionItemsModel;
    TrackerItemsModel* m_actionItemDetailsModel;

    SearchResultsModel* m_searchResultsModel;

    SortFilterProxyModel* m_clientsModelProxy;
    SortFilterProxyModel* m_unfilteredclientsModelProxy;
    SortFilterProxyModel* m_peopleModelProxy;
    SortFilterProxyModel* m_companyPeopleModelProxy;
    SortFilterProxyModel* m_unfilteredPeopleModelProxy;
    SortFilterProxyModel* m_projectsListModelProxy;
    SortFilterProxyModel* m_projectInformationModelProxy;
    SortFilterProxyModel* m_teamsModelProxy;
    SortFilterProxyModel* m_statusReportItemsModelProxy;
    SortFilterProxyModel* m_projectTeamMembersModelProxy;
    SortFilterProxyModel* m_projectLocationsModelProxy;
    SortFilterProxyModel* m_projectNotesModelProxy;
    SortFilterProxyModel* m_projectEditingNotesModelProxy;
    SortFilterProxyModel* m_actionItemProjectNotesModelProxy;
    SortFilterProxyModel* m_actionItemsDetailsMeetingsModelProxy;
    SortFilterProxyModel* m_trackerItemsMeetingsModelProxy;
    SortFilterProxyModel* m_meetingAttendeesModelProxy;
    SortFilterProxyModel* m_notesActionItemsModelProxy;
    SortFilterProxyModel* m_trackerItemCommentsModelProxy;
    SortFilterProxyModel* m_projectActionItemsModelProxy;
    SortFilterProxyModel* m_actionItemDetailsModelProxy;
    SortFilterProxyModel* m_searchResultsModelProxy;

    // xml utility functions
    QList<QDomNode> findTableNodes(const QDomNode& xmlelement, const QString& tablename);
    // list of created models
    QList<SqlQueryModel*> m_openRecordsets;

    // keep track of key column changes to update display
    QList<KeyColumnChange> m_keyColumnChanges;

signals:

};

extern DatabaseObjects global_DBObjects;
extern QMutex db_mutex;

#endif // DATABASEOBJECTS_H
