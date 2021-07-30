#include "pndatabaseobjects.h"

#include <QUuid>

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

QStringList PNDatabaseObjects::locations = {
    "File Folder",
    "Web Link",
    "Microsoft Project",
    "Word Document",
    "Excel Document",
    "PDF File",
    "Generic File (System Identified)"
};

PNDatabaseObjects global_DBObjects(nullptr);

PNDatabaseObjects::PNDatabaseObjects(QObject *parent) : QObject(parent)
{
    m_DatabaseFile.clear();
}

bool PNDatabaseObjects::OpenDatabase(QString& databasepath)
{
    m_DatabaseFile = databasepath;

    m_SQLiteDB = QSqlDatabase::addDatabase("QSQLITE");

    if (QFileInfo::exists(m_DatabaseFile))
        m_SQLiteDB.setDatabaseName(m_DatabaseFile);
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
            QString(tr("File %1 does not exist.")).arg(m_DatabaseFile), QMessageBox::Cancel);
        return false;
    }

    if (!m_SQLiteDB.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
            m_SQLiteDB.lastError().text(), QMessageBox::Cancel);
        return false;
    }

    m_ClientsModel = new ClientsModel(nullptr);
    m_UnfilteredClientsModel = new ClientsModel(nullptr);
    m_PeopleModel = new PeopleModel(nullptr);
    m_CompanyPeopleModel = new PeopleModel(nullptr);
    m_UnfilteredPeopleModel = new PeopleModel(nullptr);
    m_ProjectInformationModel = new ProjectsModel(nullptr);
    m_TeamsModel = new TeamsModel(nullptr);
    m_StatusReportItemsModel = new StatusReportItemsModel(nullptr);
    m_ProjectTeamMembersModel = new ProjectTeamMembersModel(nullptr);
    m_ProjectLocationsModel = new ProjectLocationsModel(nullptr);
    m_ProjectNotesModel = new ProjectNotesModel(nullptr);
    m_ActionItemProjectNotesModel = new ActionItemProjectNotesModel(nullptr);
    m_ActinoItemsDetailsMeetingsModel = new ActionItemsDetailsMeetingsModel(nullptr);
    m_MeetingAttendeesModel = new MeetingAttendeesModel(nullptr);
    m_NotesActionItemsModel = new NotesActionItemsModel(nullptr);

    //m_PeopleModel->setShowBlank(true);

    return true;
}

QString PNDatabaseObjects::Execute(const QString& sql)
{
    QSqlQuery query;
    query.exec(sql);

    if (query.next())
        return query.value(0).toString();
    else
        return QString();
}

void PNDatabaseObjects::CloseDatabase()
{
    delete m_ClientsModel;
    delete m_UnfilteredClientsModel;
    delete m_PeopleModel;
    delete m_CompanyPeopleModel;
    delete m_UnfilteredPeopleModel;
    delete m_ProjectInformationModel;
    delete m_TeamsModel;
    delete m_StatusReportItemsModel;
    delete m_ProjectTeamMembersModel;
    delete m_ProjectLocationsModel;
    delete m_ProjectNotesModel;
    delete m_ActionItemProjectNotesModel;
    delete m_ActinoItemsDetailsMeetingsModel;
    delete m_MeetingAttendeesModel;
    delete m_NotesActionItemsModel;

    m_ClientsModel= nullptr;
    m_UnfilteredClientsModel= nullptr;
    m_PeopleModel = nullptr;
    m_CompanyPeopleModel= nullptr;
    m_UnfilteredPeopleModel= nullptr;
    m_ProjectInformationModel= nullptr;
    m_TeamsModel= nullptr;
    m_StatusReportItemsModel= nullptr;
    m_ProjectTeamMembersModel= nullptr;
    m_ProjectLocationsModel= nullptr;
    m_ProjectNotesModel= nullptr;
    m_ActionItemProjectNotesModel= nullptr;
    m_ActinoItemsDetailsMeetingsModel= nullptr;
    m_MeetingAttendeesModel= nullptr;
    m_NotesActionItemsModel= nullptr;

    m_SQLiteDB.close();
}

void PNDatabaseObjects::BackupDatabase(QWidget& parent, QFileInfo& file)
{
    // TODO:  This may not make sense to do when using the QT SQL interface
}

bool PNDatabaseObjects::SaveParameter( const QString& ParameterName, const QString& ParameterValue )
{
    QSqlQuery select("select parameter_value from application_settings where parameter_name = ?;");
    select.bindValue(0, ParameterName);
    if (select.exec())
    {
        if (select.next())
        {
            QSqlQuery update("update application_settings set parameter_value = ? where parameter_name = ?;");
            update.bindValue(0, ParameterValue);
            update.bindValue(1, ParameterName);
            if (update.exec())
                return true;
        }
        else
        {
            QSqlQuery insert("insert into application_settings (parameter_id, parameter_name, parameter_value) values (?, ?, ?);");
            insert.bindValue(2, QUuid::createUuid().toString());
            insert.bindValue(1, ParameterName);
            insert.bindValue(2, ParameterValue);
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

QString PNDatabaseObjects::LoadParameter( const QString& ParameterName )
{
    QSqlQuery select("select parameter_value from application_settings where parameter_name = ?;");
    select.bindValue(0, ParameterName);
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


void PNDatabaseObjects::SetShowAllTrackerItems(bool value)
{
    SaveParameter("UserFilter:ShowAllTrackerItems", (value ? "1": "0"));
}

void PNDatabaseObjects::SetShowClosedProjects(bool value)
{
    SaveParameter("UserFilter:ShowClosedProjects", (value ? "1": "0"));
}

bool PNDatabaseObjects::GetShowClosedProjects()
{
    QString value = LoadParameter("UserFilter:ShowClosedProjects");
    bool ret = (bool)value.toUInt();
    return ret;
}

void PNDatabaseObjects::SetShowInternalItems(bool value)
{
    SaveParameter("UserFilter:ShowInternalItems", (value ? "1": "0"));
}

bool PNDatabaseObjects::GetShowInternalItems()
{
    QString value = LoadParameter("UserFilter:ShowInternalItems");
    bool ret = (bool)value.toUInt();
    return ret;
}

void PNDatabaseObjects::SetGlobalClientFilter(QString value)
{
    SaveParameter("UserFilter:ClientFilter", value );
}

QString PNDatabaseObjects::GetGlobalClientFilter()
{
    return LoadParameter("UserFilter:ClientFilter");
}

void PNDatabaseObjects::SetGlobalProjectFilter(QString value)
{
    SaveParameter("UserFilter:ProjectFilter", value );
}

QString PNDatabaseObjects::GetGlobalProjectFilter()
{
    return LoadParameter("UserFilter:ProjectFilter");
}

void PNDatabaseObjects::SetProjectManager(QString value)
{
    SaveParameter("Preferences:ProjectManager", value );
}

QString PNDatabaseObjects::GetProjectManager()
{
    return LoadParameter("Preferences:ProjectManager");
}

void PNDatabaseObjects::SetManagingCompany(QString value)
{
    SaveParameter("Preferences:ManagingCompany", value );
}

QString PNDatabaseObjects::GetManagingCompany()
{
    return LoadParameter("Preferences:ManagingCompany");
}

void PNDatabaseObjects::SetGlobalSearches( bool Refresh )
{ 
    // setup default filters
    if (GetShowClosedProjects())
    {
        projectactionitemsmodel()->ClearFilter(9);
        projectinformationmodel()->ClearFilter(14);
        searchresultsmodel()->ClearFilter(6);
    }
    else
    {
        projectactionitemsmodel()->SetFilter(9, tr("Active"));
        projectinformationmodel()->SetFilter(14, tr("Active"));
        searchresultsmodel()->SetFilter(6, tr("Active"));
    }

    if (GetShowInternalItems())
    {
        projectnotesmodel()->ClearFilter(5);
        actionitemsdetailsmeetingsmodel()->ClearFilter(3);
        notesactionitemsmodel()->ClearFilter(15);
        actionitemprojectnotesmodel()->ClearFilter(3);
        projectactionitemsmodel()->ClearFilter(15);
        actionitemsdetailsmodel()->ClearFilter(3);
        searchresultsmodel()->ClearFilter(4);
    }
    else
    {
        projectnotesmodel()->SetFilter(5, tr("0"));
        actionitemsdetailsmeetingsmodel()->SetFilter(3, tr("0"));
        notesactionitemsmodel()->SetFilter(15, tr("0"));
        actionitemprojectnotesmodel()->SetFilter(3, tr("0"));
        projectactionitemsmodel()->SetFilter(15, tr("0"));
        actionitemsdetailsmodel()->SetFilter(3, tr("0"));
        searchresultsmodel()->SetFilter(4, tr("0"));
    }

    if (GetGlobalClientFilter().isEmpty())
    {
        peoplemodel()->ClearUserFilter(5);
        peoplemodel()->DeactivateUserFilter(tr("GlobalClientFilter"));
        clientsmodel()->ClearFilter(0);
        clientsmodel()->DeactivateUserFilter(tr("GlobalClientFilter"));
        projectactionitemsmodel()->ClearFilter(18);
        projectinformationmodel()->ClearFilter(13);
        searchresultsmodel()->ClearFilter(5);
    }
    else
    {
        //QString managing = Execute(QString("select client_name from clients where client_id = '%1'").arg(GetManagingCompany()));
        //QString filtered = Execute(QString("select client_name from clients where client_id = '%1'").arg(GetGlobalClientFilter()));

        QStringList managingnclientids;
        // make sure list of people can show the managing company
        managingnclientids.append(GetManagingCompany());
        managingnclientids.append(GetGlobalClientFilter());

        clientsmodel()->SetUserFilter(0, managingnclientids );
        clientsmodel()->ActivateUserFilter("GlobalClientFilter");

        peoplemodel()->SetUserFilter(5, managingnclientids);
        peoplemodel()->ActivateUserFilter(tr("GlobalClientFilter"));

        projectactionitemsmodel()->SetFilter(18, GetGlobalClientFilter());
        projectinformationmodel()->SetFilter(13, GetGlobalClientFilter());
        searchresultsmodel()->SetFilter(5, GetGlobalClientFilter());
    }

    if (GetGlobalProjectFilter().isEmpty())
    {
        projectactionitemsmodel()->ClearFilter(14);
        projectinformationmodel()->ClearFilter(0);
        searchresultsmodel()->ClearFilter(7);
    }
    else
    {
        projectactionitemsmodel()->SetFilter(14, GetGlobalProjectFilter());
        projectinformationmodel()->SetFilter(0, GetGlobalProjectFilter());

        QString projectnumber = Execute(QString("select project_number from projects where project_id = '%1'").arg(GetGlobalProjectFilter()));

        searchresultsmodel()->SetFilter(5, projectnumber);
    }


    if (Refresh)
    {
        peoplemodel()->Refresh();
        clientsmodel()->Refresh();
        projectnotesmodel()->Refresh();
        actionitemsdetailsmeetingsmodel()->Refresh();
        notesactionitemsmodel()->Refresh();
        actionitemprojectnotesmodel()->Refresh();
        actionitemsdetailsmodel()->Refresh();

        projectactionitemsmodel()->Refresh();
        projectinformationmodel()->Refresh();
        searchresultsmodel()->Refresh();
    }
}

bool PNDatabaseObjects::ExecuteDDL(const QString& SQL)
{
  // TODO : finish
}


