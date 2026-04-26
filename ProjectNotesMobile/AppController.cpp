// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "AppController.h"

#include "QLogger.h"
#include "QLoggerWriter.h"

#include "sqlitesyncpro.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QTextDocument>
#include <QThread>
#include <QTimer>

using namespace QLogger;

// ── IdRowIndex ───────────────────────────────────────────────────────────────
// O(1) id→row hash for any QAbstractItemModel. Marks itself dirty on any
// model change and rebuilds lazily on the next lookup. Replaces the per-row
// linear scans that QML list delegates were hitting on every binding
// re-evaluation (scroll, quick search, sync row update).
class IdRowIndex
{
public:
    IdRowIndex(QAbstractItemModel* model, int idColumn)
        : m_model(model), m_idColumn(idColumn)
    {
        const auto invalidate = [this] { m_dirty = true; };
        QObject::connect(model, &QAbstractItemModel::modelReset,    model, invalidate);
        QObject::connect(model, &QAbstractItemModel::layoutChanged, model, invalidate);
        QObject::connect(model, &QAbstractItemModel::dataChanged,   model, invalidate);
        QObject::connect(model, &QAbstractItemModel::rowsInserted,  model, invalidate);
        QObject::connect(model, &QAbstractItemModel::rowsRemoved,   model, invalidate);
    }

    int rowFor(const QString& id) const
    {
        if (id.isEmpty()) return -1;
        if (m_dirty) rebuild();
        return m_rows.value(id, -1);
    }

private:
    void rebuild() const
    {
        m_rows.clear();
        const int n = m_model->rowCount();
        m_rows.reserve(n);
        for (int row = 0; row < n; ++row) {
            const QString id = m_model->data(m_model->index(row, m_idColumn)).toString();
            if (!id.isEmpty()) m_rows.insert(id, row);
        }
        m_dirty = false;
    }

    QAbstractItemModel* m_model;
    int                 m_idColumn;
    mutable bool                m_dirty = true;
    mutable QHash<QString, int> m_rows;
};

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
    DatabaseObjects::setLocalSettingsCallbacks(
        [](const QString& key, const QString& val) { global_MobileSettings.setValue(key, val); },
        [](const QString& key) -> QString { return global_MobileSettings.getValue(key).toString(); }
    );

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
    if (m_syncApi && m_syncApiThread) {
        // Shut the sync engine down on its own thread, then stop the thread.
        // BlockingQueuedConnection waits for any in-flight initialize() call
        // to return before shutdown() runs, so we never tear the engine down
        // mid-bootstrap.
        SqliteSyncPro* api = m_syncApi;
        QMetaObject::invokeMethod(api, [api]() { api->shutdown(); },
                                  Qt::BlockingQueuedConnection);
        m_syncApiThread->quit();
        m_syncApiThread->wait();
        // m_syncApi is auto-deleted via the QThread::finished → deleteLater
        // connection set up in configureSyncApi().
        m_syncApi = nullptr;
    }
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

    if (!global_DBObjects.openDatabase(dbPath, connName, true)) {
        emit errorOccurred(tr("Database Error"), tr("Failed to open database at %1").arg(dbPath));
        return false;
    }

    QLog_Debug(DEBUGLOG, QString("Database opened: %1").arg(dbPath));

    // On first install set mobile-friendly defaults before applying filters.
    // On subsequent launches the values stored in the database are used as-is.
    if (isNewDatabase)
        global_DBObjects.setShowClosedProjects(true);

    // Apply filters selected from the view menu option.
    global_DBObjects.setGlobalSearches(false);

    // Build O(1) id→row indexes over the proxy models that QML list pages
    // and detail-page combobox lookups hit on every binding evaluation.
    // The team-members index keys on people_id (col 2) since callers look up
    // a row by the person, not by the team_members.id.
    global_DBObjects.clientsmodel()->refresh();
    m_clientsIndex.reset(
        new IdRowIndex(global_DBObjects.clientsmodelproxy(), 0));
    global_DBObjects.peoplemodel()->refresh();
    m_peopleIndex.reset(
        new IdRowIndex(global_DBObjects.peoplemodelproxy(), 0));
    global_DBObjects.projectinformationmodel()->refresh();
    m_projectsIndex.reset(
        new IdRowIndex(global_DBObjects.projectinformationmodelproxy(), 0));
    global_DBObjects.projectteammembersmodel()->refresh();
    m_teamMembersByPersonIndex.reset(
         new IdRowIndex(global_DBObjects.projectteammembersmodelproxy(), 2));

    // Tell QML the view-option properties are now readable.
    emit viewOptionsChanged();
    emit databaseReady();
    return true;
}

// ── Sync ─────────────────────────────────────────────────────────────────────

void AppController::configureSyncApi()
{
    if (!m_syncApi) {
        // Lazy-create the sync API on its own thread. Done lazily so apps
        // with sync disabled never spin up the thread.
        m_syncApiThread = new QThread(this);
        m_syncApiThread->setObjectName(QStringLiteral("SqliteSyncProThread"));

        m_syncApi = new SqliteSyncPro;  // no parent — lives on the API thread
        m_syncApi->moveToThread(m_syncApiThread);

        // Tear the engine down on its own thread when the thread exits.
        connect(m_syncApiThread, &QThread::finished,
                m_syncApi,        &QObject::deleteLater);

        // Cross-thread signal connections — auto-connection becomes queued.
        connect(m_syncApi, &SqliteSyncPro::rowChanged,
                this,      &AppController::onSyncRowChanged);
        connect(m_syncApi, &SqliteSyncPro::syncCompleted,
                this,      &AppController::onSyncComplete);
        connect(m_syncApi, &SqliteSyncPro::syncProgress,
                this,      &AppController::onSyncProgress);
        connect(m_syncApi, &SqliteSyncPro::syncStatusUpdated,
                this,      &AppController::onSyncStatusUpdated);

        m_syncApiThread->start();
    }

    // Setters are mutex-protected inside SqliteSyncPro, so it is safe to
    // call them directly from the main thread.
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

    SqliteSyncPro* api = m_syncApi;
    const QString dbPath = MobileSettings::dataLocation() + "/ProjectNotes.db";

    // Run the heavy bootstrap (auth, WAL pragma, table discovery, persistent
    // DB open) on the API thread so the UI stays responsive during launch.
    QMetaObject::invokeMethod(api, [this, api, dbPath]() {
        // If a sync loop is already running, wake it for an immediate cycle
        // instead of calling initialize() again. Re-initializing while the
        // loop thread is live would overwrite m_syncWorker/m_syncThread; the
        // old thread's cleanup lambda would then delete the NEW worker,
        // causing a crash in run().
        if (api->isInitialized()) {
            api->retryNow();
            return;
        }
        api->setDatabasePath(dbPath);
        if (!api->initialize()) {
            // Init failed — flip the bar red on the UI thread.
            QMetaObject::invokeMethod(this, [this]() {
                setSyncProgress(0.0, true);
            }, Qt::QueuedConnection);
        }
    }, Qt::QueuedConnection);
}

void AppController::stopSync()
{
    if (!m_syncApi) return;
    SqliteSyncPro* api = m_syncApi;
    QMetaObject::invokeMethod(api, [api]() { api->shutdown(); },
                              Qt::QueuedConnection);
}

void AppController::onSyncComplete(const SyncResult& result)
{
    // Flush all row changes accumulated during the sync cycle in one pass.
    global_DBObjects.updateDisplayData();

    if (result.success) {
        m_syncHasError = false;
        // Ask SqliteSyncPro to count remaining pending records and emit
        // syncStatusUpdated — same as the desktop.  That signal is the sole
        // authority on whether the bar shows or hides after a cycle.
        // checkSyncStatus touches the persistent DB connection that lives on
        // the API thread, so dispatch it there.
        if (SqliteSyncPro* api = m_syncApi) {
            QMetaObject::invokeMethod(api, [api, result]() {
                api->checkSyncStatus(result);
            }, Qt::QueuedConnection);
        }
    } else {
        // Turn bar red; no popup — the bar is the only sync error indicator.
        setSyncProgress(m_syncProgress, true);
    }
}

void AppController::onSyncRowChanged(const QString& tableName, const QString& id)
{
    global_DBObjects.pushRowChange(tableName, id, KeyColumnChange::Update);
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

    SqliteSyncPro* api = m_syncApi;
    const QString dbPath = MobileSettings::dataLocation() + "/ProjectNotes.db";

    QMetaObject::invokeMethod(api, [this, api, dbPath]() {
        api->setDatabasePath(dbPath);
        if (!api->isInitialized()) {
            if (!api->initialize()) {
                QMetaObject::invokeMethod(this, [this]() {
                    setSyncProgress(0.0, true);  // red bar — no popup
                }, Qt::QueuedConnection);
                return;
            }
        }
        api->syncAll();
    }, Qt::QueuedConnection);
}

// ── Record editing ────────────────────────────────────────────────────────────

bool AppController::savePerson(int row, const QString& name, const QString& email,
                                const QString& officePhone, const QString& cellPhone,
                                const QString& clientId, const QString& role)
{
    global_DBObjects.setLastSaveError("");
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
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

bool AppController::saveProject(int row, const QString& projectNumber,
                                 const QString& projectName, const QString& projectStatus,
                                 const QString& primaryContactId, const QString& clientId,
                                 const QString& lastStatusDate, const QString& lastInvoiceDate,
                                 const QString& invoicingPeriod, const QString& statusReportPeriod)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.projectinformationmodelproxy();
    if (row < 0 || row >= model->rowCount())
        return false;
    bool ok = true;
    ok &= model->setData(model->index(row,  1), projectNumber);
    ok &= model->setData(model->index(row,  2), projectName);
    ok &= model->setData(model->index(row,  3), lastStatusDate);
    ok &= model->setData(model->index(row,  4), lastInvoiceDate);
    ok &= model->setData(model->index(row,  5), primaryContactId);
    ok &= model->setData(model->index(row, 11), invoicingPeriod);
    ok &= model->setData(model->index(row, 12), statusReportPeriod);
    ok &= model->setData(model->index(row, 13), clientId);
    ok &= model->setData(model->index(row, 14), projectStatus);
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

bool AppController::saveStatusItem(int row, const QString& category, const QString& description)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.statusreportitemsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = true;
    ok &= model->setData(model->index(row, 2), category);
    ok &= model->setData(model->index(row, 3), description);
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

bool AppController::saveTeamMember(int row, const QString& peopleId, const QString& role, bool receiveStatusReport)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.projectteammembersmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = true;
    ok &= model->setData(model->index(row, 2), peopleId);
    ok &= model->setData(model->index(row, 4), receiveStatusReport ? "1" : "0");
    ok &= model->setData(model->index(row, 5), role);
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

bool AppController::saveProjectLocation(int row, const QString& locationType,
                                         const QString& description, const QString& path)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.projectlocationsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = true;
    ok &= model->setData(model->index(row, 2), locationType);
    ok &= model->setData(model->index(row, 3), description);
    ok &= model->setData(model->index(row, 4), path);
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

bool AppController::saveProjectNote(int row, const QString& title, const QString& date,
                                     const QString& note, bool internalItem)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.projectnotesmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = true;
    ok &= model->setData(model->index(row, 2), title);
    ok &= model->setData(model->index(row, 3), date);
    ok &= model->setData(model->index(row, 4), note);
    ok &= model->setData(model->index(row, 5), internalItem ? "1" : "0");
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

bool AppController::saveClient(int row, const QString& clientName)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.clientsmodelproxy();
    if (row < 0 || row >= model->rowCount())
        return false;
    bool ok = model->setData(model->index(row, 1), clientName);
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

int AppController::clientRowForId(const QString& clientId) const
{
    return m_clientsIndex ? m_clientsIndex->rowFor(clientId) : -1;
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
    return m_peopleIndex ? m_peopleIndex->rowFor(peopleId) : -1;
}

QString AppController::peopleIdAtRow(int row) const
{
    QAbstractItemModel* model = global_DBObjects.peoplemodelproxy();
    if (row < 0 || row >= model->rowCount())
        return {};
    return model->data(model->index(row, 0)).toString();
}

QString AppController::peopleNameForId(const QString& personId) const
{
    if (!m_peopleIndex) return {};
    const int row = m_peopleIndex->rowFor(personId);
    if (row < 0) return {};
    QAbstractItemModel* model = global_DBObjects.peoplemodelproxy();
    return model->data(model->index(row, 1)).toString();  // col 1 = name
}

QString AppController::peopleEmailForId(const QString& personId) const
{
    if (!m_peopleIndex) return {};
    const int row = m_peopleIndex->rowFor(personId);
    if (row < 0) return {};
    QAbstractItemModel* model = global_DBObjects.peoplemodelproxy();
    return model->data(model->index(row, 2)).toString();  // col 2 = email
}

QString AppController::clientNameForId(const QString& clientId) const
{
    if (!m_clientsIndex) return {};
    const int row = m_clientsIndex->rowFor(clientId);
    if (row < 0) return {};
    QAbstractItemModel* model = global_DBObjects.clientsmodelproxy();
    return model->data(model->index(row, 1)).toString();  // col 1 = client_name
}

QString AppController::projectNumberForId(const QString& projectId) const
{
    if (!m_projectsIndex) return {};
    const int row = m_projectsIndex->rowFor(projectId);
    if (row < 0) return {};
    QAbstractItemModel* model = global_DBObjects.projectinformationmodelproxy();
    return model->data(model->index(row, 1)).toString();  // col 1 = project_number
}

QString AppController::projectNameForId(const QString& projectId) const
{
    if (!m_projectsIndex) return {};
    const int row = m_projectsIndex->rowFor(projectId);
    if (row < 0) return {};
    QAbstractItemModel* model = global_DBObjects.projectinformationmodelproxy();
    return model->data(model->index(row, 2)).toString();  // col 2 = project_name
}

QString AppController::teamMemberEmailList() const
{
    QAbstractItemModel* model = global_DBObjects.projectteammembersmodelproxy();
    QStringList emails;
    for (int row = 0; row < model->rowCount(); ++row) {
        const QString email = model->data(model->index(row, 6)).toString();  // col 6 = email
        if (!email.isEmpty() && !emails.contains(email))
            emails.append(email);
    }
    return emails.join(",");
}

QString AppController::attendeeEmailList() const
{
    QAbstractItemModel* model = global_DBObjects.meetingattendeesmodelproxy();
    QStringList emails;
    for (int row = 0; row < model->rowCount(); ++row) {
        const QString email = model->data(model->index(row, 5)).toString();  // col 5 = email
        if (!email.isEmpty() && !emails.contains(email))
            emails.append(email);
    }
    return emails.join(",");
}

QString AppController::htmlToPlainText(const QString& html) const
{
    if (html.isEmpty() || !html.contains('<'))
        return html;
    QTextDocument doc;
    doc.setHtml(html);
    return doc.toPlainText();
}

QString AppController::lastSaveError() const
{
    return global_DBObjects.lastSaveError();
}

// teamMemberRowForPersonId — search col 2 (people_id) in projectTeamMembersModelProxy
int AppController::teamMemberRowForPersonId(const QString& personId) const
{
    return m_teamMembersByPersonIndex
        ? m_teamMembersByPersonIndex->rowFor(personId) : -1;
}

// teamMemberPersonIdAtRow — return col 2 (people_id) from projectTeamMembersModelProxy
QString AppController::teamMemberPersonIdAtRow(int row) const
{
    QAbstractItemModel* model = global_DBObjects.projectteammembersmodelproxy();
    if (row < 0 || row >= model->rowCount()) return {};
    return model->data(model->index(row, 2)).toString();
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
    return proxyRowFromSource(global_DBObjects.projectinformationmodelproxy(),
                              global_DBObjects.projectinformationmodel()->newRecord());
}

bool AppController::deleteProject(int row)
{
    return deleteProxyRow(global_DBObjects.projectinformationmodelproxy(),
                          global_DBObjects.projectinformationmodel(), row);
}

int AppController::copyProject(int row)
{
    return copyProxyRow(global_DBObjects.projectinformationmodelproxy(),
                        global_DBObjects.projectinformationmodel(), row);
}

QVariantMap AppController::getProjectData(int row) const
{
    return proxyRowToMap(global_DBObjects.projectinformationmodelproxy(), row);
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

// ── Tracker item detail (single-record model) ─────────────────────────────────

void AppController::openTrackerItem(const QString& itemId)
{
    // Filter the detail model to the specific item (col 0 = id)
    global_DBObjects.actionitemsdetailsmodel()->setFilter(0, itemId);
    global_DBObjects.actionitemsdetailsmodel()->refresh();

    // Keep the per-project team-member lookup in sync with the opened item.
    // TrackerItemDetailPage uses projectTeamMembersModel for Identified By and
    // Assigned To. When navigating from ProjectDetailsPage this was already set
    // up by setProjectFilter(projectId), but the home/all-items path only calls
    // openTrackerItem(itemId). Without refreshing this filter here, the detail
    // page sees the wrong project team list (or an empty one) and save paths
    // that depend on those comboboxes break.
    QString projectId;
    if (global_DBObjects.actionitemsdetailsmodel()->rowCount(QModelIndex()) > 0) {
        projectId = global_DBObjects.actionitemsdetailsmodel()
                        ->data(global_DBObjects.actionitemsdetailsmodel()->index(0, 14))
                        .toString();
    }

    if (!projectId.isEmpty())
        global_DBObjects.projectteammembersmodel()->setFilter(1, projectId);
    else
        global_DBObjects.projectteammembersmodel()->clearFilter(1);
    global_DBObjects.projectteammembersmodel()->refresh();

    // Filter comments to the same item (col 1 = item_id)
    global_DBObjects.trackeritemscommentsmodel()->setFilter(1, itemId);
    global_DBObjects.trackeritemscommentsmodel()->refresh();
}

int AppController::addTrackerItem(const QString& projectId)
{
    QVariant fk(projectId);
    QModelIndex srcIdx = global_DBObjects.actionitemsdetailsmodel()->newRecord(&fk);
    if (!srcIdx.isValid()) return -1;

    // Force-insert immediately so the item gets a stable UUID; this is required
    // so we can call openTrackerItem(itemId) straight after addTrackerItem().
    global_DBObjects.actionitemsdetailsmodel()->insertCacheRow(srcIdx.row());

    // Read the assigned id and switch the detail model's filter to this new item,
    // so row 0 of trackerItemDetailModel points to the freshly-created record.
    QString newId = global_DBObjects.actionitemsdetailsmodel()
                        ->data(global_DBObjects.actionitemsdetailsmodel()->index(srcIdx.row(), 0))
                        .toString();
    if (!newId.isEmpty())
        openTrackerItem(newId);

    return 0;  // after openTrackerItem the new record is always at proxy row 0
}

bool AppController::deleteTrackerItemDetail(int row)
{
    return deleteProxyRow(global_DBObjects.actionitemsdetailsmodelproxy(),
                          global_DBObjects.actionitemsdetailsmodel(), row);
}

int AppController::copyTrackerItemDetail(int row)
{
    int newRow = copyProxyRow(global_DBObjects.actionitemsdetailsmodelproxy(),
                              global_DBObjects.actionitemsdetailsmodel(), row);
    if (newRow < 0) return -1;

    // Switch detail filter to the copy so row 0 is the new record.
    QAbstractItemModel* proxy = global_DBObjects.actionitemsdetailsmodelproxy();
    QString newId = proxy->data(proxy->index(newRow, 0)).toString();
    if (!newId.isEmpty())
        openTrackerItem(newId);

    return 0;  // detail model filtered to copy → always row 0
}

QVariantMap AppController::getTrackerItemDetailData(int row) const
{
    return proxyRowToMap(global_DBObjects.actionitemsdetailsmodelproxy(), row);
}

bool AppController::saveTrackerItemDetail(int row,
                                          const QString& itemNumber,
                                          const QString& itemType,
                                          const QString& itemName,
                                          const QString& description,
                                          const QString& identifiedBy,
                                          const QString& assignedTo,
                                          const QString& priority,
                                          const QString& status,
                                          const QString& dateIdentified,
                                          const QString& dateDue,
                                          bool           internalItem)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.actionitemsdetailsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = true;
    ok &= model->setData(model->index(row,  1), itemNumber);
    ok &= model->setData(model->index(row,  2), itemType);
    ok &= model->setData(model->index(row,  3), itemName);
    ok &= model->setData(model->index(row,  4), identifiedBy);
    ok &= model->setData(model->index(row,  5), dateIdentified);
    ok &= model->setData(model->index(row,  6), description);
    ok &= model->setData(model->index(row,  7), assignedTo);
    ok &= model->setData(model->index(row,  8), priority);
    ok &= model->setData(model->index(row,  9), status);
    ok &= model->setData(model->index(row, 10), dateDue);
    ok &= model->setData(model->index(row, 15), internalItem ? "1" : "0");
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

QString AppController::trackerItemIdAtRow(int row) const
{
    QAbstractItemModel* model = global_DBObjects.actionitemsdetailsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return {};
    return model->data(model->index(row, 0)).toString();
}

bool AppController::isItemNumberUnique(const QString& itemId, const QString& itemNumber) const
{
    if (itemNumber.isEmpty()) return true;
    QString safeId     = itemId.trimmed();
    QString safeNumber = itemNumber.trimmed().replace("'", "''");
    safeId.replace("'", "''");
    QString sql = QString(
        "SELECT COUNT(*) FROM item_tracker "
        "WHERE project_id = (SELECT project_id FROM item_tracker WHERE id = '%1') "
        "AND item_number = '%2' "
        "AND id != '%1' "
        "AND deleted = 0"
    ).arg(safeId, safeNumber);
    return global_DBObjects.execute(sql).toInt() == 0;
}

// ── Tracker item comments ─────────────────────────────────────────────────────

int AppController::addComment(const QString& itemId)
{
    QVariant fk(itemId);
    return proxyRowFromSource(global_DBObjects.trackeritemscommentsmodelproxy(),
                              global_DBObjects.trackeritemscommentsmodel()->newRecord(&fk));
}

bool AppController::deleteComment(int row)
{
    return deleteProxyRow(global_DBObjects.trackeritemscommentsmodelproxy(),
                          global_DBObjects.trackeritemscommentsmodel(), row);
}

int AppController::copyComment(int row)
{
    return copyProxyRow(global_DBObjects.trackeritemscommentsmodelproxy(),
                        global_DBObjects.trackeritemscommentsmodel(), row);
}

QVariantMap AppController::getCommentData(int row) const
{
    return proxyRowToMap(global_DBObjects.trackeritemscommentsmodelproxy(), row);
}

bool AppController::saveComment(int row, const QString& date,
                                 const QString& note, const QString& updatedBy)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.trackeritemscommentsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = true;
    ok &= model->setData(model->index(row, 2), date);
    ok &= model->setData(model->index(row, 3), note);
    ok &= model->setData(model->index(row, 4), updatedBy);
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

// ── Meeting attendees ─────────────────────────────────────────────────────────

void AppController::setNoteFilter(const QString& noteId, const QString& projectId)
{
    // Attendees: note_id is col 1
    global_DBObjects.meetingattendeesmodel()->setFilter(1, noteId);
    global_DBObjects.meetingattendeesmodel()->refresh();

    // Note action items: note_id is col 13
    global_DBObjects.notesactionitemsmodel()->setFilter(13, noteId);
    global_DBObjects.notesactionitemsmodel()->refresh();

    Q_UNUSED(projectId)  // reserved for future use if needed
}

int AppController::addAttendee(const QString& noteId)
{
    QVariant fk(noteId);
    return proxyRowFromSource(global_DBObjects.meetingattendeesmodelproxy(),
                              global_DBObjects.meetingattendeesmodel()->newRecord(&fk));
}

bool AppController::deleteAttendee(int row)
{
    return deleteProxyRow(global_DBObjects.meetingattendeesmodelproxy(),
                          global_DBObjects.meetingattendeesmodel(), row);
}

QVariantMap AppController::getAttendeeData(int row) const
{
    return proxyRowToMap(global_DBObjects.meetingattendeesmodelproxy(), row);
}

bool AppController::saveAttendee(int row, const QString& personId)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.meetingattendeesmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    bool ok = model->setData(model->index(row, 2), personId);
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

// ── Note action items ─────────────────────────────────────────────────────────

int AppController::addNoteActionItem(const QString& noteId, const QString& projectId)
{
    QVariant fk1(noteId);
    QVariant fk2(projectId);
    QModelIndex srcIdx = global_DBObjects.notesactionitemsmodel()->newRecord(&fk1, &fk2);
    if (!srcIdx.isValid()) return -1;
    // Force-insert so the item has a stable UUID for openTrackerItem()
    global_DBObjects.notesactionitemsmodel()->insertCacheRow(srcIdx.row());

    // Switch detail model to new item so row 0 is immediately readable.
    QString newId = global_DBObjects.notesactionitemsmodel()
                        ->data(global_DBObjects.notesactionitemsmodel()->index(srcIdx.row(), 0))
                        .toString();
    if (!newId.isEmpty())
        openTrackerItem(newId);

    return 0;  // detail model filtered to new item → always row 0
}

bool AppController::deleteNoteActionItem(int row)
{
    return deleteProxyRow(global_DBObjects.notesactionitemsmodelproxy(),
                          global_DBObjects.notesactionitemsmodel(), row);
}

QString AppController::noteActionItemIdAtRow(int row) const
{
    QAbstractItemModel* model = global_DBObjects.notesactionitemsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return {};
    return model->data(model->index(row, 0)).toString();
}

// ── Model refresh helpers ─────────────────────────────────────────────────────

void AppController::refreshTeamMembers()
{
    global_DBObjects.projectteammembersmodel()->refresh();
}

void AppController::refreshMeetingAttendees()
{
    global_DBObjects.meetingattendeesmodel()->refresh();
}

void AppController::refreshTrackerComments()
{
    global_DBObjects.trackeritemscommentsmodel()->refresh();
}

void AppController::refreshTrackerItems()
{
    global_DBObjects.trackeritemsmodel()->refresh();
}

void AppController::refreshAllItems()
{
    global_DBObjects.allitemsmodel()->refresh();
}

void AppController::refreshProjectNotes()
{
    global_DBObjects.projectnotesmodel()->refresh();
}

void AppController::refreshNoteActionItems()
{
    global_DBObjects.notesactionitemsmodel()->refresh();
}

void AppController::refreshProjectsList()
{
    global_DBObjects.projectinformationmodel()->refresh();
}

void AppController::setQuickSearch(QAbstractItemModel* model, const QString& text)
{
    if (auto* proxy = dynamic_cast<SortFilterProxyModel*>(model))
        proxy->setQuickSearch(text);
}

// ── Model accessors ──────────────────────────────────────────────────────────

QAbstractItemModel* AppController::projectsListModel() const
{
    return global_DBObjects.projectinformationmodelproxy();
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

QAbstractItemModel* AppController::trackerItemCommentsModel() const
{
    return global_DBObjects.trackeritemscommentsmodelproxy();
}

QAbstractItemModel* AppController::notesActionItemsModel() const
{
    return global_DBObjects.notesactionitemsmodelproxy();
}

QAbstractItemModel* AppController::trackerItemDetailModel() const
{
    return global_DBObjects.actionitemsdetailsmodelproxy();
}
