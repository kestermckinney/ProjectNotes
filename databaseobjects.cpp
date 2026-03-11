// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseobjects.h"
#include "databasestructure.h"

#include <QUuid>
#include <QThread>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


// selection values for fields
QStringList DatabaseObjects::item_type = {
    "Tracker",
    "Action"
};

QStringList DatabaseObjects::item_status = {
    "Assigned",
    "Defered",
    "New",
    "Resolved",
    "Cancelled"
};

QStringList DatabaseObjects::item_priority = {
    "Low",
    "Medium",
    "High"
};

QStringList DatabaseObjects::project_status = {
    "Active",
    "Closed"
};

QStringList DatabaseObjects::status_item_status = {
    "In Progress",
    "Completed",
    "Starting"
};

QStringList DatabaseObjects::invoicing_period = {
    "Monthly",
    "Milestone",
    "Complete"
};

QStringList DatabaseObjects::status_report_period = {
    "Bi-Weekly",
    "Monthly",
    "Weekly",
    "None"
};

QStringList DatabaseObjects::file_types = {
    "File Folder",
    "Web Link",
    "Microsoft Project",
    "Word Document",
    "Excel Document",
    "PowerPoint Document",
    "PDF File",
    "Generic File (System Identified)"
};

DatabaseObjects global_DBObjects(nullptr);
QMutex db_mutex;


DatabaseObjects::DatabaseObjects(QObject *parent) : QObject(parent)
{
    m_databaseFile.clear();
}

bool DatabaseObjects::createDatabase(QString& databasepath)
{
    m_databaseFile = databasepath;

    m_sqliteDb = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());

    QFile::remove(m_databaseFile);  // if it exists remove it.  Dialog should have prompted you.

    m_sqliteDb.setDatabaseName(m_databaseFile);

    if (!m_sqliteDb.open())
    {
        if (m_gui)
            QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
                m_sqliteDb.lastError().text(), QMessageBox::Cancel);

        m_databaseFile.clear(); // set empty if bad file
        return false;
    }

    DatabaseStructure ds;
    ds.CreateDatabase();

    m_sqliteDb.close();

    return true;
}

bool DatabaseObjects::openDatabase(const QString& databasepath, const QString& connectionname, bool gui)
{
    m_databaseFile = databasepath;
    m_gui = gui;

    if (QSqlDatabase::contains(connectionname))
        m_sqliteDb = QSqlDatabase::database(connectionname);
    else
        m_sqliteDb = QSqlDatabase::addDatabase("QSQLITE", connectionname);

    if (QFileInfo::exists(m_databaseFile))
    {
        m_sqliteDb.setDatabaseName(m_databaseFile);
    }
    else
    {
        if (m_gui)
            QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
                              QString(tr("File %1 does not exist.")).arg(m_databaseFile), QMessageBox::Cancel);

        m_databaseFile.clear(); // set empty if bad file
        return false;
    }

    if (!m_sqliteDb.open())
    {
        if (m_gui)
            QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
                                  m_sqliteDb.lastError().text(), QMessageBox::Cancel);

        m_databaseFile.clear(); // set empty if bad file
        return false;
    }

    m_sqliteDb.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    m_sqliteDb.setConnectOptions("QSQLITE_BUSY_TIMEOUT=400");

    if (m_gui)
    {
        DatabaseStructure ds;
        ds.UpgradeDatabase();
    }

    m_clientsModel = new ClientsModel(this);
    m_clientsModelProxy = new SortFilterProxyModel();
    m_clientsModelProxy->setSourceModel(m_clientsModel);

    m_unfilteredclientsModel = new ClientsModel(this);
    m_unfilteredclientsModelProxy = new SortFilterProxyModel();
    m_unfilteredclientsModelProxy->setSourceModel(m_unfilteredclientsModel);
    m_unfilteredclientsModel->setShowBlank();
    m_unfilteredclientsModel->setNoExport();

    m_peopleModel = new PeopleModel(this);
    m_peopleModelProxy = new SortFilterProxyModel();
    m_peopleModelProxy->setSourceModel(m_peopleModel);

    m_companyPeopleModel = new PeopleModel(this);
    m_companyPeopleModelProxy = new SortFilterProxyModel();
    m_companyPeopleModelProxy->setSourceModel(m_companyPeopleModel);

    m_unfilteredPeopleModel = new PeopleModel(this);
    m_unfilteredPeopleModelProxy = new SortFilterProxyModel();
    m_unfilteredPeopleModelProxy->setSourceModel(m_unfilteredPeopleModel);
    m_unfilteredPeopleModel->setShowBlank();
    m_unfilteredPeopleModel->setNoExport();

    m_projectInformationModel = new ProjectsModel(this);
    m_projectInformationModelProxy = new SortFilterProxyModel();
    m_projectInformationModelProxy->setSourceModel(m_projectInformationModel);

    m_projectsListModel = new ProjectsListModel(this);
    m_projectsListModelProxy = new SortFilterProxyModel();
    m_projectsListModelProxy->setSourceModel(m_projectsListModel);

    m_teamsModel = new TeamsModel(this);
    m_teamsModelProxy = new SortFilterProxyModel();
    m_teamsModelProxy->setSourceModel(m_teamsModel);

    m_statusReportItemsModel = new StatusReportItemsModel(this);
    m_statusReportItemsModelProxy = new SortFilterProxyModel();
    m_statusReportItemsModelProxy->setSourceModel(m_statusReportItemsModel);

    m_projectTeamMembersModel = new ProjectTeamMembersModel(this);
    m_projectTeamMembersModelProxy = new SortFilterProxyModel();
    m_projectTeamMembersModelProxy->setSourceModel(m_projectTeamMembersModel);

    m_projectLocationsModel = new ProjectLocationsModel(this);
    m_projectLocationsModelProxy = new SortFilterProxyModel();
    m_projectLocationsModelProxy->setSourceModel(m_projectLocationsModel);

    m_projectNotesModel = new ProjectNotesModel(this);
    m_projectNotesModelProxy = new SortFilterProxyModel();
    m_projectNotesModelProxy->setSourceModel(m_projectNotesModel);

    m_projectEditingNotesModel = new ProjectNotesModel(this);
    m_projectEditingNotesModelProxy = new SortFilterProxyModel();
    m_projectEditingNotesModelProxy->setSourceModel(m_projectEditingNotesModel);

    m_actionItemProjectNotesModel = new ActionItemProjectNotesModel(this);
    m_actionItemProjectNotesModelProxy = new SortFilterProxyModel();
    m_actionItemProjectNotesModelProxy->setSourceModel(m_actionItemProjectNotesModel);

    m_trackerItemsMeetingsModel = new ActionItemsDetailsMeetingsModel(this);
    m_trackerItemsMeetingsModelProxy = new SortFilterProxyModel();
    m_trackerItemsMeetingsModelProxy->setSourceModel(m_trackerItemsMeetingsModel);
    m_trackerItemsMeetingsModel->setShowBlank();
    m_trackerItemsMeetingsModel->setNoExport();

    m_actionItemsDetailsMeetingsModel = new ActionItemsDetailsMeetingsModel(this);
    m_actionItemsDetailsMeetingsModelProxy = new SortFilterProxyModel();
    m_actionItemsDetailsMeetingsModelProxy->setSourceModel(m_actionItemsDetailsMeetingsModel);
    m_actionItemsDetailsMeetingsModel->setShowBlank();
    m_actionItemsDetailsMeetingsModel->setNoExport();

    m_projectActionItemsModel = new TrackerItemsModel(this);
    m_projectActionItemsModelProxy = new SortFilterProxyModel();
    m_projectActionItemsModelProxy->setSourceModel(m_projectActionItemsModel);

    m_actionItemDetailsModel = new TrackerItemsModel(this);
    m_actionItemDetailsModelProxy = new SortFilterProxyModel();
    m_actionItemDetailsModelProxy->setSourceModel(m_actionItemDetailsModel);

    m_meetingAttendeesModel = new MeetingAttendeesModel(this);
    m_meetingAttendeesModelProxy = new SortFilterProxyModel();
    m_meetingAttendeesModelProxy->setSourceModel(m_meetingAttendeesModel);

    m_notesActionItemsModel = new NotesActionItemsModel(this);
    m_notesActionItemsModelProxy = new SortFilterProxyModel();
    m_notesActionItemsModelProxy->setSourceModel(m_notesActionItemsModel);

    m_trackerItemCommentsModel = new TrackerItemCommentsModel(this);
    m_trackerItemCommentsModelProxy = new SortFilterProxyModel();
    m_trackerItemCommentsModelProxy->setSourceModel(m_trackerItemCommentsModel);

    m_searchResultsModel = new SearchResultsModel(this);
    m_searchResultsModelProxy = new SortFilterProxyModel();
    m_searchResultsModelProxy->setSourceModel(m_searchResultsModel);

    return true;
}

void DatabaseObjects::addModel(SqlQueryModel* model)
{
    QList<SqlQueryModel*>::iterator it_recordsets = m_openRecordsets.begin();

    while(it_recordsets != m_openRecordsets.end())
    {

        if (model->getOrderKey() < (*it_recordsets)->getOrderKey())
        {
            m_openRecordsets.insert(it_recordsets, model);

            return;
        }

        it_recordsets++;
    }

    m_openRecordsets.append(model); // add to the list of open recordsets
}

void DatabaseObjects::removeModel(SqlQueryModel* model)
{
    m_openRecordsets.removeAll(model);  // remove from the list of open recordsets
}

SqlQueryModel* DatabaseObjects::createExportObject(const QString& tableName)
{
    if (tableName.compare("clients", Qt::CaseInsensitive) == 0)
        return new ClientsModel(this);
    else if (tableName.compare("people", Qt::CaseInsensitive) == 0)
        return new PeopleModel(this);
    else if (tableName.compare("projects", Qt::CaseInsensitive) == 0)
        return new ProjectsModel(this);
    else if (tableName.compare("status_report_items", Qt::CaseInsensitive) == 0)
        return new StatusReportItemsModel(this);
    else if (tableName.compare("project_people", Qt::CaseInsensitive) == 0)
        return new ProjectTeamMembersModel(this); // use this one because it hase more related fields for exporting
    else if (tableName.compare("project_locations", Qt::CaseInsensitive) == 0)
        return new ProjectLocationsModel(this);
    else if (tableName.compare("project_notes", Qt::CaseInsensitive) == 0)
        return new ProjectNotesModel(this); // has id and and name of project
    else if (tableName.compare("item_tracker", Qt::CaseInsensitive) == 0)
        return new TrackerItemsModel(this);
    else if (tableName.compare("meeting_attendees", Qt::CaseInsensitive) == 0)
        return new MeetingAttendeesModel(this);
    else if (tableName.compare("item_tracker", Qt::CaseInsensitive) == 0)
        return new NotesActionItemsModel(this);
    else if (tableName.compare("item_tracker_updates", Qt::CaseInsensitive) == 0)
        return new TrackerItemCommentsModel(this);
    else
        return nullptr;
}

QString DatabaseObjects::execute(const QString& sql)
{
    QString val;

    DB_LOCK;
    {
        QSqlQuery query(m_sqliteDb);

        if (m_sqliteDb.transaction())
        {
            query.prepare(sql);
            query.exec();

#ifdef QT_DEBUG
            QLog_Debug(DEBUGLOG, QString("Execute: %1").arg(sql));
#endif

            QSqlError e = query.lastError();
            if (e.isValid())
            {
                m_sqliteDb.rollback();
#ifdef QT_DEBUG
                QLog_Debug(DEBUGLOG, QString("Exec Error: %1").arg(e.text()));
#endif
            }
            else
            {
                m_sqliteDb.commit();
            }


            if (query.next())
            {
                val = query.value(0).toString();
#ifdef QT_DEBUG
                QLog_Debug(DEBUGLOG, QString("Result: %1").arg(query.value(0).toString()));
#endif
            }
#ifdef QT_DEBUG
            else
            {
                QLog_Debug(DEBUGLOG, QString("Result No Record"));
            }
#endif
        }
#ifdef QT_DEBUG
        else
        {
            QLog_Debug(DEBUGLOG, QString("Was not able lock for a transaction."));
        }
#endif
    }
    DB_UNLOCK;

    return val;
}

void DatabaseObjects::closeDatabase()
{
    // don't close twice
    if (m_databaseFile.isEmpty())
        return;

    delete m_clientsModel;
    delete m_unfilteredclientsModel;
    delete m_peopleModel;
    delete m_companyPeopleModel;
    delete m_unfilteredPeopleModel;
    delete m_projectInformationModel;
    delete m_projectsListModel;
    delete m_teamsModel;
    delete m_statusReportItemsModel;
    delete m_projectTeamMembersModel;
    delete m_projectLocationsModel;
    delete m_projectNotesModel;
    delete m_projectEditingNotesModel;
    delete m_actionItemProjectNotesModel;
    delete m_actionItemDetailsModel;
    delete m_actionItemsDetailsMeetingsModel;
    delete m_trackerItemsMeetingsModel;
    delete m_projectActionItemsModel;
    delete m_trackerItemCommentsModel;
    delete m_meetingAttendeesModel;
    delete m_notesActionItemsModel;

    delete m_searchResultsModel;

    m_clientsModel= nullptr;
    m_unfilteredclientsModel= nullptr;
    m_peopleModel= nullptr;
    m_companyPeopleModel= nullptr;
    m_unfilteredPeopleModel= nullptr;
    m_projectInformationModel= nullptr;
    m_projectsListModel= nullptr;
    m_teamsModel= nullptr;
    m_statusReportItemsModel= nullptr;
    m_projectTeamMembersModel= nullptr;
    m_projectLocationsModel= nullptr;
    m_projectNotesModel= nullptr;
    m_projectEditingNotesModel = nullptr;
    m_actionItemProjectNotesModel = nullptr;
    m_actionItemDetailsModel= nullptr;
    m_actionItemsDetailsMeetingsModel= nullptr;
    m_trackerItemsMeetingsModel = nullptr;
    m_projectActionItemsModel= nullptr;
    m_trackerItemCommentsModel = nullptr;
    m_meetingAttendeesModel= nullptr;
    m_notesActionItemsModel= nullptr;

    m_searchResultsModel = nullptr;

    delete m_clientsModelProxy;
    delete m_unfilteredclientsModelProxy;
    delete m_peopleModelProxy;
    delete m_companyPeopleModelProxy;
    delete m_unfilteredPeopleModelProxy;
    delete m_projectInformationModelProxy;
    delete m_projectsListModelProxy;
    delete m_teamsModelProxy;
    delete m_statusReportItemsModelProxy;
    delete m_projectTeamMembersModelProxy;
    delete m_projectLocationsModelProxy;
    delete m_projectNotesModelProxy;
    delete m_projectEditingNotesModelProxy;
    delete m_actionItemProjectNotesModelProxy;
    delete m_actionItemDetailsModelProxy;
    delete m_actionItemsDetailsMeetingsModelProxy;
    delete m_trackerItemsMeetingsModelProxy;
    delete m_projectActionItemsModelProxy;
    delete m_trackerItemCommentsModelProxy;
    delete m_meetingAttendeesModelProxy;
    delete m_notesActionItemsModelProxy;
    delete m_searchResultsModelProxy;

    m_clientsModelProxy = nullptr;
    m_unfilteredclientsModelProxy = nullptr;
    m_peopleModelProxy = nullptr;
    m_companyPeopleModelProxy = nullptr;
    m_unfilteredPeopleModelProxy = nullptr;
    m_projectInformationModelProxy = nullptr;
    m_projectsListModelProxy = nullptr;
    m_teamsModelProxy = nullptr;
    m_statusReportItemsModelProxy = nullptr;
    m_projectTeamMembersModelProxy = nullptr;
    m_projectLocationsModelProxy = nullptr;
    m_projectNotesModelProxy = nullptr;
    m_projectEditingNotesModelProxy = nullptr;
    m_actionItemProjectNotesModelProxy = nullptr;
    m_actionItemDetailsModelProxy = nullptr;
    m_actionItemsDetailsMeetingsModelProxy = nullptr;
    m_trackerItemsMeetingsModelProxy = nullptr;
    m_projectActionItemsModelProxy = nullptr;
    m_trackerItemCommentsModelProxy = nullptr;
    m_meetingAttendeesModelProxy = nullptr;
    m_notesActionItemsModelProxy = nullptr;
    m_searchResultsModelProxy = nullptr;

    m_sqliteDb.close();

    m_databaseFile.clear();
}

void DatabaseObjects::backupDatabase(const QString& file)
{
    DB_LOCK;
    QSqlQuery qry(m_sqliteDb);
    qry.prepare( "BEGIN IMMEDIATE;");

    qry.exec();

    QFile::remove(file); // copy command won't overwrite
    if (!QFile::copy(m_databaseFile, file))
    {
        if (m_gui)
            QMessageBox::critical(nullptr, QObject::tr("Database Backup Failed"), QString("Failed to backup the database.") );
    }

    qry.prepare( "ROLLBACK;");

    qry.exec();

    DB_UNLOCK;
}

bool DatabaseObjects::saveParameter( const QString& parametername, const QString& parametervalue )
{
    DB_LOCK;
    QSqlQuery select(m_sqliteDb);
    if(!select.prepare("select parameter_value from application_settings where parameter_name = ?;"))
    {
        if (m_gui)
            QMessageBox::critical(nullptr, QObject::tr("Database Access Failed"), QString("Failed to access a saved setting. You may need to restart Project Notes.\n\nError:\n%1").arg(select.lastError().text()) );

        return false;
    }

    select.bindValue(0, parametername);

    if (select.exec())
    {

        if (select.next())
        {
            QSqlQuery update(m_sqliteDb);
            update.prepare("update application_settings set parameter_value = ? where parameter_name = ?;");
            update.bindValue(0, parametervalue);
            update.bindValue(1, parametername);
            getDb().transaction();
            if (update.exec())
            {
                getDb().commit();
                DB_UNLOCK;
                return true;
            }

            getDb().rollback();
        }
        else
        {
            QSqlQuery insert(m_sqliteDb);
            insert.prepare("insert into application_settings (parameter_id, parameter_name, parameter_value) values (?, ?, ?);");
            insert.bindValue(0, QUuid::createUuid().toString());
            insert.bindValue(1, parametername);
            insert.bindValue(2, parametervalue);
            getDb().transaction();
            if (insert.exec())
            {
                getDb().commit();
                DB_UNLOCK;
                return true;
            }
            getDb().rollback();
        }

    }
    else
    {
        DB_UNLOCK;

        if (m_gui)
            QMessageBox::critical(nullptr, QObject::tr("Database Access Failed"), QString("Failed to access a saved setting.  You may need to restart Project Notes.\n\nError:\n%1").arg(select.lastError().text()));

        return false;
    }

    DB_UNLOCK;
    return false;
}

SqlQueryModel* DatabaseObjects::findOpenTable(const QString& tablename)
{
    for ( SqlQueryModel* m : getOpenModels())
    {
        if (m->tablename().compare(tablename, Qt::CaseInsensitive) == 0)
            return m;
    }

    return nullptr;
}

QString DatabaseObjects::loadParameter( const QVariant& parametername )
{
    QSqlQuery select(m_sqliteDb);
    if (!select.prepare("select parameter_value from application_settings where parameter_name = ?"))
    {
        if (m_gui)
            QMessageBox::critical(nullptr, QObject::tr("Database Access Failed"), QString("Failed to access a saved setting. You may need to restart Project Notes.\n\nError:\n%1").arg(select.lastError().text()) );

        return QString();
    }

    select.bindValue(0, parametername);

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
        if (m_gui)
            QMessageBox::critical(nullptr, QObject::tr("Database Access Failed"), QString("Failed to access a saved setting. You may need to restart Project Notes.\n\nError:\n%1").arg(select.lastError().text()) );

        DB_UNLOCK;
        return QString();
    }
}

void DatabaseObjects::setShowResolvedTrackerItems(bool value)
{
    saveParameter("UserFilter:ShowResolvedTrackerItems", (value ? "1": "0"));
}

bool DatabaseObjects::getShowResolvedTrackerItems()
{
    QString value = loadParameter("UserFilter:ShowResolvedTrackerItems");
    bool ret = (bool)value.toUInt();
    return ret;
}

void DatabaseObjects::setShowClosedProjects(bool value)
{
    saveParameter("UserFilter:ShowClosedProjects", (value ? "1": "0"));
}

bool DatabaseObjects::getShowClosedProjects()
{
    QString value = loadParameter("UserFilter:ShowClosedProjects");
    bool ret = (bool)value.toUInt();
    return ret;
}

void DatabaseObjects::setShowInternalItems(bool value)
{
    saveParameter("UserFilter:ShowInternalItems", (value ? "1": "0"));
}

bool DatabaseObjects::getShowInternalItems()
{
    QString value = loadParameter("UserFilter:ShowInternalItems");
    bool ret = (bool)value.toUInt();
    return ret;
}

void DatabaseObjects::setGlobalClientFilter(QString value)
{
    saveParameter("UserFilter:ClientFilter", value );
}

QString DatabaseObjects::getGlobalClientFilter()
{
    return loadParameter("UserFilter:ClientFilter");
}

void DatabaseObjects::setGlobalProjectFilter(QString value)
{
    saveParameter("UserFilter:ProjectFilter", value );
}

QString DatabaseObjects::getGlobalProjectFilter()
{
    return loadParameter("UserFilter:ProjectFilter");
}

void DatabaseObjects::setProjectManager(QString value)
{
    saveParameter("Preferences:ProjectManager", value );
}

QString DatabaseObjects::getProjectManager()
{
    return loadParameter("Preferences:ProjectManager");
}

void DatabaseObjects::setManagingCompany(QString value)
{
    saveParameter("Preferences:ManagingCompany", value );
}

QString DatabaseObjects::getManagingCompany()
{
    return loadParameter("Preferences:ManagingCompany");
}

void DatabaseObjects::setGlobalSearches( bool refresh )
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
        trackeritemsmodel()->setFilter(9, "Resolved", SqlQueryModel::NotEqual);
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
        // don't clear this one becasue we may have it open  projectinformationmodel()->clearFilter(0);
        projectslistmodel()->clearFilter(0);
        searchresultsmodel()->clearFilter(7);
    }
    else
    {
        projectslistmodel()->setFilter(0, getGlobalProjectFilter());

        QString projectnumber = execute(QString("select project_number from projects where project_id = '%1'").arg(getGlobalProjectFilter()));

        searchresultsmodel()->setFilter(5, projectnumber);
    }


    if (refresh)
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

QDomDocument* DatabaseObjects::createXMLExportDoc(QList<SqlQueryModel*>* querymodels)
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

    for (SqlQueryModel* m : *querymodels)
    {
        QDomElement e = m->toQDomElement(doc);

        root.appendChild(e);
    }

    return doc;
}

QDomDocument* DatabaseObjects::createXMLExportDoc(SqlQueryModel* querymodel, const QString& filter)
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

    QDomElement e = querymodel->toQDomElement(doc, filter);
    root.appendChild(e);

    return doc;
}

QList<QDomNode> DatabaseObjects::findTableNodes(const QDomNode& xmlelement, const QString& tablename)
{
    QList<QDomNode> domlist;

    QDomNode node = xmlelement.firstChild();

    while (!node.isNull())
    {
        if (node.nodeName() == "table" && node.toElement().attributeNode("name").value() == tablename)
        {
            domlist.append(node);
        }

        domlist.append(findTableNodes(node, tablename));

        node = node.nextSibling();
    }

    return domlist;
}

bool DatabaseObjects::importXMLDoc(const QDomDocument& xmldoc)
{
    // import clients
    QDomElement root = xmldoc.documentElement();
    QList<QDomNode> domlist;

    domlist = findTableNodes(root, "clients");
    if (!domlist.empty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, "importXMLDoc processing clients.");
#endif
        ClientsModel clients_model(this);

        for (QDomNode& tablenode : domlist)
            if (!clients_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
    }

    // import people
    domlist = findTableNodes(root, "people");
    if (!domlist.empty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, "importXMLDoc processing people.");
#endif
        PeopleModel people_model(this);

        for (QDomNode& tablenode : domlist)
            if(!people_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
    }

    // import projects
    domlist = findTableNodes(root, "projects");
    if (!domlist.empty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, "importXMLDoc processing projects.");
#endif
        ProjectsModel projects_model(this);

        for (QDomNode& tablenode : domlist)
            if (!projects_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
    }

    // import project people
    domlist = findTableNodes(root, "project_people");
    if (!domlist.empty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, "importXMLDoc processing project_people.");
#endif
        ProjectTeamMembersModel project_people_model(this);

        for (QDomNode& tablenode : domlist)
            if (!project_people_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
    }

    // import status report items
    domlist = findTableNodes(root, "status_report_items");
    if (!domlist.empty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, "importXMLDoc processing status_report_items.");
#endif
        StatusReportItemsModel status_report_items_model(this);

        for (QDomNode& tablenode : domlist)
            if(!status_report_items_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
    }

    // import project locations
    domlist = findTableNodes(root, "project_locations");
    if (!domlist.empty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, "importXMLDoc processing project_locations.");
#endif
        ProjectLocationsModel project_locations_model(this);

        for (QDomNode& tablenode : domlist)
            if(!project_locations_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
    }


    // import project notes
    domlist = findTableNodes(root, "project_notes");
    if (!domlist.empty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, "importXMLDoc processing project_notes.");
#endif
        ProjectNotesModel project_notes_model(this);

        for (QDomNode& tablenode : domlist)
            if(!project_notes_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
    }

    // import item tracker
    domlist = findTableNodes(root, "item_tracker");
    if (!domlist.empty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, "importXMLDoc processing item_tracker.");
#endif
        TrackerItemsModel tracker_items_model(this);

        for (QDomNode& tablenode : domlist)
            if(!tracker_items_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
    }

    // import meeting attendees
    domlist = findTableNodes(root, "meeting_attendees");
    if (!domlist.empty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, "importXMLDoc processing meeting_attendees.");
#endif
        MeetingAttendeesModel meeting_attendees_model(this);

        for (QDomNode& tablenode : domlist)
            if(!meeting_attendees_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
    }

    // import tracker items updates
    domlist = findTableNodes(root, "item_tracker_updates");
    if (!domlist.empty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, "importXMLDoc processing item_tracker_updates.");
#endif
        TrackerItemCommentsModel item_tracker_updates_model(this);

        for (QDomNode& tablenode : domlist)
            if(!item_tracker_updates_model.importXMLNode(tablenode))
                return false;

        domlist.clear();
    }

    return true;
}

QList<SqlQueryModel*>* DatabaseObjects::getData(const QDomDocument& xmldoc)
{
    QList<SqlQueryModel*> *model_list = new QList<SqlQueryModel*>();

    QDomElement root = xmldoc.documentElement();
    QList<QDomNode> domlist;

    // import clients
    domlist = findTableNodes(root, "clients");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            SqlQueryModel* model = new ClientsModel(this);
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
            SqlQueryModel* model = new PeopleModel(this);
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
            SqlQueryModel* model = new ProjectsModel(this);
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
            SqlQueryModel* model = new ProjectTeamMembersModel(this);
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
            SqlQueryModel* model = new StatusReportItemsModel(this);
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
            SqlQueryModel* model = new ProjectLocationsModel(this);
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
            SqlQueryModel* model = new ProjectNotesModel(this);
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
            SqlQueryModel* model = new TrackerItemsModel(this);
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
            SqlQueryModel* model = new MeetingAttendeesModel(this);
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
            SqlQueryModel* model = new TrackerItemCommentsModel(this);
            model->setFilter(tablenode);
            model->refresh();
            model_list->append(model);
        }

        domlist.clear();
    }

    return model_list;
}

void DatabaseObjects::addDefaultPMToProject(const QString& projectId)
{
    QString pm = getProjectManager();
    QString guid = QUuid::createUuid().toString();

    QString insert = QString("insert into project_people (teammember_id, people_id, project_id, role) select '%3', '%2', '%1', 'Project Manager' where not exists (select 1 from project_people where project_id = '%1' and people_id = '%2' )").arg(projectId).arg(pm).arg(guid);

    execute(insert);
}

void DatabaseObjects::addDefaultPMToMeeting(const QString& noteId)
{
    QString pm = getProjectManager();
    QString guid = QUuid::createUuid().toString();
    QString guid2 = QUuid::createUuid().toString();

    QString project_id = execute(QString("select project_id from project_notes where note_id='%1'").arg(noteId));
    QString insertpm = QString("insert into project_people (teammember_id, people_id, project_id, role) select '%3', '%2', '%1', 'Project Manager' where not exists (select 1 from project_people where project_id = '%1' and people_id = '%2' )").arg(project_id).arg(pm).arg(guid2);

    execute(insertpm);

    QString insert = QString("insert into meeting_attendees (attendee_id, person_id, note_id) select '%3', '%2', '%1' where not exists (select 1 from meeting_attendees where note_id = '%1' and person_id = '%2' )").arg(noteId).arg(pm).arg(guid);

    execute(insert);

    // make sure displays get updated
    global_DBObjects.pushRowChange("project_notes", noteId);
    global_DBObjects.pushRowChange("project_people", guid2);
    global_DBObjects.pushRowChange("meeting_attendees", guid);
    global_DBObjects.updateDisplayData();
}

// Push a new change; skips if exact duplicate already exists
void DatabaseObjects::pushRowChange(const QString& table, const QVariant& value, const KeyColumnChange::OperationType optype)
{
    KeyColumnChange newChange{table, value, optype};
    if (!m_keyColumnChanges.contains(newChange))
    {
        m_keyColumnChanges.append(newChange);
    }

    // qDebug() << "Push change to " << table << " of id " << value << " to change stack.";
}

// Pop the last added change; returns true if successful, false if empty
bool DatabaseObjects::popRowChange(KeyColumnChange& outChange)
{
    if (m_keyColumnChanges.isEmpty())
    {
        return false;
    }

    outChange = m_keyColumnChanges.takeLast();

    // qDebug() << "processing changed to " << outChange.table << " of id " << outChange.value << " from change stack.";

    return true;
}

// Bonus: merge changes from another store (skipping duplicates)
void DatabaseObjects::addColumnChanges(const DatabaseObjects& source)
{
    for (const KeyColumnChange& ch : source.m_keyColumnChanges)
    {
        if (!m_keyColumnChanges.contains(ch))
        {
            m_keyColumnChanges.append(ch);
        }
    }
}

void DatabaseObjects::updateDisplayData()
{
    KeyColumnChange keyColChange;

    while (popRowChange(keyColChange))
    {
        SqlQueryModel* recordset = nullptr;

        QListIterator<SqlQueryModel*> it_recordsets(getOpenModels());

        // look through all recordsets that are open
        while(it_recordsets.hasNext())
        {
            recordset = it_recordsets.next();

            if (recordset->tablename().compare( keyColChange.table) == 0)
            {
                if (keyColChange.operation_type == KeyColumnChange::Update)
                {
                    QModelIndex qmi = recordset->findIndex(keyColChange.value, 0);
                    if (qmi.isValid())
                    {
                        recordset->reloadRecord(qmi);
                        // qDebug() << "Updating display for table " << recordset->tablename() << " row " << qmi.row() << " with value " << keyColChange.value;
                    }
                }
                else if (keyColChange.operation_type == KeyColumnChange::Delete)
                {
                    QModelIndex qmi = recordset->findIndex(keyColChange.value, 0);
                    if (qmi.isValid())
                    {
                        recordset->removeCacheRecord(qmi);
                    }
                }
                else  // it is an insert
                {
                    // load record to a temporary row then filter check it
                    recordset->loadAndFilterRow(keyColChange.value);
                }
            }
        }
    }
}
