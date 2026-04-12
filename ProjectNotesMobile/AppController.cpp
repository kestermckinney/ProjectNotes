// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "AppController.h"

#include "QLogger.h"
#include "QLoggerWriter.h"

#include "sqlitesyncpro.h"

#include <QDir>
#include <QFileInfo>
#include <QTimer>

using namespace QLogger;

// ── Singleton plumbing ───────────────────────────────────────────────────────

static AppController* s_instance = nullptr;

AppController* AppController::create(QQmlEngine* /*engine*/, QJSEngine* /*scriptEngine*/)
{
    if (!s_instance)
        s_instance = new AppController();
    return s_instance;
}

// ── Constructor / Destructor ─────────────────────────────────────────────────

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    // Set up structured logging to the app data directory
    const QString logPath = MobileSettings::dataLocation() + "/logs";
    QDir().mkpath(logPath);

    QLoggerManager *logmanager = QLoggerManager::getInstance();

#ifdef QT_DEBUG
    logmanager->addDestination("debugging.log", DEBUGLOG, LogLevel::Debug, logPath, LogMode::OnlyFile);
#endif

    logmanager->addDestination("error.log", ERRORLOG, LogLevel::Error, logPath, LogMode::OnlyFile);

    logmanager->resume();

    // Auto-start sync whenever sync settings change and are now complete
    connect(this, &AppController::syncSettingsChanged,
            this, &AppController::onSyncSettingsChanged);
}

AppController::~AppController()
{
    stopSync();
    global_DBObjects.closeDatabase();
}

// ── Database ─────────────────────────────────────────────────────────────────

bool AppController::openOrCreateDatabase()
{
    const QString dataDir  = MobileSettings::dataLocation();
    const QString dbPath   = dataDir + "/ProjectNotes.db";
    const QString connName = "ProjectNotesMobile";

    QDir().mkpath(dataDir);

    const bool isNewDatabase = !QFileInfo::exists(dbPath);

    if (isNewDatabase) {
        QLog_Debug(DEBUGLOG, QString("Creating new database: %1").arg(dbPath));
        if (!global_DBObjects.createDatabase(dbPath)) {
            emit errorOccurred(tr("Database Error"), tr("Failed to create database at %1").arg(dbPath));
            return false;
        }
    }

    if (!global_DBObjects.openDatabase(dbPath, connName, false)) {
        emit errorOccurred(tr("Database Error"), tr("Failed to open database at %1").arg(dbPath));
        return false;
    }

    QLog_Debug(DEBUGLOG, QString("Database opened: %1").arg(dbPath));

    // On first install set mobile-friendly defaults before applying filters.
    // On subsequent launches the values stored in the database are used as-is.
    if (isNewDatabase)
        global_DBObjects.setShowClosedProjects(true);

    // Apply filters and populate all models before notifying QML.
    // setGlobalSearches(false) wires up filter state without running queries,
    // then setGlobalSearches(true) runs the actual SQL in dependency order.
    global_DBObjects.setGlobalSearches(false);
    global_DBObjects.setGlobalSearches(true);

    // Tell QML the view-option properties are now readable.
    emit viewOptionsChanged();
    emit databaseReady();
    return true;
}

// ── Sync ─────────────────────────────────────────────────────────────────────

void AppController::configureSyncApi()
{
    if (!m_syncApi) {
        m_syncApi = new SqliteSyncPro(this);
        connect(m_syncApi, &SqliteSyncPro::syncCompleted,
                this,      &AppController::onSyncComplete);
        connect(m_syncApi, &SqliteSyncPro::syncProgress,
                this,      &AppController::onSyncProgress);
        connect(m_syncApi, &SqliteSyncPro::syncStatusUpdated,
                this,      &AppController::onSyncStatusUpdated,
                Qt::QueuedConnection);

        // Mobile-friendly limits: smaller batches to conserve bandwidth and
        // battery; longer interval between cycles than the desktop default.
        m_syncApi->setDefaultBatchSize(25);
        m_syncApi->setSyncIntervalMs(30000);  // 30 s between cycles
    }

    m_syncApi->setSyncHostType(global_MobileSettings.getSyncHostType());
    m_syncApi->setPostgrestUrl(global_MobileSettings.getSyncPostgrestUrl());
    m_syncApi->setEmail(global_MobileSettings.getSyncEmail());
    m_syncApi->setPassword(global_MobileSettings.getSyncPassword());
    m_syncApi->setEncryptionPhrase(global_MobileSettings.getSyncEncryptionPhrase());
    m_syncApi->setSupabaseKey(global_MobileSettings.getSyncSupabaseKey());
}

void AppController::startSync()
{
    if (!global_MobileSettings.getSyncEnabled()) return;
    if (!global_DBObjects.isOpen()) return;

    configureSyncApi();

    // If a sync loop is already running, wake it for an immediate cycle instead
    // of calling initialize() again. Re-initializing while the loop thread is
    // live would overwrite m_syncWorker/m_syncThread; the old thread's cleanup
    // lambda would then delete the NEW worker, causing a crash in run().
    if (m_syncApi->isInitialized()) {
        m_syncApi->retryNow();
        return;
    }

    m_syncApi->setDatabasePath(MobileSettings::dataLocation() + "/ProjectNotes.db");
    if (!m_syncApi->initialize()) {
        // Initialization failed — show red bar so the user knows sync is broken.
        setSyncProgress(0.0, true);
    }
}

void AppController::stopSync()
{
    if (m_syncApi)
        m_syncApi->shutdown();
}

void AppController::onSyncComplete(const SyncResult& result)
{
    if (result.success) {
        m_syncHasError = false;
        // Ask SqliteSyncPro to count remaining pending records and emit
        // syncStatusUpdated — same as the desktop.  That signal is the sole
        // authority on whether the bar shows or hides after a cycle.
        m_syncApi->checkSyncStatus(result);
    } else {
        // Turn bar red; no popup — the bar is the only sync error indicator.
        setSyncProgress(m_syncProgress, true);
    }
}

void AppController::onSyncProgress(const QString& /*tableName*/, int /*processed*/, int /*total*/)
{
    // The engine always emits total = -1 (unknown), so we can't compute a real percentage here.
    // Just ensure the bar is visible while records are flowing.  If it's already showing a
    // real percentage from the last checkSyncStatus, leave it alone so it doesn't jump.
    if (m_syncProgress < 0)
        setSyncProgress(0.01, m_syncHasError);
}

void AppController::onSyncStatusUpdated(int percentComplete, qint64 /*pendingPush*/, qint64 /*pendingPull*/)
{
    // Mirror the desktop: show bar while percentComplete < 100, hide at 100.
    // We never force the bar to 0 on cycle start — it only appears when
    // SqliteSyncPro actually has pending records to push or pull.
    if (percentComplete >= 100) {
        // Fully synced — hide the bar (and clear any prior error colour).
        setSyncProgress(-1.0, false);
    } else {
        setSyncProgress(percentComplete / 100.0, m_syncHasError);
    }
}

void AppController::onSyncSettingsChanged()
{
    if (!global_MobileSettings.getSyncEnabled())      return;
    if (!global_DBObjects.isOpen())                   return;
    if (global_MobileSettings.getSyncPostgrestUrl().isEmpty()) return;
    if (global_MobileSettings.getSyncEmail().isEmpty())        return;
    if (global_MobileSettings.getSyncPassword().isEmpty())     return;

    // Defer so the property setter call stack unwinds before startSync() runs
    QTimer::singleShot(0, this, &AppController::startSync);
}

void AppController::setSyncProgress(qreal progress, bool hasError)
{
    if (qFuzzyCompare(m_syncProgress, progress) && m_syncHasError == hasError)
        return;
    m_syncProgress = progress;
    m_syncHasError = hasError;
    emit syncProgressChanged();
}

// ── Filter helpers ───────────────────────────────────────────────────────────

void AppController::applyFilterToProjectsList(bool showClosed)
{
    global_DBObjects.setShowClosedProjects(showClosed);
    global_DBObjects.setGlobalSearches(true);
    emit viewOptionsChanged();
}

void AppController::setPeopleFilter(const QString& filter)
{
    if (filter.isEmpty())
        global_DBObjects.peoplemodel()->clearUserSearchString(1);
    else
        global_DBObjects.peoplemodel()->setUserSearchString(1, filter);
    global_DBObjects.peoplemodel()->refresh();
}

void AppController::setClientsFilter(const QString& filter)
{
    if (filter.isEmpty())
        global_DBObjects.clientsmodel()->clearUserSearchString(1);
    else
        global_DBObjects.clientsmodel()->setUserSearchString(1, filter);
    global_DBObjects.clientsmodel()->refresh();
}

void AppController::setProjectFilter(const QString& projectId)
{
    // Status report items: project_id is col 1
    global_DBObjects.statusreportitemsmodel()->setFilter(1, projectId);
    global_DBObjects.statusreportitemsmodel()->refresh();

    // Team members: project_id is col 1
    global_DBObjects.projectteammembersmodel()->setFilter(1, projectId);
    global_DBObjects.projectteammembersmodel()->refresh();

    // Tracker items (project view): project_id is col 14
    global_DBObjects.trackeritemsmodel()->setFilter(14, projectId);
    global_DBObjects.trackeritemsmodel()->refresh();

    // Project locations: project_id is col 1
    global_DBObjects.projectlocationsmodel()->setFilter(1, projectId);
    global_DBObjects.projectlocationsmodel()->refresh();

    // Project notes: project_id is col 1
    global_DBObjects.projectnotesmodel()->setFilter(1, projectId);
    global_DBObjects.projectnotesmodel()->refresh();
}

void AppController::setAllItemsFilter(const QString& filter)
{
    SqlQueryModel* model = global_DBObjects.allitemsmodel();
    if (filter.isEmpty()) {
        model->clearUserSearchString(3);  // item_name col 3
    } else {
        model->setUserSearchString(3, filter);
    }
    model->refresh();
}

// ── View options ─────────────────────────────────────────────────────────────

void AppController::setNewAndAssignedOnly(bool v)
{
    // v = true  → filter tracker items to New and Assigned status only
    // v = false → show all tracker items regardless of status
    global_DBObjects.setShowResolvedTrackerItems(!v);

    if (v) {
        global_DBObjects.trackeritemsmodel()->setFilter(9, "New,Assigned", SqlQueryModel::In);
        global_DBObjects.allitemsmodel()->setFilter(9, "New,Assigned", SqlQueryModel::In);
    } else {
        global_DBObjects.trackeritemsmodel()->clearFilter(9);
        global_DBObjects.allitemsmodel()->clearFilter(9);
    }

    global_DBObjects.trackeritemsmodel()->refresh();
    global_DBObjects.allitemsmodel()->refresh();
    emit viewOptionsChanged();
}

// ── Sync All ─────────────────────────────────────────────────────────────────

void AppController::syncAll()
{
    if (!global_MobileSettings.getSyncEnabled()) return;
    if (!global_DBObjects.isOpen()) return;

    configureSyncApi();
    m_syncApi->setDatabasePath(MobileSettings::dataLocation() + "/ProjectNotes.db");

    if (!m_syncApi->isInitialized()) {
        if (!m_syncApi->initialize()) {
            setSyncProgress(0.0, true);  // red bar — no popup
            return;
        }
    }

    m_syncApi->syncAll();
}

// ── Record editing ────────────────────────────────────────────────────────────

bool AppController::savePerson(int row, const QString& name, const QString& email,
                                const QString& officePhone, const QString& cellPhone,
                                const QString& clientId, const QString& role)
{
    QAbstractItemModel* model = global_DBObjects.peoplemodelproxy();
    if (row < 0 || row >= model->rowCount())
        return false;
    bool ok = true;
    ok &= model->setData(model->index(row, 1), name);
    ok &= model->setData(model->index(row, 2), email);
    ok &= model->setData(model->index(row, 3), officePhone);
    ok &= model->setData(model->index(row, 4), cellPhone);
    ok &= model->setData(model->index(row, 5), clientId);
    ok &= model->setData(model->index(row, 6), role);
    return ok;
}

bool AppController::saveProject(int row, const QString& projectNumber,
                                 const QString& projectName, const QString& projectStatus,
                                 const QString& clientId, const QString& lastStatusDate,
                                 const QString& lastInvoiceDate, const QString& invoicingPeriod,
                                 const QString& statusReportPeriod)
{
    QAbstractItemModel* model = global_DBObjects.projectslistmodelproxy();
    if (row < 0 || row >= model->rowCount())
        return false;
    bool ok = true;
    ok &= model->setData(model->index(row,  1), projectNumber);
    ok &= model->setData(model->index(row,  2), projectName);
    ok &= model->setData(model->index(row,  3), lastStatusDate);
    ok &= model->setData(model->index(row,  4), lastInvoiceDate);
    ok &= model->setData(model->index(row, 11), invoicingPeriod);
    ok &= model->setData(model->index(row, 12), statusReportPeriod);
    ok &= model->setData(model->index(row, 13), clientId);
    ok &= model->setData(model->index(row, 14), projectStatus);
    return ok;
}

bool AppController::saveStatusItem(int row, const QString& category, const QString& description)
{
    QAbstractItemModel* model = global_DBObjects.statusreportitemsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = true;
    ok &= model->setData(model->index(row, 2), category);
    ok &= model->setData(model->index(row, 3), description);
    return ok;
}

bool AppController::saveTeamMember(int row, const QString& peopleId, const QString& role, bool receiveStatusReport)
{
    QAbstractItemModel* model = global_DBObjects.projectteammembersmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = true;
    ok &= model->setData(model->index(row, 2), peopleId);
    ok &= model->setData(model->index(row, 4), receiveStatusReport ? "1" : "0");
    ok &= model->setData(model->index(row, 5), role);
    return ok;
}

bool AppController::saveProjectLocation(int row, const QString& locationType,
                                         const QString& description, const QString& path)
{
    QAbstractItemModel* model = global_DBObjects.projectlocationsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = true;
    ok &= model->setData(model->index(row, 2), locationType);
    ok &= model->setData(model->index(row, 3), description);
    ok &= model->setData(model->index(row, 4), path);
    return ok;
}

bool AppController::saveProjectNote(int row, const QString& title, const QString& date,
                                     const QString& note, bool internalItem)
{
    QAbstractItemModel* model = global_DBObjects.projectnotesmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = true;
    ok &= model->setData(model->index(row, 2), title);
    ok &= model->setData(model->index(row, 3), date);
    ok &= model->setData(model->index(row, 4), note);
    ok &= model->setData(model->index(row, 5), internalItem ? "1" : "0");
    return ok;
}

bool AppController::saveClient(int row, const QString& clientName)
{
    QAbstractItemModel* model = global_DBObjects.clientsmodelproxy();
    if (row < 0 || row >= model->rowCount())
        return false;
    return model->setData(model->index(row, 1), clientName);
}

int AppController::clientRowForId(const QString& clientId) const
{
    QAbstractItemModel* model = global_DBObjects.clientsmodelproxy();
    for (int row = 0; row < model->rowCount(); ++row) {
        if (model->data(model->index(row, 0)).toString() == clientId)
            return row;
    }
    return -1;
}

QString AppController::clientIdAtRow(int row) const
{
    QAbstractItemModel* model = global_DBObjects.clientsmodelproxy();
    if (row < 0 || row >= model->rowCount())
        return {};
    return model->data(model->index(row, 0)).toString();
}

int AppController::peopleRowForId(const QString& peopleId) const
{
    QAbstractItemModel* model = global_DBObjects.peoplemodelproxy();
    for (int row = 0; row < model->rowCount(); ++row) {
        if (model->data(model->index(row, 0)).toString() == peopleId)
            return row;
    }
    return -1;
}

QString AppController::peopleIdAtRow(int row) const
{
    QAbstractItemModel* model = global_DBObjects.peoplemodelproxy();
    if (row < 0 || row >= model->rowCount())
        return {};
    return model->data(model->index(row, 0)).toString();
}

// ── Add / Delete / Copy helpers ───────────────────────────────────────────────

// Map source model index to proxy row after a newRecord() call.
// Returns the proxy row, or srcIdx.row() if the proxy maps it to an invalid index
// (e.g., the new record is filtered out — should not happen for newly created rows).
static int proxyRowFromSource(SortFilterProxyModel* proxy, const QModelIndex& srcIdx)
{
    if (!srcIdx.isValid()) return -1;
    QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
    return proxyIdx.isValid() ? proxyIdx.row() : srcIdx.row();
}

// Delete the record at proxy row |row| via the source model.
static bool deleteProxyRow(SortFilterProxyModel* proxy, SqlQueryModel* source, int row)
{
    QModelIndex proxyIdx = proxy->index(row, 0);
    if (!proxyIdx.isValid()) return false;
    return source->deleteRecord(proxy->mapToSource(proxyIdx));
}

// Copy the record at proxy row |row|, return the new proxy row.
static int copyProxyRow(SortFilterProxyModel* proxy, SqlQueryModel* source, int row)
{
    QModelIndex proxyIdx = proxy->index(row, 0);
    if (!proxyIdx.isValid()) return -1;
    QModelIndex newSrc = source->copyRecord(proxy->mapToSource(proxyIdx));
    return proxyRowFromSource(proxy, newSrc);
}

// Read all named-column roles (Qt::UserRole+n) from |proxy| at |row| into a map.
static QVariantMap proxyRowToMap(SortFilterProxyModel* proxy, int row)
{
    if (row < 0 || row >= proxy->rowCount()) return {};
    QVariantMap result;
    const auto roles = proxy->roleNames();
    const QModelIndex idx = proxy->index(row, 0);
    for (auto it = roles.constBegin(); it != roles.constEnd(); ++it) {
        if (it.key() >= Qt::UserRole)
            result[QString::fromUtf8(it.value())] = proxy->data(idx, it.key());
    }
    return result;
}

// ── Projects ──────────────────────────────────────────────────────────────────

int AppController::addProject()
{
    return proxyRowFromSource(global_DBObjects.projectslistmodelproxy(),
                              global_DBObjects.projectslistmodel()->newRecord());
}

bool AppController::deleteProject(int row)
{
    return deleteProxyRow(global_DBObjects.projectslistmodelproxy(),
                          global_DBObjects.projectslistmodel(), row);
}

int AppController::copyProject(int row)
{
    return copyProxyRow(global_DBObjects.projectslistmodelproxy(),
                        global_DBObjects.projectslistmodel(), row);
}

QVariantMap AppController::getProjectData(int row) const
{
    return proxyRowToMap(global_DBObjects.projectslistmodelproxy(), row);
}

// ── People ────────────────────────────────────────────────────────────────────

int AppController::addPerson()
{
    return proxyRowFromSource(global_DBObjects.peoplemodelproxy(),
                              global_DBObjects.peoplemodel()->newRecord());
}

bool AppController::deletePerson(int row)
{
    return deleteProxyRow(global_DBObjects.peoplemodelproxy(),
                          global_DBObjects.peoplemodel(), row);
}

int AppController::copyPerson(int row)
{
    return copyProxyRow(global_DBObjects.peoplemodelproxy(),
                        global_DBObjects.peoplemodel(), row);
}

QVariantMap AppController::getPersonData(int row) const
{
    return proxyRowToMap(global_DBObjects.peoplemodelproxy(), row);
}

// ── Clients ───────────────────────────────────────────────────────────────────

int AppController::addClient()
{
    return proxyRowFromSource(global_DBObjects.clientsmodelproxy(),
                              global_DBObjects.clientsmodel()->newRecord());
}

bool AppController::deleteClient(int row)
{
    return deleteProxyRow(global_DBObjects.clientsmodelproxy(),
                          global_DBObjects.clientsmodel(), row);
}

int AppController::copyClient(int row)
{
    return copyProxyRow(global_DBObjects.clientsmodelproxy(),
                        global_DBObjects.clientsmodel(), row);
}

QVariantMap AppController::getClientData(int row) const
{
    return proxyRowToMap(global_DBObjects.clientsmodelproxy(), row);
}

// ── Status Report Items ───────────────────────────────────────────────────────

int AppController::addStatusItem(const QString& projectId)
{
    QVariant fk(projectId);
    return proxyRowFromSource(global_DBObjects.statusreportitemsmodelproxy(),
                              global_DBObjects.statusreportitemsmodel()->newRecord(&fk));
}

bool AppController::deleteStatusItem(int row)
{
    return deleteProxyRow(global_DBObjects.statusreportitemsmodelproxy(),
                          global_DBObjects.statusreportitemsmodel(), row);
}

int AppController::copyStatusItem(int row)
{
    return copyProxyRow(global_DBObjects.statusreportitemsmodelproxy(),
                        global_DBObjects.statusreportitemsmodel(), row);
}

QVariantMap AppController::getStatusItemData(int row) const
{
    return proxyRowToMap(global_DBObjects.statusreportitemsmodelproxy(), row);
}

// ── Project Team Members ──────────────────────────────────────────────────────

int AppController::addTeamMember(const QString& projectId)
{
    QVariant fk(projectId);
    return proxyRowFromSource(global_DBObjects.projectteammembersmodelproxy(),
                              global_DBObjects.projectteammembersmodel()->newRecord(&fk));
}

bool AppController::deleteTeamMember(int row)
{
    return deleteProxyRow(global_DBObjects.projectteammembersmodelproxy(),
                          global_DBObjects.projectteammembersmodel(), row);
}

int AppController::copyTeamMember(int row)
{
    return copyProxyRow(global_DBObjects.projectteammembersmodelproxy(),
                        global_DBObjects.projectteammembersmodel(), row);
}

QVariantMap AppController::getTeamMemberData(int row) const
{
    return proxyRowToMap(global_DBObjects.projectteammembersmodelproxy(), row);
}

// ── Project Locations ─────────────────────────────────────────────────────────

int AppController::addProjectLocation(const QString& projectId)
{
    QVariant fk(projectId);
    return proxyRowFromSource(global_DBObjects.projectlocationsmodelproxy(),
                              global_DBObjects.projectlocationsmodel()->newRecord(&fk));
}

bool AppController::deleteProjectLocation(int row)
{
    return deleteProxyRow(global_DBObjects.projectlocationsmodelproxy(),
                          global_DBObjects.projectlocationsmodel(), row);
}

int AppController::copyProjectLocation(int row)
{
    return copyProxyRow(global_DBObjects.projectlocationsmodelproxy(),
                        global_DBObjects.projectlocationsmodel(), row);
}

QVariantMap AppController::getProjectLocationData(int row) const
{
    return proxyRowToMap(global_DBObjects.projectlocationsmodelproxy(), row);
}

// ── Project Notes ─────────────────────────────────────────────────────────────

int AppController::addProjectNote(const QString& projectId)
{
    QVariant fk(projectId);
    return proxyRowFromSource(global_DBObjects.projectnotesmodelproxy(),
                              global_DBObjects.projectnotesmodel()->newRecord(&fk));
}

bool AppController::deleteProjectNote(int row)
{
    return deleteProxyRow(global_DBObjects.projectnotesmodelproxy(),
                          global_DBObjects.projectnotesmodel(), row);
}

int AppController::copyProjectNote(int row)
{
    return copyProxyRow(global_DBObjects.projectnotesmodelproxy(),
                        global_DBObjects.projectnotesmodel(), row);
}

QVariantMap AppController::getProjectNoteData(int row) const
{
    return proxyRowToMap(global_DBObjects.projectnotesmodelproxy(), row);
}

// ── Preferences helpers ───────────────────────────────────────────────────────

int AppController::managingCompanyIndex() const
{
    const QString id = global_DBObjects.getManagingCompany();
    if (id.isEmpty())
        return 0;
    QAbstractItemModel* model = global_DBObjects.clientsmodelproxy();
    for (int row = 0; row < model->rowCount(); ++row) {
        if (model->data(model->index(row, 0)).toString() == id)
            return row;
    }
    return 0;
}

void AppController::setManagingCompanyByRow(int row)
{
    QAbstractItemModel* model = global_DBObjects.clientsmodelproxy();
    if (row < 0 || row >= model->rowCount())
        return;
    global_DBObjects.setManagingCompany(model->data(model->index(row, 0)).toString());
}

int AppController::projectManagerIndex() const
{
    const QString id = global_DBObjects.getProjectManager();
    if (id.isEmpty())
        return 0;
    QAbstractItemModel* model = global_DBObjects.peoplemodelproxy();
    for (int row = 0; row < model->rowCount(); ++row) {
        if (model->data(model->index(row, 0)).toString() == id)
            return row;
    }
    return 0;
}

void AppController::setProjectManagerByRow(int row)
{
    QAbstractItemModel* model = global_DBObjects.peoplemodelproxy();
    if (row < 0 || row >= model->rowCount())
        return;
    global_DBObjects.setProjectManager(model->data(model->index(row, 0)).toString());
}

// ── Model accessors ──────────────────────────────────────────────────────────

QAbstractItemModel* AppController::projectsListModel() const
{
    return global_DBObjects.projectslistmodelproxy();
}

QAbstractItemModel* AppController::projectsModel() const
{
    return global_DBObjects.projectinformationmodelproxy();
}

QAbstractItemModel* AppController::clientsModel() const
{
    return global_DBObjects.clientsmodelproxy();
}

QAbstractItemModel* AppController::peopleModel() const
{
    return global_DBObjects.peoplemodelproxy();
}

QAbstractItemModel* AppController::trackerItemsModel() const
{
    return global_DBObjects.trackeritemsmodelproxy();
}

QAbstractItemModel* AppController::allItemsModel() const
{
    return global_DBObjects.allitemsmodelproxy();
}

QAbstractItemModel* AppController::projectNotesModel() const
{
    return global_DBObjects.projectnotesmodelproxy();
}

QAbstractItemModel* AppController::meetingAttendeesModel() const
{
    return global_DBObjects.meetingattendeesmodelproxy();
}

QAbstractItemModel* AppController::searchResultsModel() const
{
    return global_DBObjects.searchresultsmodelproxy();
}

QAbstractItemModel* AppController::statusReportItemsModel() const
{
    return global_DBObjects.statusreportitemsmodelproxy();
}

QAbstractItemModel* AppController::projectTeamMembersModel() const
{
    return global_DBObjects.projectteammembersmodelproxy();
}

QAbstractItemModel* AppController::projectLocationsModel() const
{
    return global_DBObjects.projectlocationsmodelproxy();
}
