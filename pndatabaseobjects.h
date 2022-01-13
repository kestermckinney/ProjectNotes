#ifndef PNDATABASEOBJECTS_H
#define PNDATABASEOBJECTS_H

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
#include "projectactionitemsmodel.h"
#include "searchresultsmodel.h"
#include <pnsortfilterproxymodel.h>

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
    explicit PNDatabaseObjects(QObject *t_parent = nullptr);
    bool OpenDatabase(QString& t_databasepath);
    void CloseDatabase();
    QString Execute(const QString& t_sql);

    void BackupDatabase(QWidget& t_parent, QFileInfo& t_file);
    bool SaveParameter( const QString& t_parametername, const QString& t_parametervalue );
    QString LoadParameter( const QString& t_parametername );

    bool ExecuteDDL(const QString& t_sql);
    void SetGlobalSearches( bool t_refresh );
    QString& GetDatabaseFile() { return m_database_file; }
    bool isOpen() { return !m_database_file.isEmpty(); }

    ClientsModel* clientsmodel() { return m_clients_model; }
    ClientsModel* unfilteredclientsmodel() { return m_unfilteredclients_model; }
    PeopleModel* peoplemodel() { return m_people_model; }
    PeopleModel* companypeoplemodel() { return m_company_people_model; }
    PeopleModel* unfilteredpeoplemodel() { return m_unfiltered_people_model; }
    ProjectsModel* projectinformationmodel() { return m_project_information_model; }
    ProjectsListModel* projectslistmodel() { return m_projects_list_model; }
    TeamsModel* teamsmodel() { return m_teams_model; }
    StatusReportItemsModel* statusreportitemsmodel() { return m_status_report_items_model; }
    ProjectTeamMembersModel* projectteammembersmodel() { return m_project_team_members_model; }
    ProjectLocationsModel* projectlocationsmodel() { return m_project_locations_model; }
    ProjectNotesModel* projectnotesmodel() { return m_project_notes_model; }
    ActionItemProjectNotesModel* actionitemprojectnotesmodel() { return m_action_item_project_notes_model; }
    ActionItemsDetailsMeetingsModel* actionitemsdetailsmeetingsmodel() { return m_action_items_details_meetings_model; }
    MeetingAttendeesModel* meetingattendeesmodel() { return m_meeting_attendees_model; }
    NotesActionItemsModel* notesactionitemsmodel() { return m_notes_action_items_model; }
    ItemDetailTeamListModel* itemdetailteamlistmodel() { return m_item_detail_team_list_model; }
    TrackerItemCommentsModel* trackeritemscommentsmodel() { return m_tracker_item_comments_model; }
    ProjectActionItemsModel* projectactionitemsmodel() { return m_project_action_items_model; }
    ProjectActionItemsModel* actionitemsdetailsmodel() { return m_action_item_details_model; }

    PNSortFilterProxyModel* clientsmodelproxy() { return m_clients_model_proxy; }
    PNSortFilterProxyModel* unfilteredclientsmodelproxy() { return m_unfilteredclients_model_proxy; }
    PNSortFilterProxyModel* peoplemodelproxy() { return m_people_model_proxy; }
    PNSortFilterProxyModel* companypeoplemodelproxy() { return m_company_people_model_proxy; }
    PNSortFilterProxyModel* unfilteredpeoplemodelproxy() { return m_unfiltered_people_model_proxy; }
    PNSortFilterProxyModel* projectinformationmodelproxy() { return m_project_information_model_proxy; }
    PNSortFilterProxyModel* projectslistmodelproxy() { return m_projects_list_model_proxy; }
    PNSortFilterProxyModel* teamsmodelproxy() { return m_teams_model_proxy; }
    PNSortFilterProxyModel* statusreportitemsmodelproxy() { return m_status_report_items_model_proxy; }
    PNSortFilterProxyModel* projectteammembersmodelproxy() { return m_project_team_members_model_proxy; }
    PNSortFilterProxyModel* projectlocationsmodelproxy() { return m_project_locations_model_proxy; }
    PNSortFilterProxyModel* projectnotesmodelproxy() { return m_project_notes_model_proxy; }
    PNSortFilterProxyModel* actionitemprojectnotesmodelproxy() { return m_action_item_project_notes_model_proxy; }
    PNSortFilterProxyModel* actionitemsdetailsmeetingsmodelproxy() { return m_action_items_details_meetings_model_proxy; }
    PNSortFilterProxyModel* meetingattendeesmodelproxy() { return m_meeting_attendees_model_proxy; }
    PNSortFilterProxyModel* notesactionitemsmodelproxy() { return m_notes_action_items_model_proxy; }
    PNSortFilterProxyModel* itemdetailteamlistmodelproxy() { return m_item_detail_team_list_model_proxy; }
    PNSortFilterProxyModel* trackeritemscommentsmodelproxy() { return m_tracker_item_comments_model_proxy; }
    PNSortFilterProxyModel* projectactionitemsmodelproxy() { return m_project_action_items_model_proxy; }
    PNSortFilterProxyModel* actionitemsdetailsmodelproxy() { return m_action_item_details_model_proxy; }

    SearchResultsModel* searchresultsmodel() { return m_search_results_model; }

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
    void SetShowAllTrackerItems(bool t_value);
    void SetShowClosedProjects(bool t_value);
    bool GetShowClosedProjects();
    void SetShowInternalItems(bool t_value);
    bool GetShowInternalItems();
    void SetGlobalClientFilter(QString t_value);
    QString GetGlobalClientFilter();
    void SetGlobalProjectFilter(QString t_value);
    QString GetGlobalProjectFilter();
    void SetProjectManager(QString t_value);
    QString GetProjectManager();
    void SetManagingCompany(QString t_value);
    QString GetManagingCompany();

private:
    QString m_database_file;
    QSqlDatabase m_sqlite_db;

    ClientsModel* m_clients_model;
    ClientsModel* m_unfilteredclients_model;
    PeopleModel* m_people_model;
    PeopleModel* m_company_people_model;
    PeopleModel* m_unfiltered_people_model;
    ProjectsListModel* m_projects_list_model;
    ProjectsModel* m_project_information_model;
    TeamsModel* m_teams_model;
    StatusReportItemsModel* m_status_report_items_model;
    ProjectTeamMembersModel* m_project_team_members_model;
    ProjectLocationsModel* m_project_locations_model;
    ProjectNotesModel* m_project_notes_model;
    ActionItemProjectNotesModel* m_action_item_project_notes_model;
    ActionItemsDetailsMeetingsModel* m_action_items_details_meetings_model;
    MeetingAttendeesModel* m_meeting_attendees_model;
    NotesActionItemsModel* m_notes_action_items_model;
    ItemDetailTeamListModel* m_item_detail_team_list_model;
    TrackerItemCommentsModel* m_tracker_item_comments_model;
    ProjectActionItemsModel* m_project_action_items_model;
    ProjectActionItemsModel* m_action_item_details_model;

    SearchResultsModel* m_search_results_model;

    PNSortFilterProxyModel* m_clients_model_proxy;
    PNSortFilterProxyModel* m_unfilteredclients_model_proxy;
    PNSortFilterProxyModel* m_people_model_proxy;
    PNSortFilterProxyModel* m_company_people_model_proxy;
    PNSortFilterProxyModel* m_unfiltered_people_model_proxy;
    PNSortFilterProxyModel* m_projects_list_model_proxy;
    PNSortFilterProxyModel* m_project_information_model_proxy;
    PNSortFilterProxyModel* m_teams_model_proxy;
    PNSortFilterProxyModel* m_status_report_items_model_proxy;
    PNSortFilterProxyModel* m_project_team_members_model_proxy;
    PNSortFilterProxyModel* m_project_locations_model_proxy;
    PNSortFilterProxyModel* m_project_notes_model_proxy;
    PNSortFilterProxyModel* m_action_item_project_notes_model_proxy;
    PNSortFilterProxyModel* m_action_items_details_meetings_model_proxy;
    PNSortFilterProxyModel* m_meeting_attendees_model_proxy;
    PNSortFilterProxyModel* m_notes_action_items_model_proxy;
    PNSortFilterProxyModel* m_item_detail_team_list_model_proxy;
    PNSortFilterProxyModel* m_tracker_item_comments_model_proxy;
    PNSortFilterProxyModel* m_project_action_items_model_proxy;
    PNSortFilterProxyModel* m_action_item_details_model_proxy;

signals:

};

extern PNDatabaseObjects global_DBObjects;

#endif // PNDATABASEOBJECTS_H
