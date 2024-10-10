// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

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
#include "trackeritemsmodel.h"
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
#include <QDomAttr>
#include <QDomNodeList>

// common colors
#define QCOLOR_YELLOW QColor(173, 172, 58)
#define QCOLOR_RED QColor(153, 33, 23)
#define QCOLOR_BLUE QColor(0, 15, 128)
#define QCOLOR_GRAY QColor(235, 235, 235)

class PNDatabaseObjects : public QObject
{
    Q_OBJECT
public:
    explicit PNDatabaseObjects(QObject *t_parent = nullptr);
    bool openDatabase(QString& t_databasepath);
    bool createDatabase(QString& t_databasepath);
    void closeDatabase();
    QString execute(const QString& t_sql);

    void backupDatabase(const QString& t_file);
    bool saveParameter( const QString& t_parametername, const QString& t_parametervalue );
    QString loadParameter( const QVariant& t_parametername );

    void setGlobalSearches( bool t_refresh );
    QString& getDatabaseFile() { return m_database_file; }
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
    ProjectNotesModel* projecteditingnotesmodel() { return m_project_editing_notes_model; }
    ActionItemProjectNotesModel* actionitemprojectnotesmodel() { return m_action_item_project_notes_model; }
    ActionItemsDetailsMeetingsModel* actionitemsdetailsmeetingsmodel() { return m_action_items_details_meetings_model; }
    ActionItemsDetailsMeetingsModel* trackeritemsmeetingsmodel() { return m_tracker_items_meetings_model; }
    MeetingAttendeesModel* meetingattendeesmodel() { return m_meeting_attendees_model; }
    NotesActionItemsModel* notesactionitemsmodel() { return m_notes_action_items_model; }
    ItemDetailTeamListModel* itemdetailteamlistmodel() { return m_item_detail_team_list_model; }
    TrackerItemCommentsModel* trackeritemscommentsmodel() { return m_tracker_item_comments_model; }
    TrackerItemsModel* trackeritemsmodel() { return m_project_action_items_model; }
    TrackerItemsModel* actionitemsdetailsmodel() { return m_action_item_details_model; }

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
    PNSortFilterProxyModel* projecteditingnotesmodelproxy() { return m_project_editing_notes_model_proxy; }
    PNSortFilterProxyModel* actionitemprojectnotesmodelproxy() { return m_action_item_project_notes_model_proxy; }
    PNSortFilterProxyModel* actionitemsdetailsmeetingsmodelproxy() { return m_action_items_details_meetings_model_proxy; }
    PNSortFilterProxyModel* trackeritemsmeetingsmodelproxy() { return m_tracker_items_meetings_model_proxy; }
    PNSortFilterProxyModel* meetingattendeesmodelproxy() { return m_meeting_attendees_model_proxy; }
    PNSortFilterProxyModel* notesactionitemsmodelproxy() { return m_notes_action_items_model_proxy; }
    PNSortFilterProxyModel* itemdetailteamlistmodelproxy() { return m_item_detail_team_list_model_proxy; }
    PNSortFilterProxyModel* trackeritemscommentsmodelproxy() { return m_tracker_item_comments_model_proxy; }
    PNSortFilterProxyModel* trackeritemsmodelproxy() { return m_project_action_items_model_proxy; }
    PNSortFilterProxyModel* actionitemsdetailsmodelproxy() { return m_action_item_details_model_proxy; }

    SearchResultsModel* searchresultsmodel() { return m_search_results_model; }
    PNSortFilterProxyModel* searchresultsmodelproxy() { return m_search_results_model_proxy; }

    // selection values for fields
    static QStringList item_type;
    static QStringList item_status;
    static QStringList item_priority;
    static QStringList project_status;
    static QStringList status_item_status;
    static QStringList invoicing_period;
    static QStringList status_report_period;
    static QStringList file_types;

    // global searches
    void setShowResolvedTrackerItems(bool t_value);
    bool getShowResolvedTrackerItems();
    void setShowClosedProjects(bool t_value);
    bool getShowClosedProjects();
    void setShowInternalItems(bool t_value);
    bool getShowInternalItems();
    void setGlobalClientFilter(QString t_value);
    QString getGlobalClientFilter();
    void setGlobalProjectFilter(QString t_value);
    QString getGlobalProjectFilter();
    void setProjectManager(QString t_value);
    QString getProjectManager();
    void setManagingCompany(QString t_value);
    QString getManagingCompany();

    QDomDocument* createXMLExportDoc(PNSqlQueryModel* t_querymodel, const QString& t_filter = QString());
    QDomDocument* createXMLExportDoc(QList<PNSqlQueryModel*>* t_querymodels);

    bool importXMLDoc(const QDomDocument& t_xmldoc);
    QList<PNSqlQueryModel*>* getData(const QDomDocument& t_xmldoc);

    // helper functions
    void addDefaultPMToProject(const QString& t_project_id);
    void addDefaultPMToMeeting(const QString& t_note_id);

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
    ProjectNotesModel* m_project_editing_notes_model;
    ActionItemProjectNotesModel* m_action_item_project_notes_model;
    ActionItemsDetailsMeetingsModel* m_action_items_details_meetings_model;
    ActionItemsDetailsMeetingsModel* m_tracker_items_meetings_model;
    MeetingAttendeesModel* m_meeting_attendees_model;
    NotesActionItemsModel* m_notes_action_items_model;
    ItemDetailTeamListModel* m_item_detail_team_list_model;
    TrackerItemCommentsModel* m_tracker_item_comments_model;
    TrackerItemsModel* m_project_action_items_model;
    TrackerItemsModel* m_action_item_details_model;

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
    PNSortFilterProxyModel* m_project_editing_notes_model_proxy;
    PNSortFilterProxyModel* m_action_item_project_notes_model_proxy;
    PNSortFilterProxyModel* m_action_items_details_meetings_model_proxy;
    PNSortFilterProxyModel* m_tracker_items_meetings_model_proxy;
    PNSortFilterProxyModel* m_meeting_attendees_model_proxy;
    PNSortFilterProxyModel* m_notes_action_items_model_proxy;
    PNSortFilterProxyModel* m_item_detail_team_list_model_proxy;
    PNSortFilterProxyModel* m_tracker_item_comments_model_proxy;
    PNSortFilterProxyModel* m_project_action_items_model_proxy;
    PNSortFilterProxyModel* m_action_item_details_model_proxy;
    PNSortFilterProxyModel* m_search_results_model_proxy;

    // xml utility functions
    QList<QDomNode> findTableNodes(const QDomNode& t_xmlelement, const QString& t_tablename);

signals:

};

extern PNDatabaseObjects global_DBObjects;

#endif // PNDATABASEOBJECTS_H
