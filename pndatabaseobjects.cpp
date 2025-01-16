// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pndatabaseobjects.h"
#include "databasestructure.h"

#include <QUuid>
#include <QThread>
//#include <QDebug>

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
    "Completed",
    "Starting"
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
QMutex db_mutex;


PNDatabaseObjects::PNDatabaseObjects(QObject *parent) : QObject(parent)
{
    m_database_file.clear();
}

bool PNDatabaseObjects::createDatabase(QString& t_databasepath)
{
    m_database_file = t_databasepath;

    m_sqlite_db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());

    QFile::remove(m_database_file);  // if it exists remove it.  Dialog should have prompted you.

    m_sqlite_db.setDatabaseName(m_database_file);

    if (!m_sqlite_db.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
            m_sqlite_db.lastError().text(), QMessageBox::Cancel);
        m_database_file.clear(); // set empty if bad file
        return false;
    }

    DatabaseStructure ds;
    ds.CreateDatabase();

    m_sqlite_db.close();

    return true;
}

bool PNDatabaseObjects::openDatabase(QString& databasepath)
{
    m_database_file = databasepath;

    m_sqlite_db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());

    if (QFileInfo::exists(m_database_file))
    {
        m_sqlite_db.setDatabaseName(m_database_file);
    }
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

    DatabaseStructure ds;
    ds.UpgradeDatabase();

    m_clients_model = new ClientsModel(this);
    m_clients_model_proxy = new PNSortFilterProxyModel();
    m_clients_model_proxy->setSourceModel(m_clients_model);

    m_unfilteredclients_model = new ClientsModel(this);
    m_unfilteredclients_model_proxy = new PNSortFilterProxyModel();
    m_unfilteredclients_model_proxy->setSourceModel(m_unfilteredclients_model);
    m_unfilteredclients_model->setShowBlank();
    m_unfilteredclients_model->setNoExport();

    m_people_model = new PeopleModel(this);
    m_people_model_proxy = new PNSortFilterProxyModel();
    m_people_model_proxy->setSourceModel(m_people_model);

    m_company_people_model = new PeopleModel(this);
    m_company_people_model_proxy = new PNSortFilterProxyModel();
    m_company_people_model_proxy->setSourceModel(m_company_people_model);

    m_unfiltered_people_model = new PeopleModel(this);
    m_unfiltered_people_model_proxy = new PNSortFilterProxyModel();
    m_unfiltered_people_model_proxy->setSourceModel(m_unfiltered_people_model);
    m_unfiltered_people_model->setShowBlank();
    m_unfiltered_people_model->setNoExport();

    m_project_information_model = new ProjectsModel(this);
    m_project_information_model_proxy = new PNSortFilterProxyModel();
    m_project_information_model_proxy->setSourceModel(m_project_information_model);

    m_projects_list_model = new ProjectsListModel(this);
    m_projects_list_model_proxy = new PNSortFilterProxyModel();
    m_projects_list_model_proxy->setSourceModel(m_projects_list_model);

    m_teams_model = new TeamsModel(this);
    m_teams_model_proxy = new PNSortFilterProxyModel();
    m_teams_model_proxy->setSourceModel(m_teams_model);

    m_status_report_items_model = new StatusReportItemsModel(this);
    m_status_report_items_model_proxy = new PNSortFilterProxyModel();
    m_status_report_items_model_proxy->setSourceModel(m_status_report_items_model);

    m_project_team_members_model = new ProjectTeamMembersModel(this);
    m_project_team_members_model_proxy = new PNSortFilterProxyModel();
    m_project_team_members_model_proxy->setSourceModel(m_project_team_members_model);

    m_project_locations_model = new ProjectLocationsModel(this);
    m_project_locations_model_proxy = new PNSortFilterProxyModel();
    m_project_locations_model_proxy->setSourceModel(m_project_locations_model);

    m_project_notes_model = new ProjectNotesModel(this);
    m_project_notes_model_proxy = new PNSortFilterProxyModel();
    m_project_notes_model_proxy->setSourceModel(m_project_notes_model);

    m_project_editing_notes_model = new ProjectNotesModel(this);
    m_project_editing_notes_model_proxy = new PNSortFilterProxyModel();
    m_project_editing_notes_model_proxy->setSourceModel(m_project_editing_notes_model);

    m_action_item_project_notes_model = new ActionItemProjectNotesModel(this);
    m_action_item_project_notes_model_proxy = new PNSortFilterProxyModel();
    m_action_item_project_notes_model_proxy->setSourceModel(m_action_item_project_notes_model);

    m_tracker_items_meetings_model = new ActionItemsDetailsMeetingsModel(this);
    m_tracker_items_meetings_model_proxy = new PNSortFilterProxyModel();
    m_tracker_items_meetings_model_proxy->setSourceModel(m_tracker_items_meetings_model);
    m_tracker_items_meetings_model->setShowBlank();
    m_tracker_items_meetings_model->setNoExport();

    m_action_items_details_meetings_model = new ActionItemsDetailsMeetingsModel(this);
    m_action_items_details_meetings_model_proxy = new PNSortFilterProxyModel();
    m_action_items_details_meetings_model_proxy->setSourceModel(m_action_items_details_meetings_model);
    m_action_items_details_meetings_model->setShowBlank();
    m_action_items_details_meetings_model->setNoExport();

    m_project_action_items_model = new TrackerItemsModel(this);
    m_project_action_items_model_proxy = new PNSortFilterProxyModel();
    m_project_action_items_model_proxy->setSourceModel(m_project_action_items_model);

    m_action_item_details_model = new TrackerItemsModel(this);
    m_action_item_details_model_proxy = new PNSortFilterProxyModel();
    m_action_item_details_model_proxy->setSourceModel(m_action_item_details_model);

    m_meeting_attendees_model = new MeetingAttendeesModel(this);
    m_meeting_attendees_model_proxy = new PNSortFilterProxyModel();
    m_meeting_attendees_model_proxy->setSourceModel(m_meeting_attendees_model);

    m_notes_action_items_model = new NotesActionItemsModel(this);
    m_notes_action_items_model_proxy = new PNSortFilterProxyModel();
    m_notes_action_items_model_proxy->setSourceModel(m_notes_action_items_model);

    m_tracker_item_comments_model = new TrackerItemCommentsModel(this);
    m_tracker_item_comments_model_proxy = new PNSortFilterProxyModel();
    m_tracker_item_comments_model_proxy->setSourceModel(m_tracker_item_comments_model);

    m_search_results_model = new SearchResultsModel(this);
    m_search_results_model_proxy = new PNSortFilterProxyModel();
    m_search_results_model_proxy->setSourceModel(m_search_results_model);

    return true;
}

void PNDatabaseObjects::addModel(PNSqlQueryModel* t_model)
{
    QList<PNSqlQueryModel*>::iterator it_recordsets = m_open_recordsets.begin();

    while(it_recordsets != m_open_recordsets.end())
    {

        if (t_model->getOrderKey() < (*it_recordsets)->getOrderKey())
        {
            m_open_recordsets.insert(it_recordsets, t_model);

            return;
        }

        it_recordsets++;
    }

    m_open_recordsets.append(t_model); // add to the list of open recordsets
}

void PNDatabaseObjects::removeModel(PNSqlQueryModel* t_model)
{
    m_open_recordsets.removeAll(t_model);  // remove from the list of open recordsets
}

QString PNDatabaseObjects::execute(const QString& t_sql)
{
    QSqlQuery query(m_sqlite_db);

    DB_LOCK;

    m_sqlite_db.transaction();
    //query.exec(t_sql);
    query.execBatch();

    QSqlError e = query.lastError();
    if (e.isValid())
    {
        m_sqlite_db.rollback();

        qDebug() << "Exec Error:  " << e.text();
        qDebug() << "For SQL:  " << t_sql;
    }
    else
    {
        m_sqlite_db.commit();
    }

    DB_UNLOCK;

    if (query.next())
    {
        return query.value(0).toString();
    }
    else
    {
        return QString();
    }
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
    delete m_project_editing_notes_model;
    delete m_action_item_project_notes_model;
    delete m_action_item_details_model;
    delete m_action_items_details_meetings_model;
    delete m_tracker_items_meetings_model;
    delete m_project_action_items_model;
    delete m_tracker_item_comments_model;
    delete m_meeting_attendees_model;
    delete m_notes_action_items_model;

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
    m_project_editing_notes_model = nullptr;
    m_action_item_project_notes_model = nullptr;
    m_action_item_details_model= nullptr;
    m_action_items_details_meetings_model= nullptr;
    m_tracker_items_meetings_model = nullptr;
    m_project_action_items_model= nullptr;
    m_tracker_item_comments_model = nullptr;
    m_meeting_attendees_model= nullptr;
    m_notes_action_items_model= nullptr;

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
    delete m_project_editing_notes_model_proxy;
    delete m_action_item_project_notes_model_proxy;
    delete m_action_item_details_model_proxy;
    delete m_action_items_details_meetings_model_proxy;
    delete m_tracker_items_meetings_model_proxy;
    delete m_project_action_items_model_proxy;
    delete m_tracker_item_comments_model_proxy;
    delete m_meeting_attendees_model_proxy;
    delete m_notes_action_items_model_proxy;
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
    m_project_editing_notes_model_proxy = nullptr;
    m_action_item_project_notes_model_proxy = nullptr;
    m_action_item_details_model_proxy = nullptr;
    m_action_items_details_meetings_model_proxy = nullptr;
    m_tracker_items_meetings_model_proxy = nullptr;
    m_project_action_items_model_proxy = nullptr;
    m_tracker_item_comments_model_proxy = nullptr;
    m_meeting_attendees_model_proxy = nullptr;
    m_notes_action_items_model_proxy = nullptr;
    m_search_results_model_proxy = nullptr;

    m_sqlite_db.close();

    m_database_file.clear();
}

void PNDatabaseObjects::backupDatabase(const QString& t_file)
{
    QSqlQuery qry(m_sqlite_db);
    qry.prepare( "BEGIN IMMEDIATE;");

    DB_LOCK;
    qry.exec();
    DB_UNLOCK;

    QFile::remove(t_file); // copy command won't overwrite
    if (!QFile::copy(m_database_file, t_file))
    {
        QMessageBox::critical(nullptr, QObject::tr("Database Backup Failed"), QString("Failed to backup the database.") );
    }

    qry.prepare( "ROLLBACK;");

    DB_LOCK;
    qry.exec();
    DB_UNLOCK;
}

bool PNDatabaseObjects::saveParameter( const QString& t_parametername, const QString& t_parametervalue )
{
    QSqlQuery select(m_sqlite_db);
    if(!select.prepare("select parameter_value from application_settings where parameter_name = ?;"))
    {
        QMessageBox::critical(nullptr, QObject::tr("Database Access Failed"), QString("Failed to access a saved setting. You may need to restart Project Notes.\n\nError:\n%1").arg(select.lastError().text()) );
        return false;
    }

    select.bindValue(0, t_parametername);

    if (select.exec())
    {

        if (select.next())
        {
            QSqlQuery update(m_sqlite_db);
            update.prepare("update application_settings set parameter_value = ? where parameter_name = ?;");
            update.bindValue(0, t_parametervalue);
            update.bindValue(1, t_parametername);
            DB_LOCK;
            getDb().transaction();
            if (update.exec())
            {
                getDb().commit();
                DB_UNLOCK;
                return true;
            }
            getDb().rollback();
            DB_UNLOCK;
        }
        else
        {
            QSqlQuery insert(m_sqlite_db);
            insert.prepare("insert into application_settings (parameter_id, parameter_name, parameter_value) values (?, ?, ?);");
            insert.bindValue(0, QUuid::createUuid().toString());
            insert.bindValue(1, t_parametername);
            insert.bindValue(2, t_parametervalue);
            DB_LOCK;
            getDb().transaction();
            if (insert.exec())
            {
                getDb().commit();
                DB_UNLOCK;
                return true;
            }
            getDb().rollback();
            DB_UNLOCK;
        }

    }
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Database Access Failed"), QString("Failed to access a saved setting.  You may need to restart Project Notes.\n\nError:\n%1").arg(select.lastError().text()));
        return false;
    }

    return false;
}

PNSqlQueryModel* PNDatabaseObjects::findOpenTable(const QString& t_tablename)
{
    for ( PNSqlQueryModel* m : getOpenModels())
    {
        if (m->tablename().compare(t_tablename, Qt::CaseInsensitive) == 0)
            return m;
    }

    return nullptr;
}

bool PNDatabaseObjects::refreshDirty()
{
    bool foundsome = false;

    for ( PNSqlQueryModel* m : getOpenModels())
    {
        if (m->isDirty())
        {
            //qDebug() << "Refreshing Dirty Table: " << m->tablename();

            m->refresh();
            foundsome = true;
        }
    }

    return foundsome;
}

QString PNDatabaseObjects::loadParameter( const QVariant& t_parametername )
{
    QSqlQuery select(m_sqlite_db);
    if (!select.prepare("select parameter_value from application_settings where parameter_name = ?"))
    {
        QMessageBox::critical(nullptr, QObject::tr("Database Access Failed"), QString("Failed to access a saved setting. You may need to restart Project Notes.\n\nError:\n%1").arg(select.lastError().text()) );
        return QString();
    }

    select.bindValue(0, t_parametername);

    DB_LOCK;

    if (select.exec())
    {
        if (select.next())
        {
            DB_UNLOCK;
            return select.value(0).toString();
        }
        else
        {
            DB_UNLOCK;
            return QString();
        }
    }
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Database Access Failed"), QString("Failed to access a saved setting. You may need to restart Project Notes.\n\nError:\n%1").arg(select.lastError().text()) );
        DB_UNLOCK;
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
        projectinformationmodel()->clearFilter(14);
        projectslistmodel()->clearFilter(14);
        searchresultsmodel()->clearFilter(6);
    }
    else
    {
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
        actionitemsdetailsmodel()->clearFilter(15);
        searchresultsmodel()->clearFilter(4);
    }
    else
    {
        projectnotesmodel()->setFilter(5, "0");
        actionitemsdetailsmeetingsmodel()->setFilter(3, "0");
        notesactionitemsmodel()->setFilter(15, "0");
        actionitemprojectnotesmodel()->setFilter(3, "0");
        trackeritemsmodel()->setFilter(15, "0");
        actionitemsdetailsmodel()->setFilter(15, "0");
        searchresultsmodel()->setFilter(4, "0");
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
        //trackeritemsmodel()->clearFilter(14);
        // don't clear this one becasue we may have it open  projectinformationmodel()->clearFilter(0);
        projectslistmodel()->clearFilter(0);
        searchresultsmodel()->clearFilter(7);
    }
    else
    {
        //trackeritemsmodel()->setFilter(14, getGlobalProjectFilter());
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

QDomDocument* PNDatabaseObjects::createXMLExportDoc(QList<PNSqlQueryModel*>* t_querymodels)
{
    QDomDocument* doc = new QDomDocument();
    QDomProcessingInstruction xmlDecl = doc->createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc->appendChild(xmlDecl);

    QDomElement root = doc->createElement("projectnotes");
    doc->appendChild(root).toElement();

    root.setAttribute("filepath", getDatabaseFile());
    root.setAttribute("export_date", QDateTime::currentDateTime().toString("MM/dd/yyyy h:m:s ap"));

    QString companyname = execute(QString("select client_name from clients where client_id='%1'").arg(getManagingCompany()));
    QString managername = execute(QString("select name from people where people_id='%1'").arg(getProjectManager()));

    root.setAttribute("project_manager_id", getProjectManager());
    root.setAttribute("managing_company_id", getManagingCompany());
    root.setAttribute("managing_company_name", companyname);
    root.setAttribute("managing_manager_name", managername);

    for (PNSqlQueryModel* m : *t_querymodels)
    {
        QDomElement e = m->toQDomElement(doc);

        root.appendChild(e);
    }

    return doc;
}

QDomDocument* PNDatabaseObjects::createXMLExportDoc(PNSqlQueryModel* t_querymodel, const QString& t_filter)
{
    QDomDocument* doc = new QDomDocument();
    QDomProcessingInstruction xmlDecl = doc->createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc->appendChild(xmlDecl);

    QDomElement root = doc->createElement("projectnotes");
    doc->appendChild(root).toElement();

    root.setAttribute("filepath", getDatabaseFile());
    root.setAttribute("export_date", QDateTime::currentDateTime().toString("MM/dd/yyyy h:m:s ap"));

    QString companyname = execute(QString("select client_name from clients where client_id='%1'").arg(getManagingCompany()));
    QString managername = execute(QString("select name from people where people_id='%1'").arg(getProjectManager()));

    root.setAttribute("project_manager_id", getProjectManager());
    root.setAttribute("managing_company_id", getManagingCompany());
    root.setAttribute("managing_company_name", companyname);
    root.setAttribute("managing_manager_name", managername);

    QDomElement e = t_querymodel->toQDomElement(doc, t_filter);
    root.appendChild(e);

    return doc;
}

QList<QDomNode> PNDatabaseObjects::findTableNodes(const QDomNode& t_xmlelement, const QString& t_tablename)
{
    QList<QDomNode> domlist;

    QDomNode node = t_xmlelement.firstChild();

    while (!node.isNull())
    {
        if (node.nodeName() == "table" && node.toElement().attributeNode("name").value() == t_tablename)
        {
                domlist.append(node);
                //qDebug() << "Found Node: " << node.nodeName() << " Name: " << t_tablename;
        }

        domlist.append(findTableNodes(node, t_tablename));

        node = node.nextSibling();
    }

    return domlist;
}

bool PNDatabaseObjects::importXMLDoc(const QDomDocument& t_xmldoc)
{
    // import clients
    QDomElement root = t_xmldoc.documentElement();
    QList<QDomNode> domlist;

    domlist = findTableNodes(root, "clients");
    if (!domlist.empty())
    {
        ClientsModel clients_model(this);

        for (QDomNode& tablenode : domlist)
            if (!clients_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
        clients_model.refreshByTableName();
    }

    // import people
    domlist = findTableNodes(root, "people");
    if (!domlist.empty())
    {
        PeopleModel people_model(this);

        for (QDomNode& tablenode : domlist)
            if(!people_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
        people_model.refreshByTableName();
    }

    // import projects
    domlist = findTableNodes(root, "projects");
    if (!domlist.empty())
    {
        ProjectsModel projects_model(this);

        for (QDomNode& tablenode : domlist)
            if (!projects_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
        projects_model.refreshByTableName();
    }

    // import project people
    domlist = findTableNodes(root, "project_people");
    if (!domlist.empty())
    {
        ProjectTeamMembersModel project_people_model(this);

        for (QDomNode& tablenode : domlist)
            if (!project_people_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
        project_people_model.refreshByTableName();
    }

    // import status report items
    domlist = findTableNodes(root, "status_report_items");
    if (!domlist.empty())
    {
        StatusReportItemsModel status_report_items_model(this);

        for (QDomNode& tablenode : domlist)
            if(!status_report_items_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
        status_report_items_model.refreshByTableName();
    }

    // import project locations
    domlist = findTableNodes(root, "project_locations");
    if (!domlist.empty())
    {
        ProjectLocationsModel project_locations_model(this);

        for (QDomNode& tablenode : domlist)
            if(!project_locations_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
        project_locations_model.refreshByTableName();
    }


    // import project notes
    domlist = findTableNodes(root, "project_notes");
    if (!domlist.empty())
    {
        ProjectNotesModel project_notes_model(this);

        for (QDomNode& tablenode : domlist)
            if(!project_notes_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
        project_notes_model.refreshByTableName();
    }

    // import item tracker
    domlist = findTableNodes(root, "item_tracker");
    if (!domlist.empty())
    {
        TrackerItemsModel tracker_items_model(this);

        for (QDomNode& tablenode : domlist)
            if(!tracker_items_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
        tracker_items_model.refreshByTableName();
    }

    // import meeting attendees
    domlist = findTableNodes(root, "meeting_attendees");
    if (!domlist.empty())
    {
        MeetingAttendeesModel meeting_attendees_model(this);

        for (QDomNode& tablenode : domlist)
            if(!meeting_attendees_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
        meeting_attendees_model.refreshByTableName();
    }

    // import tracker items updates
    domlist = findTableNodes(root, "item_tracker_updates");
    if (!domlist.empty())
    {
        TrackerItemCommentsModel item_tracker_updates_model(this);

        for (QDomNode& tablenode : domlist)
            if(!item_tracker_updates_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
        item_tracker_updates_model.refreshByTableName();
    }

    refreshDirty();

    return true;
}

QList<PNSqlQueryModel*>* PNDatabaseObjects::getData(const QDomDocument& t_xmldoc)
{
    QList<PNSqlQueryModel*> *model_list = new QList<PNSqlQueryModel*>();

    QDomElement root = t_xmldoc.documentElement();
    QList<QDomNode> domlist;

    // import clients
    domlist = findTableNodes(root, "clients");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            PNSqlQueryModel* model = new ClientsModel(this, false);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();
    }

    // import people
    domlist = findTableNodes(root, "people");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            PNSqlQueryModel* model = new PeopleModel(this, false);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();
    }

    // import projects
    domlist = findTableNodes(root, "projects");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            PNSqlQueryModel* model = new ProjectsModel(this, false);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();

    }

    // import project people
    domlist = findTableNodes(root, "project_people");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            PNSqlQueryModel* model = new ProjectTeamMembersModel(this, false);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();
    }

    // import status report items
    domlist = findTableNodes(root, "status_report_items");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            PNSqlQueryModel* model = new StatusReportItemsModel(this, false);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();
    }

    // import project locations
    domlist = findTableNodes(root, "project_locations");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            PNSqlQueryModel* model = new ProjectLocationsModel(this, false);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();
    }


    // import project notes
    domlist = findTableNodes(root, "project_notes");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            PNSqlQueryModel* model = new ProjectNotesModel(this, false);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();
    }

    // import item tracker
    domlist = findTableNodes(root, "item_tracker");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            PNSqlQueryModel* model = new TrackerItemsModel(this, false);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();
    }

    // import meeting attendees
    domlist = findTableNodes(root, "meeting_attendees");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            PNSqlQueryModel* model = new MeetingAttendeesModel(this, false);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();
    }

    // import tracker items updates
    domlist = findTableNodes(root, "item_tracker_updates");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            PNSqlQueryModel* model = new TrackerItemCommentsModel(this, false);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();
    }

    return model_list;
}

void PNDatabaseObjects::addDefaultPMToProject(const QString& t_project_id)
{
    QString pm = getProjectManager();
    QString guid = QUuid::createUuid().toString();

    QString insert = QString("insert into project_people (teammember_id, people_id, project_id, role) select '%3', '%2', '%1', 'Project Manager' where not exists (select 1 from project_people where project_id = '%1' and people_id = '%2' )").arg(t_project_id).arg(pm).arg(guid);
    //qDebug() << "Adding default pm to project: " << insert;

    execute(insert);
}

void PNDatabaseObjects::addDefaultPMToMeeting(const QString& t_note_id)
{
    QString pm = getProjectManager();
    QString guid = QUuid::createUuid().toString();
    QString guid2 = QUuid::createUuid().toString();

    QString project_id = execute(QString("select project_id from project_notes where note_id='%1'").arg(t_note_id));
    QString insertpm = QString("insert into project_people (teammember_id, people_id, project_id, role) select '%3', '%2', '%1', 'Project Manager' where not exists (select 1 from project_people where project_id = '%1' and people_id = '%2' )").arg(project_id).arg(pm).arg(guid2);
    //qDebug() << "Adding default pm to project: " << insertpm;

    execute(insertpm);

    QString insert = QString("insert into meeting_attendees (attendee_id, person_id, note_id) select '%3', '%2', '%1' where not exists (select 1 from meeting_attendees where note_id = '%1' and person_id = '%2' )").arg(t_note_id).arg(pm).arg(guid);
    //qDebug() << "Adding default pm to meeting: " << insert;

    meetingattendeesmodel()->setDirty();
    teamsmodel()->setDirty();
    projectteammembersmodel()->setDirty();

    execute(insert);
}

// TODO: you can remove someone from a team by just reselecting a new person

