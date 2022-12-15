#include "pndatabaseobjects.h"

#include <QUuid>
#include <QDebug>

// selection values for fields
QStringList PNDatabaseObjects::item_type = {
    "Tracker",
    "Action"
};

QStringList PNDatabaseObjects::item_status = {
    "Assigned",
    "Defered",
    "New",
    "Resolved",
    "Cancelled"
};

QStringList PNDatabaseObjects::item_priority = {
    "Low",
    "Medium",
    "High"
};

QStringList PNDatabaseObjects::project_status = {
    "Active",
    "Closed"
};

QStringList PNDatabaseObjects::status_item_status = {
    "In Progress",
    "Completed"
};

QStringList PNDatabaseObjects::invoicing_period = {
    "Monthly",
    "Milestone",
    "Complete"
};

QStringList PNDatabaseObjects::status_report_period = {
    "Bi-Weekly",
    "Monthly",
    "Weekly",
    "None"
};

QStringList PNDatabaseObjects::file_types = {
    "File Folder",
    "Web Link",
    "Microsoft Project",
    "Word Document",
    "Excel Document",
    "PowerPoint Document",
    "PDF File",
    "Generic File (System Identified)"
};

PNDatabaseObjects global_DBObjects(nullptr);

PNDatabaseObjects::PNDatabaseObjects(QObject *parent) : QObject(parent)
{
    m_database_file.clear();
}

bool PNDatabaseObjects::openDatabase(QString& databasepath)
{
    m_database_file = databasepath;

    m_sqlite_db = QSqlDatabase::addDatabase("QSQLITE");

    if (QFileInfo::exists(m_database_file))
        m_sqlite_db.setDatabaseName(m_database_file);
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
            QString(tr("File %1 does not exist.")).arg(m_database_file), QMessageBox::Cancel);
        m_database_file.clear(); // set empty if bad file
        return false;
    }

    if (!m_sqlite_db.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
            m_sqlite_db.lastError().text(), QMessageBox::Cancel);
        m_database_file.clear(); // set empty if bad file
        return false;
    }

    m_clients_model = new ClientsModel(nullptr);
    m_clients_model_proxy = new PNSortFilterProxyModel();
    m_clients_model_proxy->setSourceModel(m_clients_model);

    m_unfilteredclients_model = new ClientsModel(nullptr);
    m_unfilteredclients_model_proxy = new PNSortFilterProxyModel();
    m_unfilteredclients_model_proxy->setSourceModel(m_unfilteredclients_model);
    m_unfilteredclients_model->setShowBlank();

    m_people_model = new PeopleModel(nullptr);
    // setup lookup/drop down values
    m_people_model->setLookup(5, m_unfilteredclients_model, 0, 1);
    m_people_model_proxy = new PNSortFilterProxyModel();
    m_people_model_proxy->setSourceModel(m_people_model);

    m_company_people_model = new PeopleModel(nullptr);
    m_company_people_model_proxy = new PNSortFilterProxyModel();
    m_company_people_model_proxy->setSourceModel(m_company_people_model);

    m_unfiltered_people_model = new PeopleModel(nullptr);
    m_unfiltered_people_model_proxy = new PNSortFilterProxyModel();
    m_unfiltered_people_model_proxy->setSourceModel(m_unfiltered_people_model);
    m_unfiltered_people_model->setShowBlank();

    m_project_information_model = new ProjectsModel(nullptr);
    m_project_information_model_proxy = new PNSortFilterProxyModel();
    m_project_information_model_proxy->setSourceModel(m_project_information_model);

    m_projects_list_model = new ProjectsListModel(nullptr);
    // setup lookup/drop down t_value
    m_projects_list_model->setLookup(5, m_unfiltered_people_model, 0, 1);
    m_projects_list_model->setLookup(13, m_unfilteredclients_model, 0, 1);
    m_projects_list_model_proxy = new PNSortFilterProxyModel();
    m_projects_list_model_proxy->setSourceModel(m_projects_list_model);

    m_teams_model = new TeamsModel(nullptr);
    m_teams_model_proxy = new PNSortFilterProxyModel();
    m_teams_model_proxy->setSourceModel(m_teams_model);

    m_status_report_items_model = new StatusReportItemsModel(nullptr);
    m_status_report_items_model_proxy = new PNSortFilterProxyModel();
    m_status_report_items_model_proxy->setSourceModel(m_status_report_items_model);

    m_project_team_members_model = new ProjectTeamMembersModel(nullptr);
    m_project_team_members_model_proxy = new PNSortFilterProxyModel();
    m_project_team_members_model_proxy->setSourceModel(m_project_team_members_model);

    m_project_locations_model = new ProjectLocationsModel(nullptr);
    m_project_locations_model_proxy = new PNSortFilterProxyModel();
    m_project_locations_model_proxy->setSourceModel(m_project_locations_model);

    m_project_notes_model = new ProjectNotesModel(nullptr);
    m_project_notes_model_proxy = new PNSortFilterProxyModel();
    m_project_notes_model_proxy->setSourceModel(m_project_notes_model);

    m_project_editing_notes_model = new ProjectNotesModel(nullptr);
    m_project_editing_notes_model_proxy = new PNSortFilterProxyModel();
    m_project_editing_notes_model_proxy->setSourceModel(m_project_editing_notes_model);

    m_action_item_project_notes_model = new ActionItemProjectNotesModel(nullptr);
    m_action_item_project_notes_model_proxy = new PNSortFilterProxyModel();
    m_action_item_project_notes_model_proxy->setSourceModel(m_action_item_project_notes_model);

    m_tracker_items_meetings_model = new ActionItemsDetailsMeetingsModel(nullptr);
    m_tracker_items_meetings_model_proxy = new PNSortFilterProxyModel();
    m_tracker_items_meetings_model_proxy->setSourceModel(m_tracker_items_meetings_model);
    m_tracker_items_meetings_model->setShowBlank();

    m_action_items_details_meetings_model = new ActionItemsDetailsMeetingsModel(nullptr);
    m_action_items_details_meetings_model_proxy = new PNSortFilterProxyModel();
    m_action_items_details_meetings_model_proxy->setSourceModel(m_action_items_details_meetings_model);
    m_action_items_details_meetings_model->setShowBlank();

    m_project_action_items_model = new TrackerItemsModel(nullptr);
    m_project_action_items_model_proxy = new PNSortFilterProxyModel();
    m_project_action_items_model_proxy->setSourceModel(m_project_action_items_model);

    m_action_item_details_model = new TrackerItemsModel(nullptr);
    m_action_item_details_model_proxy = new PNSortFilterProxyModel();
    m_action_item_details_model_proxy->setSourceModel(m_action_item_details_model);

    m_meeting_attendees_model = new MeetingAttendeesModel(nullptr);
    m_meeting_attendees_model_proxy = new PNSortFilterProxyModel();
    m_meeting_attendees_model_proxy->setSourceModel(m_meeting_attendees_model);

    m_notes_action_items_model = new NotesActionItemsModel(nullptr);
    m_notes_action_items_model_proxy = new PNSortFilterProxyModel();
    m_notes_action_items_model_proxy->setSourceModel(m_notes_action_items_model);

    m_tracker_item_comments_model = new TrackerItemCommentsModel(nullptr);
    m_tracker_item_comments_model_proxy = new PNSortFilterProxyModel();
    m_tracker_item_comments_model_proxy->setSourceModel(m_tracker_item_comments_model);

    m_search_results_model = new SearchResultsModel(nullptr);
/*
stopped here create doc funcion
    QDomDocument doc;
    QDomElement root = doc.createElement("projectnotes");
    doc.appendChild(root).toElement();

    root.setAttribute("filepath", global_DBObjects.getDatabaseFile());
    root.setAttribute("export_date", QDateTime::currentDateTime().toString("MM/dd/yyyy h:m:s ap"));
    //root.setAttribute("filter_field", fkfield);

    QString companyname = global_DBObjects.execute(QString("select client_name from clients where client_id='%1'").arg(global_DBObjects.getManagingCompany()));
    QString managername = global_DBObjects.execute(QString("select name from people where people_id='%1'").arg(global_DBObjects.getProjectManager()));

    root.setAttribute("project_manager_id", global_DBObjects.getProjectManager());
    root.setAttribute("managing_company_id", global_DBObjects.getManagingCompany());
    root.setAttribute("managing_company_name", companyname);
    root.setAttribute("managing_manager_name", managername);

    m_projects_list_model->refresh();
    QDomElement e = m_projects_list_model->toQDomElement(doc);
    root.appendChild(e);

    qDebug() << doc.toString();

*/
    m_search_results_model_proxy = new PNSortFilterProxyModel();
    m_search_results_model_proxy->setSourceModel(m_search_results_model);

    return true;
}

QString PNDatabaseObjects::execute(const QString& t_sql)
{
    QSqlQuery query;
    query.exec(t_sql);

    if (query.next())
        return query.value(0).toString();
    else
        return QString();
}

void PNDatabaseObjects::closeDatabase()
{
    delete m_clients_model;
    delete m_unfilteredclients_model;
    delete m_people_model;
    delete m_company_people_model;
    delete m_unfiltered_people_model;
    delete m_project_information_model;
    delete m_projects_list_model;
    delete m_teams_model;
    delete m_status_report_items_model;
    delete m_project_team_members_model;
    delete m_project_locations_model;
    delete m_project_notes_model;
    delete m_action_item_project_notes_model;
    delete m_action_item_details_model;
    delete m_action_items_details_meetings_model;
    delete m_tracker_items_meetings_model;
    delete m_project_action_items_model;
    delete m_tracker_item_comments_model;
    delete m_meeting_attendees_model;
    delete m_notes_action_items_model;
    delete m_item_detail_team_list_model;

    delete m_search_results_model;

    m_clients_model= nullptr;
    m_unfilteredclients_model= nullptr;
    m_people_model= nullptr;
    m_company_people_model= nullptr;
    m_unfiltered_people_model= nullptr;
    m_project_information_model= nullptr;
    m_projects_list_model= nullptr;
    m_teams_model= nullptr;
    m_status_report_items_model= nullptr;
    m_project_team_members_model= nullptr;
    m_project_locations_model= nullptr;
    m_project_notes_model= nullptr;
    m_action_item_project_notes_model = nullptr;
    m_action_item_details_model= nullptr;
    m_action_items_details_meetings_model= nullptr;
    m_tracker_items_meetings_model = nullptr;
    m_project_action_items_model= nullptr;
    m_tracker_item_comments_model = nullptr;
    m_meeting_attendees_model= nullptr;
    m_notes_action_items_model= nullptr;
    m_item_detail_team_list_model = nullptr;

    m_search_results_model = nullptr;

    delete m_clients_model_proxy;
    delete m_unfilteredclients_model_proxy;
    delete m_people_model_proxy;
    delete m_company_people_model_proxy;
    delete m_unfiltered_people_model_proxy;
    delete m_project_information_model_proxy;
    delete m_projects_list_model_proxy;
    delete m_teams_model_proxy;
    delete m_status_report_items_model_proxy;
    delete m_project_team_members_model_proxy;
    delete m_project_locations_model_proxy;
    delete m_project_notes_model_proxy;
    delete m_action_item_project_notes_model;
    delete m_action_item_details_model_proxy;
    delete m_action_items_details_meetings_model_proxy;
    delete m_tracker_items_meetings_model_proxy;
    delete m_project_action_items_model_proxy;
    delete m_tracker_item_comments_model_proxy;
    delete m_meeting_attendees_model_proxy;
    delete m_notes_action_items_model_proxy;
    delete m_item_detail_team_list_model_proxy;
    delete m_search_results_model_proxy;

    m_clients_model_proxy = nullptr;
    m_unfilteredclients_model_proxy = nullptr;
    m_people_model_proxy = nullptr;
    m_company_people_model_proxy = nullptr;
    m_unfiltered_people_model_proxy = nullptr;
    m_project_information_model_proxy = nullptr;
    m_projects_list_model_proxy = nullptr;
    m_teams_model_proxy = nullptr;
    m_status_report_items_model_proxy = nullptr;
    m_project_team_members_model_proxy = nullptr;
    m_project_locations_model_proxy = nullptr;
    m_project_notes_model_proxy = nullptr;
    m_action_item_project_notes_model = nullptr;
    m_action_item_details_model_proxy = nullptr;
    m_action_items_details_meetings_model_proxy = nullptr;
    m_tracker_items_meetings_model_proxy = nullptr;
    m_project_action_items_model_proxy = nullptr;
    m_tracker_item_comments_model_proxy = nullptr;
    m_meeting_attendees_model_proxy = nullptr;
    m_notes_action_items_model_proxy = nullptr;
    m_item_detail_team_list_model = nullptr;
    m_search_results_model_proxy = nullptr;

    m_sqlite_db.close();
    m_database_file.clear();
}

void PNDatabaseObjects::backupDatabase(QWidget& /*t_parent*/, QFileInfo& /*t_file*/)
{
    // TODO:  This may not make sense to do when using the QT SQL interface
}

bool PNDatabaseObjects::saveParameter( const QString& t_parametername, const QString& t_parametervalue )
{
    QSqlQuery select("select parameter_value from application_settings where parameter_name = ?;");
    select.bindValue(0, t_parametername);
    if (select.exec())
    {
        if (select.next())
        {
            QSqlQuery update("update application_settings set parameter_value = ? where parameter_name = ?;");
            update.bindValue(0, t_parametervalue);
            update.bindValue(1, t_parametername);
            if (update.exec())
                return true;
        }
        else
        {
            QSqlQuery insert("insert into application_settings (parameter_id, parameter_name, parameter_value) values (?, ?, ?);");
            insert.bindValue(0, QUuid::createUuid().toString());
            insert.bindValue(1, t_parametername);
            insert.bindValue(2, t_parametervalue);
            if (insert.exec())
                return true;
        }

    }
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Database Access Failed"), "Failed to access a saved setting.  You may need to restart Project Notes.");
        return false;
    }

    return false;
}

QString PNDatabaseObjects::loadParameter( const QString& t_parametername )
{
    QSqlQuery select("select parameter_value from application_settings where parameter_name = ?;");
    select.bindValue(0, t_parametername);
    if (select.exec())
    {
        if (select.next())
            return select.value(0).toString();
        else
            return QString();
    }
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Database Access Failed"), "Failed to access a saved setting.  You may need to restart Project Notes.");
        return QString();
    }
}

void PNDatabaseObjects::setShowResolvedTrackerItems(bool t_value)
{
    saveParameter("UserFilter:ShowResolvedTrackerItems", (t_value ? "1": "0"));
}

bool PNDatabaseObjects::getShowResolvedTrackerItems()
{
    QString t_value = loadParameter("UserFilter:ShowResolvedTrackerItems");
    bool ret = (bool)t_value.toUInt();
    return ret;
}

void PNDatabaseObjects::setShowClosedProjects(bool t_value)
{
    saveParameter("UserFilter:ShowClosedProjects", (t_value ? "1": "0"));
}

bool PNDatabaseObjects::getShowClosedProjects()
{
    QString t_value = loadParameter("UserFilter:ShowClosedProjects");
    bool ret = (bool)t_value.toUInt();
    return ret;
}

void PNDatabaseObjects::setShowInternalItems(bool t_value)
{
    saveParameter("UserFilter:ShowInternalItems", (t_value ? "1": "0"));
}

bool PNDatabaseObjects::getShowInternalItems()
{
    QString t_value = loadParameter("UserFilter:ShowInternalItems");
    bool ret = (bool)t_value.toUInt();
    return ret;
}

void PNDatabaseObjects::setGlobalClientFilter(QString t_value)
{
    saveParameter("UserFilter:ClientFilter", t_value );
}

QString PNDatabaseObjects::getGlobalClientFilter()
{
    return loadParameter("UserFilter:ClientFilter");
}

void PNDatabaseObjects::setGlobalProjectFilter(QString t_value)
{
    saveParameter("UserFilter:ProjectFilter", t_value );
}

QString PNDatabaseObjects::getGlobalProjectFilter()
{
    return loadParameter("UserFilter:ProjectFilter");
}

void PNDatabaseObjects::setProjectManager(QString t_value)
{
    saveParameter("Preferences:ProjectManager", t_value );
}

QString PNDatabaseObjects::getProjectManager()
{
    return loadParameter("Preferences:ProjectManager");
}

void PNDatabaseObjects::setManagingCompany(QString t_value)
{
    saveParameter("Preferences:ManagingCompany", t_value );
}

QString PNDatabaseObjects::getManagingCompany()
{
    return loadParameter("Preferences:ManagingCompany");
}

void PNDatabaseObjects::setGlobalSearches( bool t_refresh )
{ 
    // setup default filters
    if (getShowClosedProjects())
    {
        //trackeritemsmodel()->clearFilter(17);
        projectinformationmodel()->clearFilter(14);
        projectslistmodel()->clearFilter(14);
        searchresultsmodel()->clearFilter(6);
    }
    else
    {
        //trackeritemsmodel()->setFilter(17, tr("Active"));  // IT WORKS NOW but do we need it since we don't show all items in this version
        projectinformationmodel()->setFilter(14, tr("Active"));
        projectslistmodel()->setFilter(14, tr("Active"));
        searchresultsmodel()->setFilter(6, tr("Active"));
    }

    if (getShowInternalItems())
    {
        projectnotesmodel()->clearFilter(5);
        actionitemsdetailsmeetingsmodel()->clearFilter(3);
        notesactionitemsmodel()->clearFilter(15);
        actionitemprojectnotesmodel()->clearFilter(3);
        trackeritemsmodel()->clearFilter(15);
        actionitemsdetailsmodel()->clearFilter(3);
        searchresultsmodel()->clearFilter(4);
    }
    else
    {
        projectnotesmodel()->setFilter(5, tr("0"));
        actionitemsdetailsmeetingsmodel()->setFilter(3, tr("0"));
        notesactionitemsmodel()->setFilter(15, tr("0"));
        actionitemprojectnotesmodel()->setFilter(3, tr("0"));
        trackeritemsmodel()->setFilter(15, tr("0"));
        actionitemsdetailsmodel()->setFilter(3, tr("0"));
        searchresultsmodel()->setFilter(4, tr("0"));
    }

    if (getShowResolvedTrackerItems())
    {
        trackeritemsmodel()->clearFilter(9);
    }
    else
    {
        trackeritemsmodel()->setFilter(9, "Resolved", PNSqlQueryModel::NotEqual);
    }

    if (getGlobalClientFilter().isEmpty())
    {
        peoplemodel()->clearUserFilter(5);
        peoplemodel()->deactivateUserFilter(tr("GlobalClientFilter"));
        clientsmodel()->clearFilter(0);
        clientsmodel()->deactivateUserFilter(tr("GlobalClientFilter"));
        trackeritemsmodel()->clearFilter(18);
        projectinformationmodel()->clearFilter(13);
        projectslistmodel()->clearFilter(13);
        searchresultsmodel()->clearFilter(5);
    }
    else
    {
        //QString managing = Execute(QString("select client_name from clients where client_id = '%1'").arg(GetManagingCompany()));
        //QString filtered = Execute(QString("select client_name from clients where client_id = '%1'").arg(GetGlobalClientFilter()));

        QVariantList managingnclientids;
        // make sure list of people can show the managing company
        managingnclientids.append(getManagingCompany());
        managingnclientids.append(getGlobalClientFilter());

        clientsmodel()->setUserFilter(0, managingnclientids );
        clientsmodel()->activateUserFilter("GlobalClientFilter");

        peoplemodel()->setUserFilter(5, managingnclientids);
        peoplemodel()->activateUserFilter(tr("GlobalClientFilter"));

        trackeritemsmodel()->setFilter(18, getGlobalClientFilter());
        projectinformationmodel()->setFilter(13, getGlobalClientFilter());
        projectslistmodel()->setFilter(13, getGlobalClientFilter());
        searchresultsmodel()->setFilter(5, getGlobalClientFilter());
    }

    if (getGlobalProjectFilter().isEmpty())
    {
        trackeritemsmodel()->clearFilter(14);
        // don't clear this one becasue we may have it open  projectinformationmodel()->clearFilter(0);
        projectslistmodel()->clearFilter(0);
        searchresultsmodel()->clearFilter(7);
    }
    else
    {
        trackeritemsmodel()->setFilter(14, getGlobalProjectFilter());
        // don't set this one only do the lists projectinformationmodel()->setFilter(0, getGlobalProjectFilter());
        projectslistmodel()->setFilter(0, getGlobalProjectFilter());

        QString projectnumber = execute(QString("select project_number from projects where project_id = '%1'").arg(getGlobalProjectFilter()));

        searchresultsmodel()->setFilter(5, projectnumber);
    }


    if (t_refresh)
    {
        peoplemodel()->refresh();
        clientsmodel()->refresh();
        projectnotesmodel()->refresh();
        actionitemsdetailsmeetingsmodel()->refresh();
        notesactionitemsmodel()->refresh();
        actionitemprojectnotesmodel()->refresh();
        actionitemsdetailsmodel()->refresh();

        trackeritemsmodel()->refresh();
        projectinformationmodel()->refresh();
        projectslistmodel()->refresh();
        searchresultsmodel()->refresh();
    }
}

bool PNDatabaseObjects::executeDDL(const QString& /*t_sql*/)
{
    // TODO : finish

    return false;
}


