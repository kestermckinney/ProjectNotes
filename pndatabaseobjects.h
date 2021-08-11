#ifndef PNDATABASEOBJECTS_H
#define PNDATABASEOBJECTS_H

#include "projectsmodel.h"
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
#include "projectactionitemsmodel.h"
#include "searchresultsmodel.h"

#include <QObject>
#include <QSqlDatabase>
#include <QFileInfo>
#include <QMessageBox>
#include <QSqlError>
#include <QStringList>
#include <QDateTime>
#include <QStringListModel>

class PNDatabaseObjects : public QObject
{
    Q_OBJECT
public:
    explicit PNDatabaseObjects(QObject *parent = nullptr);
    bool OpenDatabase(QString& databasepath);
    void CloseDatabase();
    QString Execute(const QString& sql);

    void BackupDatabase(QWidget& parent, QFileInfo& file);
    bool SaveParameter( const QString& ParameterName, const QString& ParameterValue );
    QString LoadParameter( const QString& ParameterName );

    bool ExecuteDDL(const QString& SQL);
    void SetGlobalSearches( bool Refresh );
    QString& GetDatabaseFile() { return m_DatabaseFile; }

    ClientsModel* clientsmodel() { return m_ClientsModel; }
    ClientsModel* unfilteredclientsmodel() { return m_UnfilteredClientsModel; }
    PeopleModel* peoplemodel() { return m_PeopleModel; }
    PeopleModel* companypeoplemodel() { return m_CompanyPeopleModel; }
    PeopleModel* unfilteredpeoplemodel() { return m_UnfilteredPeopleModel; }
    ProjectsModel* projectinformationmodel() { return m_ProjectInformationModel; }
    TeamsModel* teamsmodel() { return m_TeamsModel; }
    StatusReportItemsModel* statusreportitemsmodel() { return m_StatusReportItemsModel; }
    ProjectTeamMembersModel* projectteammembersmodel() { return m_ProjectTeamMembersModel; }
    ProjectLocationsModel* projectlocationsmodel() { return m_ProjectLocationsModel; }
    ProjectNotesModel* projectnotesmodel() { return m_ProjectNotesModel; }
    ActionItemProjectNotesModel* actionitemprojectnotesmodel() { return m_ActionItemProjectNotesModel; }
    ActionItemsDetailsMeetingsModel* actionitemsdetailsmeetingsmodel() { return m_ActinoItemsDetailsMeetingsModel; }
    MeetingAttendeesModel* meetingattendeesmodel() { return m_MeetingAttendeesModel; }
    NotesActionItemsModel* notesactionitemsmodel() { return m_NotesActionItemsModel; }
    ItemDetailTeamListModel* itemdetailteamlistmodel() { return m_ItemDetailTeamListModel; }
    TrackerItemCommentsModel* trackeritemscommentsmodel() { return m_TrackerItemCommentsModel; }
    ProjectActionItemsModel* projectactionitemsmodel() { return m_ProjectActionItemsModel; }
    ProjectActionItemsModel* actionitemsdetailsmodel() { return m_ActionItemDetailsModel; }
    SearchResultsModel* searchresultsmodel() { return m_SearchResultsModel; }

    // selection values for fields
    static QStringList item_type;
    static QStringList item_status;
    static QStringList item_priority;
    static QStringList project_status;
    static QStringList status_item_status;
    static QStringList invoicing_period;
    static QStringList status_report_period;
    static QStringList locations;

    // global searches
    void SetShowAllTrackerItems(bool value);
    void SetShowClosedProjects(bool value);
    bool GetShowClosedProjects();
    void SetShowInternalItems(bool value);
    bool GetShowInternalItems();
    void SetGlobalClientFilter(QString value);
    QString GetGlobalClientFilter();
    void SetGlobalProjectFilter(QString value);
    QString GetGlobalProjectFilter();
    void SetProjectManager(QString value);
    QString GetProjectManager();
    void SetManagingCompany(QString value);
    QString GetManagingCompany();

private:
    QString m_DatabaseFile;
    QSqlDatabase m_SQLiteDB;

    ClientsModel* m_ClientsModel;
    ClientsModel* m_UnfilteredClientsModel;
    PeopleModel* m_PeopleModel;
    PeopleModel* m_CompanyPeopleModel;
    PeopleModel* m_UnfilteredPeopleModel;
    ProjectsModel* m_ProjectsModel;
    ProjectsModel* m_ProjectInformationModel;
    TeamsModel* m_TeamsModel;
    StatusReportItemsModel* m_StatusReportItemsModel;
    ProjectTeamMembersModel* m_ProjectTeamMembersModel;
    ProjectLocationsModel* m_ProjectLocationsModel;
    ProjectNotesModel* m_ProjectNotesModel;
    ActionItemProjectNotesModel* m_ActionItemProjectNotesModel;
    ActionItemsDetailsMeetingsModel* m_ActinoItemsDetailsMeetingsModel;
    MeetingAttendeesModel* m_MeetingAttendeesModel;
    NotesActionItemsModel* m_NotesActionItemsModel;
    ItemDetailTeamListModel* m_ItemDetailTeamListModel;
    TrackerItemCommentsModel* m_TrackerItemCommentsModel;
    ProjectActionItemsModel* m_ProjectActionItemsModel;
    ProjectActionItemsModel* m_ActionItemDetailsModel;
    SearchResultsModel* m_SearchResultsModel;

signals:

};

extern PNDatabaseObjects global_DBObjects;

#endif // PNDATABASEOBJECTS_H
