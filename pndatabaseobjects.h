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

#include <QObject>
#include <QSqlDatabase>
#include <QFileInfo>
#include <QMessageBox>
#include <QSqlError>
#include <QStringList>
#include <QDateTime>

class PNDatabaseObjects : public QObject
{
    Q_OBJECT
public:
    explicit PNDatabaseObjects(QString& databasepath, QObject *parent = nullptr);
    bool OpenDatabase();
    void CloseDatabase();

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
    bool GetShowClosedProjects() { return m_ShowClosedProjects; }
    bool GetShowInternalItems() { return m_ShowInternalItems; }
    QString& GetGlobalClientFilter() { return m_GlobalClientFilter; }
    QString& GetGlobalProjectFilter() { return m_GlobalProjectFilter; }
    void SetShowClosedProjects(bool show) { m_ShowClosedProjects = show; }
    void SetShowInternalItems(bool show) { m_ShowInternalItems = show; }
    void SetGlobalClientFilter(QString& filter) { m_GlobalClientFilter = filter; }
    void SetGlobalProjectFilter(QString& filter) { m_GlobalProjectFilter = filter; }

private:
    // global searches
    static bool m_ShowClosedProjects;
    static bool m_ShowInternalItems;
    static QString m_GlobalClientFilter;
    static QString m_GlobalProjectFilter;


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

signals:

};

bool PNDatabaseObjects::m_ShowClosedProjects = false;
bool PNDatabaseObjects::m_ShowInternalItems = true;

#endif // PNDATABASEOBJECTS_H
