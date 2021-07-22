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


PNDatabaseObjects::PNDatabaseObjects(QString& databasepath, QObject *parent) : QObject(parent)
{
    m_DatabaseFile = databasepath;
}

bool PNDatabaseObjects::OpenDatabase()
{
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

    m_ClientsModel = new ClientsModel(NULL);

    m_UnfilteredClientsModel = new ClientsModel(NULL);
    m_PeopleModel = new PeopleModel(NULL);
    //m_PeopleModel->setShowBlank(true);
    m_CompanyPeopleModel = new PeopleModel(NULL);
    m_UnfilteredPeopleModel = new PeopleModel(NULL);
    m_ProjectInformationModel = new ProjectsModel(NULL);
    m_TeamsModel = new TeamsModel(NULL);
    m_StatusReportItemsModel = new StatusReportItemsModel(NULL);
    m_ProjectTeamMembersModel = new ProjectTeamMembersModel(NULL);
    m_ProjectLocationsModel = new ProjectLocationsModel(NULL);
    m_ProjectNotesModel = new ProjectNotesModel(NULL);
    m_ActionItemProjectNotesModel = new ActionItemProjectNotesModel(NULL);
    m_ActinoItemsDetailsMeetingsModel = new ActionItemsDetailsMeetingsModel(NULL);
    m_MeetingAttendeesModel = new MeetingAttendeesModel(NULL);

    return true;
}

void PNDatabaseObjects::CloseDatabase()
{
    if (m_ClientsModel != NULL)
    {
        delete m_ClientsModel;
        m_ClientsModel = NULL;
    }

    if (m_UnfilteredClientsModel != NULL)
    {
        delete m_UnfilteredClientsModel;
        m_UnfilteredClientsModel = NULL;
    }

    if (m_PeopleModel != NULL)
    {
        delete m_PeopleModel;
        m_PeopleModel = NULL;
    }

    if (m_CompanyPeopleModel != NULL)
    {
        delete m_CompanyPeopleModel;
        m_CompanyPeopleModel = NULL;
    }

    if (m_UnfilteredPeopleModel != NULL)
    {
        delete m_UnfilteredPeopleModel;
        m_UnfilteredPeopleModel = NULL;
    }

    if (m_ProjectInformationModel != NULL)
    {
        delete m_ProjectInformationModel;
        m_ProjectInformationModel = NULL;
    }

    if (m_TeamsModel != NULL)
    {
        delete m_TeamsModel;
        m_TeamsModel = NULL;
    }

    if (m_StatusReportItemsModel != NULL)
    {
        delete m_StatusReportItemsModel;
        m_StatusReportItemsModel = NULL;
    }

    if (m_ProjectTeamMembersModel != NULL)
    {
        delete m_ProjectTeamMembersModel;
        m_ProjectTeamMembersModel = NULL;
    }

    if (m_ProjectLocationsModel != NULL)
    {
        delete m_ProjectLocationsModel;
        m_ProjectLocationsModel = NULL;
    }

    if (m_ProjectNotesModel != NULL)
    {
        delete m_ProjectNotesModel;
        m_ProjectNotesModel = NULL;
    }

    if (m_ActionItemProjectNotesModel != NULL)
    {
        delete m_ActionItemProjectNotesModel;
        m_ActionItemProjectNotesModel = NULL;
    }

    if (m_ActinoItemsDetailsMeetingsModel != NULL)
    {
        delete m_ActinoItemsDetailsMeetingsModel;
        m_ActinoItemsDetailsMeetingsModel = NULL;
    }

    if (m_MeetingAttendeesModel != NULL)
    {
        delete m_MeetingAttendeesModel;
        m_MeetingAttendeesModel = NULL;
    }

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
    QString retvalue;

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

bool PNDatabaseObjects::ExecuteDDL(const QString& SQL)
{
  // TODO : finish
}

void PNDatabaseObjects::SetGlobalSearches( bool Refresh )
{
  // TODO: finish
}
