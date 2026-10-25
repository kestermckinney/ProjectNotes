// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseobjects.h"
#include "recordtree.h"

#include <QScopedPointer>
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
QReadWriteLock db_rwlock;

std::function<void(const QString&, const QString&)> DatabaseObjects::s_localSave;
std::function<QString(const QString&)> DatabaseObjects::s_localLoad;

void DatabaseObjects::setLocalSettingsCallbacks(
    std::function<void(const QString&, const QString&)> saver,
    std::function<QString(const QString&)> loader)
{
    s_localSave = saver;
    s_localLoad = loader;
}

void DatabaseObjects::saveLocalParameter(const QString& name, const QString& value)
{
    if (s_localSave)
        s_localSave(name, value);
}

QString DatabaseObjects::loadLocalParameter(const QString& name)
{
    if (s_localLoad)
        return s_localLoad(name);
    return QString();
}


DatabaseObjects::DatabaseObjects(QObject *parent) : QObject(parent)
{
    m_databaseFile.clear();
}

bool DatabaseObjects::createDatabase(const QString& databasepath)
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

    m_sqliteDb.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");

    if (!m_sqliteDb.open())
    {
        if (m_gui)
            QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
                                  m_sqliteDb.lastError().text(), QMessageBox::Cancel);

        m_databaseFile.clear(); // set empty if bad file
        return false;
    }

    if (m_gui)
    {
        DatabaseStructure ds;
        if (!ds.UpgradeDatabase()) {
            m_sqliteDb.close();
            m_databaseFile.clear();
            return false;
        }
    }

    m_companiesModel = new CompaniesModel(this);
    m_companiesModelProxy = new SortFilterProxyModel(this);
    m_companiesModelProxy->setSourceModel(m_companiesModel);

    m_unfilteredCompaniesModel = new CompaniesModel(this);
    m_unfilteredCompaniesModelProxy = new SortFilterProxyModel(this);
    m_unfilteredCompaniesModelProxy->setSourceModel(m_unfilteredCompaniesModel);
    m_unfilteredCompaniesModel->setShowBlank();
    m_unfilteredCompaniesModel->setNoExport();

    m_contactsModel = new ContactsModel(this);
    m_contactsModelProxy = new SortFilterProxyModel(this);
    m_contactsModelProxy->setSourceModel(m_contactsModel);

    m_companyContactsModel = new ContactsModel(this);
    m_companyContactsModelProxy = new SortFilterProxyModel(this);
    m_companyContactsModelProxy->setSourceModel(m_companyContactsModel);

    m_unfilteredContactsModel = new ContactsModel(this);
    m_unfilteredContactsModelProxy = new SortFilterProxyModel(this);
    m_unfilteredContactsModelProxy->setSourceModel(m_unfilteredContactsModel);
    m_unfilteredContactsModel->setShowBlank();
    m_unfilteredContactsModel->setNoExport();

    m_projectInformationModel = new ProjectsModel(this);
    m_projectInformationModelProxy = new SortFilterProxyModel(this);
    m_projectInformationModelProxy->setSourceModel(m_projectInformationModel);

    m_projectsListModel = new ProjectsListModel(this);
    m_projectsListModelProxy = new SortFilterProxyModel(this);
    m_projectsListModelProxy->setSourceModel(m_projectsListModel);

    m_unfilteredProjectsListModel = new ProjectsListModel(this);
    m_unfilteredProjectsListModelProxy = new SortFilterProxyModel(this);
    m_unfilteredProjectsListModelProxy->setSourceModel(m_unfilteredProjectsListModel);
    m_unfilteredProjectsListModel->setShowBlank();
    m_unfilteredProjectsListModel->setNoExport();

    m_teamsModel = new TeamsModel(this);
    m_teamsModelProxy = new SortFilterProxyModel(this);
    m_teamsModelProxy->setSourceModel(m_teamsModel);

    m_statusReportItemsModel = new StatusReportItemsModel(this);
    m_statusReportItemsModelProxy = new SortFilterProxyModel(this);
    m_statusReportItemsModelProxy->setSourceModel(m_statusReportItemsModel);

    m_projectTeamMembersModel = new ProjectTeamMembersModel(this);
    m_projectTeamMembersModelProxy = new SortFilterProxyModel(this);
    m_projectTeamMembersModelProxy->setSourceModel(m_projectTeamMembersModel);

    m_projectLocationsModel = new ProjectLocationsModel(this);
    m_projectLocationsModelProxy = new SortFilterProxyModel(this);
    m_projectLocationsModelProxy->setSourceModel(m_projectLocationsModel);

    m_projectNotesModel = new ProjectNotesModel(this);
    m_projectNotesModelProxy = new SortFilterProxyModel(this);
    m_projectNotesModelProxy->setSourceModel(m_projectNotesModel);

    m_projectEditingNotesModel = new ProjectNotesModel(this);
    m_projectEditingNotesModelProxy = new SortFilterProxyModel(this);
    m_projectEditingNotesModelProxy->setSourceModel(m_projectEditingNotesModel);

    m_actionItemProjectNotesModel = new ActionItemProjectNotesModel(this);
    m_actionItemProjectNotesModelProxy = new SortFilterProxyModel(this);
    m_actionItemProjectNotesModelProxy->setSourceModel(m_actionItemProjectNotesModel);

    m_trackerItemsMeetingsModel = new ActionItemsDetailsMeetingsModel(this);
    m_trackerItemsMeetingsModelProxy = new SortFilterProxyModel(this);
    m_trackerItemsMeetingsModelProxy->setSourceModel(m_trackerItemsMeetingsModel);
    m_trackerItemsMeetingsModel->setShowBlank();
    m_trackerItemsMeetingsModel->setNoExport();

    m_actionItemsDetailsMeetingsModel = new ActionItemsDetailsMeetingsModel(this);
    m_actionItemsDetailsMeetingsModelProxy = new SortFilterProxyModel(this);
    m_actionItemsDetailsMeetingsModelProxy->setSourceModel(m_actionItemsDetailsMeetingsModel);
    m_actionItemsDetailsMeetingsModel->setShowBlank();
    m_actionItemsDetailsMeetingsModel->setNoExport();

    m_projectActionItemsModel = new TrackerItemsModel(this);
    m_projectActionItemsModelProxy = new SortFilterProxyModel(this);
    m_projectActionItemsModelProxy->setSourceModel(m_projectActionItemsModel);

    m_allItemsModel = new TrackerItemsModel(this);
    m_allItemsModel->setOrderBy("project_number, item_number");
    m_allItemsModelProxy = new SortFilterProxyModel(this);
    m_allItemsModelProxy->setSourceModel(m_allItemsModel);

    m_actionItemDetailsModel = new TrackerItemsModel(this);
    m_actionItemDetailsModelProxy = new SortFilterProxyModel(this);
    m_actionItemDetailsModelProxy->setSourceModel(m_actionItemDetailsModel);

    m_meetingAttendeesModel = new MeetingAttendeesModel(this);
    m_meetingAttendeesModelProxy = new SortFilterProxyModel(this);
    m_meetingAttendeesModelProxy->setSourceModel(m_meetingAttendeesModel);

    m_notesActionItemsModel = new NotesActionItemsModel(this);
    m_notesActionItemsModelProxy = new SortFilterProxyModel(this);
    m_notesActionItemsModelProxy->setSourceModel(m_notesActionItemsModel);

    m_trackerItemCommentsModel = new TrackerItemCommentsModel(this);
    m_trackerItemCommentsModelProxy = new SortFilterProxyModel(this);
    m_trackerItemCommentsModelProxy->setSourceModel(m_trackerItemCommentsModel);

    m_searchResultsModel = new SearchResultsModel(this);
    m_searchResultsModelProxy = new SortFilterProxyModel(this);
    m_searchResultsModelProxy->setSourceModel(m_searchResultsModel);

    return true;
}

void DatabaseObjects::addModel(SqlQueryModel* model)
{
    m_openRecordsets.append(model);
}

void DatabaseObjects::removeModel(SqlQueryModel* model)
{
    m_openRecordsets.removeAll(model);  // remove from the list of open recordsets
}

SqlQueryModel* DatabaseObjects::createExportObject(const QString& tableName)
{
    const QString lower = tableName.toLower();

    if (lower == QLatin1String("clients"))
        return new CompaniesModel(this);
    else if (lower == QLatin1String("people"))
        return new ContactsModel(this);
    else if (lower == QLatin1String("projects"))
        return new ProjectsModel(this);
    else if (lower == QLatin1String("status_report_items"))
        return new StatusReportItemsModel(this);
    else if (lower == QLatin1String("project_email_audiences"))
        return new ProjectEmailAudiencesModel(this);
    else if (lower == QLatin1String("project_people"))
        return new ProjectTeamMembersModel(this);
    else if (lower == QLatin1String("project_locations"))
        return new ProjectLocationsModel(this);
    else if (lower == QLatin1String("project_notes"))
        return new ProjectNotesModel(this);
    else if (lower == QLatin1String("item_tracker"))
        return new TrackerItemsModel(this);
    else if (lower == QLatin1String("meeting_attendees"))
        return new MeetingAttendeesModel(this);
    else if (lower == QLatin1String("item_tracker_updates"))
        return new TrackerItemCommentsModel(this);
    else
        return nullptr;
}

QString DatabaseObjects::execute(const QString& sql)
{
    QString val;

    QWriteLocker locker(&db_rwlock);
    {
        QSqlQuery query(m_sqliteDb);

        // Inside beginBatch()/commitBatch() every statement joins the batch's
        // transaction; starting another would fail and skip the statement.
        if (m_inBatch)
        {
            query.prepare(sql);
            if (!query.exec())
            {
                QString msg = objectName() + " - SQL QUERY FAILED: " + query.lastError().text() + "\nSQL: " + sql;
                qWarning() << msg;
                QLog_Error(ERRORLOG, msg);
                m_batchFailed = true;
                m_batchError = query.lastError().text();
            }
            else if (query.next())
            {
                val = query.value(0).toString();
            }
            return val;
        }

        if (m_sqliteDb.transaction())
        {
            query.prepare(sql);
            if (!query.exec())
            {
                QString msg = objectName() + " - SQL QUERY FAILED: " + query.lastError().text() + "\nSQL: " + sql;
                qWarning() << msg;
                QLog_Error(ERRORLOG, msg);
            }

            QSqlError e = query.lastError();
            if (e.isValid())
            {
                m_sqliteDb.rollback();
            }
            else
            {
                m_sqliteDb.commit();
            }


            if (query.next())
            {
                val = query.value(0).toString();
#ifdef QT_DEBUG
                QLog_Debug(DEBUGLOG, QString("Result: %1 for query: %2").arg(query.value(0).toString(),sql));
#endif
            }
#ifdef QT_DEBUG
            else
            {
                QLog_Debug(DEBUGLOG, QString("Result No Record for query: %1").arg(sql));
            }
#endif
        }
        else
        {
            QLog_Error(ERRORLOG, QString("Was not able lock for a transaction."));
        }
    }

    return val;
}

bool DatabaseObjects::beginBatch()
{
    QWriteLocker locker(&db_rwlock);
    if (m_inBatch || !m_sqliteDb.transaction())
        return false;
    m_inBatch = true;
    m_batchFailed = false;
    m_batchError.clear();
    return true;
}

bool DatabaseObjects::commitBatch()
{
    QWriteLocker locker(&db_rwlock);
    if (!m_inBatch)
        return false;
    m_inBatch = false;
    if (m_batchFailed) {
        m_sqliteDb.rollback();
        return false;
    }
    if (!m_sqliteDb.commit()) {
        m_batchError = m_sqliteDb.lastError().text();
        m_sqliteDb.rollback();
        return false;
    }
    return true;
}

void DatabaseObjects::rollbackBatch()
{
    QWriteLocker locker(&db_rwlock);
    if (!m_inBatch)
        return;
    m_inBatch = false;
    m_sqliteDb.rollback();
}

void DatabaseObjects::closeDatabase()
{
    // don't close twice
    if (m_databaseFile.isEmpty())
        return;

    // delete proxy models BEFORE their source models
    delete m_companiesModelProxy;
    delete m_unfilteredCompaniesModelProxy;
    delete m_contactsModelProxy;
    delete m_companyContactsModelProxy;
    delete m_unfilteredContactsModelProxy;
    delete m_projectInformationModelProxy;
    delete m_projectsListModelProxy;
    delete m_unfilteredProjectsListModelProxy;
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
    delete m_allItemsModelProxy;
    delete m_trackerItemCommentsModelProxy;
    delete m_meetingAttendeesModelProxy;
    delete m_notesActionItemsModelProxy;
    delete m_searchResultsModelProxy;

    m_companiesModelProxy = nullptr;
    m_unfilteredCompaniesModelProxy = nullptr;
    m_contactsModelProxy = nullptr;
    m_companyContactsModelProxy = nullptr;
    m_unfilteredContactsModelProxy = nullptr;
    m_projectInformationModelProxy = nullptr;
    m_projectsListModelProxy = nullptr;
    m_unfilteredProjectsListModelProxy = nullptr;
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
    m_allItemsModelProxy = nullptr;
    m_trackerItemCommentsModelProxy = nullptr;
    m_meetingAttendeesModelProxy = nullptr;
    m_notesActionItemsModelProxy = nullptr;
    m_searchResultsModelProxy = nullptr;

    // now delete source models
    delete m_companiesModel;
    delete m_unfilteredCompaniesModel;
    delete m_contactsModel;
    delete m_companyContactsModel;
    delete m_unfilteredContactsModel;
    delete m_projectInformationModel;
    delete m_projectsListModel;
    delete m_unfilteredProjectsListModel;
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
    delete m_allItemsModel;
    delete m_trackerItemCommentsModel;
    delete m_meetingAttendeesModel;
    delete m_notesActionItemsModel;

    delete m_searchResultsModel;

    m_companiesModel= nullptr;
    m_unfilteredCompaniesModel= nullptr;
    m_contactsModel= nullptr;
    m_companyContactsModel= nullptr;
    m_unfilteredContactsModel= nullptr;
    m_projectInformationModel= nullptr;
    m_projectsListModel= nullptr;
    m_unfilteredProjectsListModel = nullptr;
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
    m_allItemsModel = nullptr;
    m_trackerItemCommentsModel = nullptr;
    m_meetingAttendeesModel= nullptr;
    m_notesActionItemsModel= nullptr;

    m_searchResultsModel = nullptr;

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

bool DatabaseObjects::saveParameter(const QString& parametername, const QString& parametervalue)
{
    QString error;
    const bool saved = [&]() {
        QWriteLocker lock(&db_rwlock);
        if (!m_sqliteDb.transaction()) {
            error = m_sqliteDb.lastError().text();
            return false;
        }
        auto fail = [&](const QSqlError& sqlError) {
            error = sqlError.text();
            m_sqliteDb.rollback();
            return false;
        };
        QSqlQuery select(m_sqliteDb);
        if (!select.prepare("SELECT id FROM application_settings WHERE parameter_name = ? AND deleted = 0 "
                            "ORDER BY COALESCE(updateddate, 0) DESC, id LIMIT 1"))
            return fail(select.lastError());
        select.addBindValue(parametername);
        if (!select.exec()) return fail(select.lastError());
        const QString id = select.next() ? select.value(0).toString() : QString();
        select.finish();

        QSqlQuery write(m_sqliteDb);
        if (id.isEmpty()) {
            if (!write.prepare("INSERT INTO application_settings(id, parameter_name, parameter_value) VALUES(?, ?, ?)"))
                return fail(write.lastError());
            write.addBindValue(QUuid::createUuid().toString());
            write.addBindValue(parametername);
            write.addBindValue(parametervalue);
        } else {
            if (!write.prepare("UPDATE application_settings SET parameter_value = ? WHERE id = ? AND deleted = 0"))
                return fail(write.lastError());
            write.addBindValue(parametervalue);
            write.addBindValue(id);
        }
        if (!write.exec()) return fail(write.lastError());
        if (!m_sqliteDb.commit()) return fail(m_sqliteDb.lastError());
        return true;
    }();
    m_lastSaveError = error;
    // Show errors only after releasing the lock; modal dialogs process events.
    if (!saved && m_gui && m_guiDialogsEnabled)
        QMessageBox::critical(nullptr, tr("Database Access Failed"),
                              tr("Failed to save a setting.\n\nError:\n%1").arg(error));
    return saved;
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

QString DatabaseObjects::loadParameter(const QVariant& parametername)
{
    QString error;
    const QString value = [&]() {
        QWriteLocker lock(&db_rwlock);
        QSqlQuery select(m_sqliteDb);
        if (!select.prepare("SELECT parameter_value FROM application_settings "
                            "WHERE parameter_name = ? AND deleted = 0 "
                            "ORDER BY COALESCE(updateddate, 0) DESC, id LIMIT 1")) {
            error = select.lastError().text();
            return QString();
        }
        select.addBindValue(parametername);
        if (!select.exec()) {
            error = select.lastError().text();
            return QString();
        }
        return select.next() ? select.value(0).toString() : QString();
    }();
    if (!error.isEmpty() && m_gui && m_guiDialogsEnabled)
        QMessageBox::critical(nullptr, tr("Database Access Failed"),
                              tr("Failed to read a saved setting.\n\nError:\n%1").arg(error));
    return value;
}

void DatabaseObjects::setShowResolvedTrackerItems(bool value)
{
    saveLocalParameter("ViewFilter:ShowResolvedTrackerItems", (value ? "1": "0"));
}

bool DatabaseObjects::getShowResolvedTrackerItems()
{
    QString value = loadLocalParameter("ViewFilter:ShowResolvedTrackerItems");
    bool ret = (bool)value.toUInt();
    return ret;
}

void DatabaseObjects::setShowClosedProjects(bool value)
{
    saveLocalParameter("UserFilter:ShowClosedProjects", (value ? "1": "0"));
    emit showClosedProjectsChanged(value);
}

bool DatabaseObjects::getShowClosedProjects()
{
    QString value = loadLocalParameter("UserFilter:ShowClosedProjects");
    bool ret = (bool)value.toUInt();
    return ret;
}

void DatabaseObjects::setShowInternalItems(bool value)
{
    saveLocalParameter("UserFilter:ShowInternalItems", (value ? "1": "0"));
}

bool DatabaseObjects::getShowInternalItems()
{
    QString value = loadLocalParameter("UserFilter:ShowInternalItems");
    bool ret = (bool)value.toUInt();
    return ret;
}

void DatabaseObjects::setOffice365OpenLinksInDesktop(bool value)
{
    saveLocalParameter("Office365:OpenLinksInDesktop", (value ? "1": "0"));
}

bool DatabaseObjects::getOffice365OpenLinksInDesktop()
{
    QString value = loadLocalParameter("Office365:OpenLinksInDesktop");
    if (value.isEmpty())
        return true; // unset: default to trying the native desktop app first, matching desktop
    return (bool)value.toUInt();
}

void DatabaseObjects::setProjectManager(const QString& value)
{
    saveParameter("Preferences:ProjectManager", value );
}

QString DatabaseObjects::getProjectManager()
{
    return loadParameter("Preferences:ProjectManager");
}

void DatabaseObjects::setManagingCompany(const QString& value)
{
    saveParameter("Preferences:ManagingCompany", value );
}

QString DatabaseObjects::getManagingCompany()
{
    return loadParameter("Preferences:ManagingCompany");
}

void DatabaseObjects::setLastProjectDetailTab(const QString& projectId, int index)
{
    saveParameter("UI:LastProjectDetailTab:" + projectId, QString::number(index));
}

int DatabaseObjects::getLastProjectDetailTab(const QString& projectId)
{
    QString value = loadParameter("UI:LastProjectDetailTab:" + projectId);
    if (value.isEmpty())
        return 1; // Tracker tab
    return value.toInt();
}

void DatabaseObjects::setProjectDetailHeaderHeight(int height)
{
    saveLocalParameter("UI:ProjectDetailHeaderHeight", QString::number(height));
}

int DatabaseObjects::getProjectDetailHeaderHeight()
{
    QString value = loadLocalParameter("UI:ProjectDetailHeaderHeight");
    if (value.isEmpty())
        return 0; // no saved preference
    return value.toInt();
}

void DatabaseObjects::setProjectSidebarWidth(int width)
{
    saveLocalParameter("UI:ProjectSidebarWidth", QString::number(width));
}

int DatabaseObjects::getProjectSidebarWidth()
{
    QString value = loadLocalParameter("UI:ProjectSidebarWidth");
    if (value.isEmpty())
        return 0; // no saved preference — caller keeps its built-in default
    return value.toInt();
}

void DatabaseObjects::setProjectSidebarVisible(bool value)
{
    saveLocalParameter("UI:ProjectSidebarVisible", (value ? "1" : "0"));
}

bool DatabaseObjects::getProjectSidebarVisible()
{
    QString value = loadLocalParameter("UI:ProjectSidebarVisible");
    if (value.isEmpty())
        return true; // no saved preference — the panel starts out visible
    return (bool)value.toUInt();
}

void DatabaseObjects::setUiZoomPercent(int percent)
{
    saveLocalParameter("UI:ZoomPercent", QString::number(percent));
}

int DatabaseObjects::getUiZoomPercent()
{
    QString value = loadLocalParameter("UI:ZoomPercent");
    if (value.isEmpty())
        return 0; // no saved preference — caller keeps its built-in default
    return value.toInt();
}

void DatabaseObjects::setGlobalSearches( bool refresh )
{ 
    // setup default filters
    if (getShowClosedProjects())
    {
        projectinformationmodel()->clearFilter(14);
        projectslistmodel()->clearFilter(14);
        searchresultsmodel()->clearFilter(6);
        allitemsmodel()->clearFilter(17);
    }
    else
    {
        projectinformationmodel()->setFilter(14, tr("Active"));
        projectslistmodel()->setFilter(14, tr("Active"));
        searchresultsmodel()->setFilter(6, tr("Active"));
        allitemsmodel()->setFilter(17, tr("Active"));
    }

    if (getShowInternalItems())
    {
        projectnotesmodel()->clearFilter(5);
        actionitemsdetailsmeetingsmodel()->clearFilter(3);
        notesactionitemsmodel()->clearFilter(15);
        actionitemprojectnotesmodel()->clearFilter(3);
        trackeritemsmodel()->clearFilter(15);
        allitemsmodel()->clearFilter(15);
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
        allitemsmodel()->setFilter(15, "0");
        actionitemsdetailsmodel()->setFilter(15, "0");
        searchresultsmodel()->setFilter(4, "0");
    }

    if (!getShowResolvedTrackerItems())
    {
        trackeritemsmodel()->setFilter(9, "New,Assigned", SqlQueryModel::In);
        allitemsmodel()->setFilter(9, "New,Assigned", SqlQueryModel::In);
    }
    else
    {
        trackeritemsmodel()->clearFilter(9);
        allitemsmodel()->clearFilter(9);
    }

    projectslistmodel()->clearFilter(0);
    searchresultsmodel()->clearFilter(7);

    if (refresh)
    {
        contactsmodel()->markDirty();
        companiesmodel()->markDirty();
        projectnotesmodel()->markDirty();
        actionitemsdetailsmeetingsmodel()->markDirty();
        notesactionitemsmodel()->markDirty();
        actionitemprojectnotesmodel()->markDirty();
        actionitemsdetailsmodel()->markDirty();

        trackeritemsmodel()->markDirty();
        allitemsmodel()->markDirty();
        projectinformationmodel()->markDirty();
        projectslistmodel()->markDirty();
        unfilteredprojectslistmodel()->markDirty();
        searchresultsmodel()->markDirty();
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

    QString companyname = execute(QString("select client_name from clients where id='%1'").arg(getManagingCompany()));
    QString managername = execute(QString("select name from people where id='%1'").arg(getProjectManager()));

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

    QString companyname = execute(QString("select client_name from clients where id='%1'").arg(getManagingCompany()));
    QString managername = execute(QString("select name from people where id='%1'").arg(getProjectManager()));

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
    return importRecordTree(RecordTreeXml::treeFromDocument(xmldoc));
}

bool DatabaseObjects::importRecordTree(const RecordTree& tree, QList<RecordImportOutcome>* outcomes)
{
    // Tables are applied parents first (companies, contacts, projects, then
    // their children), each table's rows in document order, wherever the table
    // appears in the tree. A row rejected for an invalid value stops the import.
    for (const QString& name : RecordTreeImport::tableOrder())
    {
        const QList<const RecordTable*> tables = RecordTreeImport::tablesNamed(tree, name);
        if (tables.isEmpty())
            continue;

#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, QString("importRecordTree processing %1.").arg(name));
#endif
        QScopedPointer<SqlQueryModel> model(createExportObject(name));
        if (!model)
            continue;

        for (const RecordTable* table : tables)
        {
            for (const RecordRow& row : table->rows)
            {
                RecordImportOutcome outcome;
                const bool ok = model->importRecordRow(row, &outcome);
                if (outcomes)
                    outcomes->append(outcome);
                if (!ok)
                    return false;
            }
        }
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
            SqlQueryModel* model = new CompaniesModel(this);
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
            SqlQueryModel* model = new ContactsModel(this);
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

    // import project email audiences
    domlist = findTableNodes(root, "project_email_audiences");
    if (!domlist.empty())
    {
        for (QDomNode& tablenode : domlist)
        {
            SqlQueryModel* model = new ProjectEmailAudiencesModel(this);
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

    // No default Project Manager configured: do not insert a project_people row
    // at all. Inserting with an empty people_id creates a junk team-member record
    // (empty string passes the NOT NULL constraint) that later fails on XML
    // export/import when it round-trips to a real NULL.
    if (pm.trimmed().isEmpty())
        return;

    QString guid = QUuid::createUuid().toString();

    QString insert = QString("insert into project_people (id, people_id, project_id, role) select '%3', '%2', '%1', 'Project Manager' where not exists (select 1 from project_people where project_id = '%1' and people_id = '%2' and deleted = 0 )").arg(projectId).arg(pm).arg(guid);

    execute(insert);
}

void DatabaseObjects::addDefaultPMToMeeting(const QString& noteId)
{
    QString pm = getProjectManager();

    // No default Project Manager configured: skip both the team-member and the
    // meeting-attendee inserts rather than writing rows with an empty people_id /
    // person_id (see addDefaultPMToProject).
    if (pm.trimmed().isEmpty())
        return;

    QString guid = QUuid::createUuid().toString();
    QString guid2 = QUuid::createUuid().toString();

    QString project_id = execute(QString("select project_id from project_notes where id='%1'").arg(noteId));
    QString insertpm = QString("insert into project_people (id, people_id, project_id, role) select '%3', '%2', '%1', 'Project Manager' where not exists (select 1 from project_people where project_id = '%1' and people_id = '%2' and deleted = 0 )").arg(project_id).arg(pm).arg(guid2);

    execute(insertpm);

    QString insert = QString("insert into meeting_attendees (id, person_id, note_id) select '%3', '%2', '%1' where not exists (select 1 from meeting_attendees where note_id = '%1' and person_id = '%2' and deleted = 0 )").arg(noteId).arg(pm).arg(guid);

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
    if (!m_gui)
    {
        // Non-GUI instance (e.g. plugin thread): emit signal so a QueuedConnection
        // can forward the change to global_DBObjects on the GUI thread.
        emit rowChanged(table, value, static_cast<int>(optype));
        return;
    }

    // Any write can change a value other proxies resolve through their lookup
    // caches (e.g. a person rename read by the items list's assigned-to sort).
    invalidateLookupCaches(table);

    KeyColumnChange newChange{table, value, optype};
    if (!m_keyColumnChanges.contains(newChange))
    {
        m_keyColumnChanges.append(newChange);
    }

    // qDebug() << "Push change to " << table << " of id " << value << " to change stack.";
}

void DatabaseObjects::invalidateLookupCaches(const QString& table)
{
    // All proxies are direct children of this object (created in
    // createDatabaseObjects); each one drops only entries for this table.
    const auto proxies = findChildren<SortFilterProxyModel*>(QString(), Qt::FindDirectChildrenOnly);
    for (SortFilterProxyModel* proxy : proxies)
        proxy->invalidateLookupTable(table);
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

void DatabaseObjects::updateDisplayData()
{
    KeyColumnChange keyColChange;

    if (!m_keyColumnChanges.isEmpty()) {
        QVariantList pending;
        pending.reserve(m_keyColumnChanges.size());
        for (const KeyColumnChange& change : std::as_const(m_keyColumnChanges))
            pending.append(QVariantMap{{QStringLiteral("table"), change.table}, {QStringLiteral("key"), change.value},
                                       {QStringLiteral("operation"), int(change.operation_type)}});
        emit displayChangesPending(pending);
    }

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
                recordset->refreshIfDirty();

                if (keyColChange.operation_type == KeyColumnChange::Update)
                {
                    QModelIndex qmi = recordset->findIndex(keyColChange.value, 0);
                    if (qmi.isValid())
                    {
                        // qDebug() << "Updating display for table " << recordset->tablename() << " row " << qmi.row() << " with value " << keyColChange.value;
                        if (!recordset->reloadRecord(qmi))
                        {
                            // Record no longer passes base filters (e.g. was soft-deleted
                            // via sync on another device) — remove it from the view.
                            recordset->removeCacheRecord(qmi);
                        }
                    }
                    else
                    {
                        // Row not in model yet — may be a new record pulled from sync.
                        // Attempt to load it; loadAndFilterRow is a no-op if it doesn't
                        // pass the model's current filter.
                        recordset->loadAndFilterRow(keyColChange.value);
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
