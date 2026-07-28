// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "DesktopAppController.h"

#include "databaseobjects.h"
#include "sortfilterproxymodel.h"
#include "FolderManager.h"
#include "projectsmodel.h"
#include "projectnotesmodel.h"
#include "meetingattendeesmodel.h"
#include "notesactionitemsmodel.h"
#include "peoplemodel.h"
#include "clientsmodel.h"
#include "trackeritemsmodel.h"
#include "trackeritemcommentsmodel.h"
#include "projectteammembersmodel.h"
#include "projectlocationsmodel.h"
#include "statusreportitemsmodel.h"
#include "searchresultsmodel.h"

#include "pluginmanager.h"
#include "plugin.h"
#include "pythonworker.h"

#include "sqlitesyncpro.h"
#include "syncresult.h"

#include <QDir>
#include <QDomDocument>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QUrl>

DesktopAppController* DesktopAppController::s_instance = nullptr;
QString DesktopAppController::s_developerProfile;
bool    DesktopAppController::s_testSupabase = false;

DesktopAppController* DesktopAppController::create(QQmlEngine* /*engine*/, QJSEngine* /*scriptEngine*/)
{
    if (!s_instance)
        s_instance = new DesktopAppController();
    return s_instance;
}

DesktopAppController::DesktopAppController(QObject* parent)
    : QObject(parent)
{
    if (!s_instance)
        s_instance = this;

    // Install a backing store for DatabaseObjects "local" parameters (per-machine
    // view filters like UserFilter:ShowInternalItems / ShowClosedProjects /
    // ViewFilter:ShowResolvedTrackerItems). Without this the desktop app had no
    // s_localSave/s_localLoad, so setShowInternalItems() was a no-op and
    // getShowInternalItems() always returned false — the Show Internal / closed /
    // resolved toggles never took effect. We use the same QSettings the Widgets
    // app uses ("ProjectNotes"[+profile] / "AppSettings") so both frontends share
    // the same view-option state.
    DatabaseObjects::setLocalSettingsCallbacks(
        [](const QString& key, const QString& val) {
            QSettings s(QStringLiteral("ProjectNotes") + s_developerProfile,
                        QStringLiteral("AppSettings"));
            s.setValue(key, val);
        },
        [](const QString& key) -> QString {
            QSettings s(QStringLiteral("ProjectNotes") + s_developerProfile,
                        QStringLiteral("AppSettings"));
            return s.value(key).toString();
        });
}

DesktopAppController::~DesktopAppController()
{
    if (m_syncApi && m_syncApiThread) {
        // Shut the engine down on its own thread, then stop the thread.
        SqliteSyncPro* api = m_syncApi;
        QMetaObject::invokeMethod(api, [api]() { api->shutdown(); },
                                  Qt::BlockingQueuedConnection);
        m_syncApiThread->quit();
        m_syncApiThread->wait();
        m_syncApi = nullptr;   // auto-deleted via QThread::finished → deleteLater
    }
    if (m_databaseOpen)
        global_DBObjects.closeDatabase();
}

// ── Small proxy helpers ──────────────────────────────────────────────────────

static int proxyRowFromSource(SortFilterProxyModel* proxy, const QModelIndex& srcIdx)
{
    if (!srcIdx.isValid()) return -1;
    const QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
    return proxyIdx.isValid() ? proxyIdx.row() : srcIdx.row();
}

static QVariantMap proxyRowToMap(SortFilterProxyModel* proxy, int row)
{
    if (!proxy || row < 0 || row >= proxy->rowCount()) return {};
    QVariantMap result;
    const auto roles = proxy->roleNames();
    const QModelIndex idx = proxy->index(row, 0);
    for (auto it = roles.constBegin(); it != roles.constEnd(); ++it) {
        if (it.key() >= Qt::UserRole)
            result[QString::fromUtf8(it.value())] = proxy->data(idx, it.key());
    }
    return result;
}

// Delete the record at proxy |row|, surfacing the model's blocked-delete
// message (child/foreign-key references) via errorOccurred() on failure.
static bool deleteProxyRow(SortFilterProxyModel* proxy, SqlQueryModel* source, int row)
{
    const QModelIndex proxyIdx = proxy->index(row, 0);
    if (!proxyIdx.isValid()) return false;
    return source->deleteRecord(proxy->mapToSource(proxyIdx));
}

// ── Database ─────────────────────────────────────────────────────────────────

bool DesktopAppController::openOrCreateDatabase()
{
    if (m_databaseOpen)
        return true;

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!s_developerProfile.isEmpty())
        dataDir += "/" + s_developerProfile;   // matches AppSettings::dataLocation()
    const QString dbPath  = dataDir + "/ProjectNotes.db";
    const QString connName = "ProjectNotesDesktop";

    QDir().mkpath(dataDir);

    const bool isNew = !QFileInfo::exists(dbPath);
    if (isNew && !global_DBObjects.createDatabase(dbPath)) {
        emit errorOccurred(tr("Database Error"),
                           tr("Failed to create database at %1").arg(dbPath));
        return false;
    }

    if (!global_DBObjects.openDatabase(dbPath, connName, true)) {
        emit errorOccurred(tr("Database Error"),
                           tr("Failed to open database at %1").arg(dbPath));
        return false;
    }

    // The QML app surfaces save/validation failures through its own themed
    // dialog (errorOccurred → Main.qml). Suppress the models' native
    // QMessageBox popups so a single blocked edit doesn't stack two dialogs.
    global_DBObjects.setGuiDialogsEnabled(false);

    global_DBObjects.setGlobalSearches(false);
    global_DBObjects.projectinformationmodel()->refresh();
    global_DBObjects.clientsmodel()->refresh();
    global_DBObjects.peoplemodel()->refresh();

    m_databaseOpen = true;
    emit databaseReady();

    // Boot the embedded-Python plugin engine now that the database is open (some
    // plugins query it as they register their menus).
    ensurePluginManager();

    // Auto-start cloud sync if the user has it enabled (same as the Widgets app).
    // Deferred so the UI is up first; the heavy bootstrap runs on the sync thread.
    if (syncEnabled() && !syncEmail().isEmpty() && !syncPassword().isEmpty())
        QTimer::singleShot(0, this, &DesktopAppController::syncNow);

    return true;
}

// ── Models ───────────────────────────────────────────────────────────────────

QAbstractItemModel* DesktopAppController::projectsListModel() const
{ return global_DBObjects.projectinformationmodelproxy(); }
QAbstractItemModel* DesktopAppController::projectNotesModel() const
{ return global_DBObjects.projectnotesmodelproxy(); }
QAbstractItemModel* DesktopAppController::meetingAttendeesModel() const
{ return global_DBObjects.meetingattendeesmodelproxy(); }
QAbstractItemModel* DesktopAppController::notesActionItemsModel() const
{ return global_DBObjects.notesactionitemsmodelproxy(); }
QAbstractItemModel* DesktopAppController::peopleModel() const
{ return global_DBObjects.peoplemodelproxy(); }
QAbstractItemModel* DesktopAppController::clientsModel() const
{ return global_DBObjects.clientsmodelproxy(); }
QAbstractItemModel* DesktopAppController::allItemsModel() const
{ return global_DBObjects.allitemsmodelproxy(); }
QAbstractItemModel* DesktopAppController::trackerItemDetailModel() const
{ return global_DBObjects.actionitemsdetailsmodelproxy(); }
QAbstractItemModel* DesktopAppController::trackerItemCommentsModel() const
{ return global_DBObjects.trackeritemscommentsmodelproxy(); }
QAbstractItemModel* DesktopAppController::projectTrackerItemsModel() const
{ return global_DBObjects.trackeritemsmodelproxy(); }
QAbstractItemModel* DesktopAppController::projectTeamMembersModel() const
{ return global_DBObjects.projectteammembersmodelproxy(); }
QAbstractItemModel* DesktopAppController::projectLocationsModel() const
{ return global_DBObjects.projectlocationsmodelproxy(); }
QAbstractItemModel* DesktopAppController::statusReportItemsModel() const
{ return global_DBObjects.statusreportitemsmodelproxy(); }
QAbstractItemModel* DesktopAppController::searchResultsModel() const
{ return global_DBObjects.searchresultsmodelproxy(); }

// ── Global search ────────────────────────────────────────────────────────────

void DesktopAppController::performSearch(const QString& text)
{
    global_DBObjects.searchresultsmodel()->PerformSearch(text);
    global_DBObjects.searchresultsmodel()->refresh();
}

// ── XML import / export ──────────────────────────────────────────────────────

static QString localPath(const QString& fileUrlOrPath)
{
    const QUrl url(fileUrlOrPath);
    return url.isLocalFile() ? url.toLocalFile() : fileUrlOrPath;
}

bool DesktopAppController::importXmlFile(const QString& fileUrlOrPath)
{
    const QString path = localPath(fileUrlOrPath);
    QFile infile(path);
    if (!infile.open(QFile::ReadOnly | QFile::Text)) {
        emit errorOccurred(tr("Import Failed"), infile.errorString());
        return false;
    }

    QDomDocument xmldoc;
    xmldoc.setContent(&infile);
    infile.close();

    if (!global_DBObjects.importXMLDoc(xmldoc)) {
        emit errorOccurred(tr("Import Failed"), tr("Parsing the XML file failed."));
        return false;
    }

    global_DBObjects.updateDisplayData();

    // Refresh the top-level lists the UI shows.
    global_DBObjects.projectinformationmodel()->refresh();
    global_DBObjects.peoplemodel()->refresh();
    global_DBObjects.clientsmodel()->refresh();
    global_DBObjects.allitemsmodel()->refresh();
    if (FolderManager* fm = FolderManager::instance())
        fm->reload();   // an import may carry updated folder defs/memberships
    return true;
}

bool DesktopAppController::exportRecordXml(const QString& tableName, const QString& recordId,
                                           const QString& fileUrlOrPath)
{
    SqlQueryModel* exportModel = global_DBObjects.createExportObject(tableName);
    if (!exportModel) {
        emit errorOccurred(tr("Export Failed"), tr("Nothing to export for this record type."));
        return false;
    }
    exportModel->setFilter(0, recordId);   // col 0 = id
    exportModel->refresh();

    QDomDocument* xdoc = global_DBObjects.createXMLExportDoc(exportModel);

    const QString path = localPath(fileUrlOrPath);
    QFile outfile(path);
    if (!outfile.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
        emit errorOccurred(tr("Export Failed"), outfile.errorString());
        delete xdoc;
        return false;
    }

    QTextStream ts(&outfile);
    ts.setEncoding(QStringConverter::Utf8);
    xdoc->save(ts, 4);
    outfile.close();
    delete xdoc;
    return true;
}

// ── Preferences ──────────────────────────────────────────────────────────────

QString DesktopAppController::managingCompanyId() const
{ return global_DBObjects.getManagingCompany(); }
void DesktopAppController::setManagingCompanyId(const QString& clientId)
{ global_DBObjects.setManagingCompany(clientId); }
QString DesktopAppController::projectManagerId() const
{ return global_DBObjects.getProjectManager(); }
void DesktopAppController::setProjectManagerId(const QString& personId)
{ global_DBObjects.setProjectManager(personId); }

// ── View options ─────────────────────────────────────────────────────────────

bool DesktopAppController::showClosedProjects() const
{ return global_DBObjects.getShowClosedProjects(); }
void DesktopAppController::setShowClosedProjects(bool v)
{
    global_DBObjects.setShowClosedProjects(v);
    global_DBObjects.setGlobalSearches(true);
    emit viewOptionsChanged();
}

bool DesktopAppController::showInternalItems() const
{ return global_DBObjects.getShowInternalItems(); }
void DesktopAppController::setShowInternalItems(bool v)
{
    global_DBObjects.setShowInternalItems(v);
    global_DBObjects.setGlobalSearches(true);
    emit viewOptionsChanged();
}

bool DesktopAppController::newAndAssignedOnly() const
{ return !global_DBObjects.getShowResolvedTrackerItems(); }
void DesktopAppController::setNewAndAssignedOnly(bool v)
{
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

// ── Quick search ─────────────────────────────────────────────────────────────

void DesktopAppController::setQuickSearch(QAbstractItemModel* model, const QString& text)
{
    if (auto* proxy = qobject_cast<SortFilterProxyModel*>(model))
        proxy->setQuickSearch(text);
}

// ── Column filter editor ─────────────────────────────────────────────────────

// Resolve the SqlQueryModel behind a proxy model handed over from QML.
static SqlQueryModel* sourceModelOf(QAbstractItemModel* model)
{
    if (auto* proxy = qobject_cast<SortFilterProxyModel*>(model))
        return qobject_cast<SqlQueryModel*>(proxy->sourceModel());
    return nullptr;
}

QVariantList DesktopAppController::filterColumns(QAbstractItemModel* model) const
{
    QVariantList out;
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return out;

    const int cols = src->columnCount();
    for (int c = 1; c < cols; ++c) {          // column 0 is the hidden id
        if (!src->isSearchable(c)) continue;
        const SqlQueryModel::DBColumnType t = src->getType(c);
        QVariantMap m;
        m["field"]  = src->getColumnName(c);
        m["label"]  = src->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
        m["isDate"] = (t == SqlQueryModel::DBDate || t == SqlQueryModel::DBDateTime);
        out.append(m);
    }
    return out;
}

QStringList DesktopAppController::columnDistinctValues(QAbstractItemModel* model,
                                                       const QString& field) const
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return {};
    const int col = src->getColumnNumber(field);
    if (col < 0) return {};

    // Pull the distinct values from the base data rather than from the model's
    // currently-loaded rows. If we read the live rows, then any filter the user
    // has already applied has narrowed the model, so reopening the Filter dialog
    // would only offer the values that survived the filter — you could no longer
    // pick a different value. constructWhereClause(false) keeps the model's
    // built-in filters (context filter, deleted-row filter) but drops the
    // interactive user filter, matching the Widgets ValueSelectModel behavior.
    const QString dbcol = src->getColumnName(col);
    QString where = src->constructWhereClause(false);   // "" or " WHERE ... "
    where = where.isEmpty() ? QStringLiteral(" WHERE ") : (where + QStringLiteral(" AND "));

    const QString sql = "SELECT DISTINCT " + dbcol + " FROM ( " + src->BaseSQL()
                        + where + dbcol + " IS NOT NULL )";

    const SqlQueryModel::DBColumnType type = src->getType(col);
    QStringList values;
    QSqlQuery q(src->getDBOs()->getDb());
    if (q.exec(sql)) {
        while (q.next() && values.size() < 500) {
            QVariant raw = q.value(0);
            src->reformatValue(raw, type);   // match the grid's display formatting
            const QString v = raw.toString().trimmed();
            if (!v.isEmpty() && !values.contains(v))
                values.append(v);
        }
    } else {
        qWarning() << "columnDistinctValues query failed:" << q.lastError().text() << sql;
    }
    values.sort(Qt::CaseInsensitive);
    return values;
}

void DesktopAppController::applyColumnFilters(QAbstractItemModel* model,
                                              const QVariantList& specs)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return;

    src->clearAllUserSearches();

    for (const QVariant& sv : specs) {
        const QVariantMap spec = sv.toMap();
        const int col = src->getColumnNumber(spec.value("field").toString());
        if (col < 0) continue;

        const QVariantList values = spec.value("values").toList();
        if (!values.isEmpty())
            src->setUserFilter(col, values);

        const QString search = spec.value("search").toString().trimmed();
        if (!search.isEmpty())
            src->setUserSearchString(col, search);

        const QString start = spec.value("rangeStart").toString().trimmed();
        const QString end   = spec.value("rangeEnd").toString().trimmed();
        if (!start.isEmpty() || !end.isEmpty())
            src->setUserSearchRange(col, start, end);
    }

    src->activateUserFilter(QString());   // empty name → apply without persisting
}

void DesktopAppController::clearColumnFilters(QAbstractItemModel* model)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return;
    src->clearAllUserSearches();
    src->deactivateUserFilter(QString());
}

void DesktopAppController::refreshModel(QAbstractItemModel* model)
{
    if (SqlQueryModel* src = sourceModelOf(model))
        src->refresh();
}

int DesktopAppController::folderVisibleCount(QAbstractItemModel* model,
                                             const QString& folderId) const
{
    FolderManager* fm = FolderManager::instance();
    if (!model || !fm || folderId.isEmpty())
        return 0;

    // Walk the proxy's currently-visible rows (which already reflect the active
    // quick-search and column filters) and count the folder members among them.
    // Column 0 is the project id — the same value QML reads as model.id.
    int count = 0;
    const int rows = model->rowCount();
    for (int r = 0; r < rows; ++r) {
        const QString pid = model->data(model->index(r, 0)).toString();
        if (!pid.isEmpty() && fm->isProjectInFolder(pid, folderId))
            ++count;
    }
    return count;
}

// ── Python plugins ────────────────────────────────────────────────────────────

void DesktopAppController::ensurePluginManager()
{
    if (m_pluginManager)
        return;
    // The constructor initialises the embedded interpreter and loads plugins
    // from appDir/plugins (+ ~/Project Notes/plugins). It self-registers as
    // PluginManager::instance() for the Python callbacks.
    PluginManager::setDeveloperProfile(s_developerProfile);
    m_pluginManager = new PluginManager(this);

    // A plugin that imports data (Python set_data) runs on a worker thread with
    // its own non-GUI DatabaseObjects, which queues the touched rows into
    // global_DBObjects and then emits pluginRefreshRequest. Drain that queue on
    // the GUI thread so the displayed lists pick up the added/updated/removed
    // rows — the same wiring MainWindow::onRefreshRequested provides in the
    // Widgets app. Without this, background plugin imports never reach the UI.
    connect(m_pluginManager, &PluginManager::pluginRefreshRequest,
            this, [] {
                global_DBObjects.updateDisplayData();
                if (FolderManager* fm = FolderManager::instance())
                    fm->reload();
            });
}

QVariantList DesktopAppController::pluginMenusForModel(QAbstractItemModel* model)
{
    QVariantList out;
    m_pluginMenuCache.clear();
    if (!m_pluginManager)
        return out;

    SqlQueryModel* src = sourceModelOf(model);
    if (!src)
        return out;
    const QString table = src->tablename();
    if (table.isEmpty())
        return out;

    // A menu appears on a table's right-click when its dataexport matches that
    // table (empty dataexport = global menu, not a record menu) — same rule as
    // the Widgets TableView::contextMenuEvent.
    for (Plugin* p : m_pluginManager->plugins()) {
        if (!p || !p->loaded())
            continue;
        for (const PluginMenu& m : p->pythonplugin().menus()) {
            if (m.dataexport().compare(table, Qt::CaseInsensitive) != 0)
                continue;

            m_pluginMenuCache.append({ p, m.functionname(), m.tablefilter(), m.parameter() });

            QVariantMap entry;
            entry["title"]   = m.menutitle();
            entry["submenu"] = m.submenu();
            entry["index"]   = m_pluginMenuCache.size() - 1;
            out.append(entry);
        }
    }
    return out;
}

void DesktopAppController::runPluginMenu(QAbstractItemModel* model,
                                         const QString& recordId, int index)
{
    if (index < 0 || index >= m_pluginMenuCache.size())
        return;
    const PluginMenuRef& ref = m_pluginMenuCache.at(index);
    if (!ref.plugin)
        return;

    SqlQueryModel* src = sourceModelOf(model);
    if (!src)
        return;

    // Export just the selected record (col 0 = id), scoped by the menu's
    // tablefilter, then hand the XML to the plugin — mirrors slotPluginMenu.
    SqlQueryModel* exportModel = global_DBObjects.createExportObject(src->tablename());
    if (!exportModel) {
        emit errorOccurred(tr("Plugin"), tr("Nothing to export for this record type."));
        return;
    }
    exportModel->setFilter(0, recordId);
    exportModel->refresh();

    QDomDocument* xdoc = global_DBObjects.createXMLExportDoc(exportModel, ref.tablefilter);
    const QString xml = xdoc->toString();

    ref.plugin->callXmlMethod(ref.functionname, xml, ref.parameter);

    delete xdoc;
}

// ── Filters / refresh ────────────────────────────────────────────────────────

void DesktopAppController::setProjectFilter(const QString& projectId)
{
    // Project notes: project_id is col 1
    global_DBObjects.projectnotesmodel()->setFilter(1, projectId);
    global_DBObjects.projectnotesmodel()->refresh();

    // Team members: project_id is col 1
    global_DBObjects.projectteammembersmodel()->setFilter(1, projectId);
    global_DBObjects.projectteammembersmodel()->refresh();

    // Project locations: project_id is col 1
    global_DBObjects.projectlocationsmodel()->setFilter(1, projectId);
    global_DBObjects.projectlocationsmodel()->refresh();

    // Status report items: project_id is col 1
    global_DBObjects.statusreportitemsmodel()->setFilter(1, projectId);
    global_DBObjects.statusreportitemsmodel()->refresh();

    // Project tracker items (project view): project_id is col 14
    global_DBObjects.trackeritemsmodel()->setFilter(14, projectId);
    global_DBObjects.trackeritemsmodel()->refresh();
}

void DesktopAppController::setNoteFilter(const QString& noteId)
{
    // Attendees: note_id is col 1
    global_DBObjects.meetingattendeesmodel()->setFilter(1, noteId);
    global_DBObjects.meetingattendeesmodel()->refresh();

    // Note action items: note_id is col 13
    global_DBObjects.notesactionitemsmodel()->setFilter(13, noteId);
    global_DBObjects.notesactionitemsmodel()->refresh();
}

void DesktopAppController::refreshProjectNotes()
{ global_DBObjects.projectnotesmodel()->refresh(); }
void DesktopAppController::refreshMeetingAttendees()
{ global_DBObjects.meetingattendeesmodel()->refresh(); }
void DesktopAppController::refreshNoteActionItems()
{ global_DBObjects.notesactionitemsmodel()->refresh(); }
void DesktopAppController::refreshAllItems()
{ global_DBObjects.allitemsmodel()->refresh(); }
void DesktopAppController::refreshTeamMembers()
{ global_DBObjects.projectteammembersmodel()->refresh(); }
void DesktopAppController::refreshProjectLocations()
{ global_DBObjects.projectlocationsmodel()->refresh(); }
void DesktopAppController::refreshStatusItems()
{ global_DBObjects.statusreportitemsmodel()->refresh(); }
void DesktopAppController::refreshTrackerComments()
{ global_DBObjects.trackeritemscommentsmodel()->refresh(); }

// ── Lookups ──────────────────────────────────────────────────────────────────

QString DesktopAppController::projectIdAtRow(int row) const
{
    auto* proxy = global_DBObjects.projectinformationmodelproxy();
    if (!proxy || row < 0 || row >= proxy->rowCount()) return {};
    return proxy->data(proxy->index(row, 0)).toString();
}

int DesktopAppController::projectRowForId(const QString& projectId) const
{
    auto* proxy = global_DBObjects.projectinformationmodelproxy();
    if (!proxy || projectId.isEmpty()) return -1;
    for (int row = 0; row < proxy->rowCount(); ++row)
        if (proxy->data(proxy->index(row, 0)).toString() == projectId)
            return row;
    return -1;
}

QString DesktopAppController::projectNumberForId(const QString& projectId) const
{
    const int row = projectRowForId(projectId);
    if (row < 0) return {};
    auto* proxy = global_DBObjects.projectinformationmodelproxy();
    return proxy->data(proxy->index(row, 1)).toString();  // col 1 = project_number
}

QString DesktopAppController::projectNameForId(const QString& projectId) const
{
    const int row = projectRowForId(projectId);
    if (row < 0) return {};
    auto* proxy = global_DBObjects.projectinformationmodelproxy();
    return proxy->data(proxy->index(row, 2)).toString();  // col 2 = project_name
}

QString DesktopAppController::clientNameForId(const QString& clientId) const
{
    const int row = clientRowForId(clientId);
    if (row < 0) return {};
    auto* proxy = global_DBObjects.clientsmodelproxy();
    return proxy->data(proxy->index(row, 1)).toString();  // col 1 = client_name
}

int DesktopAppController::clientRowForId(const QString& clientId) const
{
    auto* proxy = global_DBObjects.clientsmodelproxy();
    if (!proxy || clientId.isEmpty()) return -1;
    for (int row = 0; row < proxy->rowCount(); ++row)
        if (proxy->data(proxy->index(row, 0)).toString() == clientId)
            return row;
    return -1;
}

QString DesktopAppController::clientIdAtRow(int row) const
{
    auto* proxy = global_DBObjects.clientsmodelproxy();
    if (!proxy || row < 0 || row >= proxy->rowCount()) return {};
    return proxy->data(proxy->index(row, 0)).toString();
}

int DesktopAppController::peopleRowForId(const QString& peopleId) const
{
    auto* proxy = global_DBObjects.peoplemodelproxy();
    if (!proxy || peopleId.isEmpty()) return -1;
    for (int row = 0; row < proxy->rowCount(); ++row)
        if (proxy->data(proxy->index(row, 0)).toString() == peopleId)
            return row;
    return -1;
}

QString DesktopAppController::peopleIdAtRow(int row) const
{
    auto* proxy = global_DBObjects.peoplemodelproxy();
    if (!proxy || row < 0 || row >= proxy->rowCount()) return {};
    return proxy->data(proxy->index(row, 0)).toString();
}

QString DesktopAppController::peopleNameForId(const QString& personId) const
{
    const int row = peopleRowForId(personId);
    if (row < 0) return {};
    auto* proxy = global_DBObjects.peoplemodelproxy();
    return proxy->data(proxy->index(row, 1)).toString();  // col 1 = name
}

// ── Picker lists ─────────────────────────────────────────────────────────────

QVariantList DesktopAppController::clientList() const
{
    QVariantList out;
    auto* proxy = global_DBObjects.clientsmodelproxy();
    if (!proxy) return out;
    for (int row = 0; row < proxy->rowCount(); ++row) {
        QVariantMap m;
        m.insert("id",   proxy->data(proxy->index(row, 0)).toString());
        m.insert("name", proxy->data(proxy->index(row, 1)).toString());
        out.append(m);
    }
    return out;
}

QVariantList DesktopAppController::peopleList() const
{
    QVariantList out;
    auto* proxy = global_DBObjects.peoplemodelproxy();
    if (!proxy) return out;
    for (int row = 0; row < proxy->rowCount(); ++row) {
        QVariantMap m;
        m.insert("id",   proxy->data(proxy->index(row, 0)).toString());
        m.insert("name", proxy->data(proxy->index(row, 1)).toString());
        out.append(m);
    }
    return out;
}

QVariantList DesktopAppController::teamMemberList(const QString& projectId,
                                                  const QStringList& includeIds) const
{
    QVariantList out;
    if (projectId.isEmpty()) return out;

    QSet<QString> seen;

    DB_LOCK;
    QSqlQuery qry(global_DBObjects.getDb());
    qry.prepare("SELECT people.id, people.name FROM project_people "
                "JOIN people ON people.id = project_people.people_id "
                "WHERE project_people.project_id = ? "
                "AND (project_people.deleted IS NULL OR project_people.deleted = 0) "
                "AND (people.deleted IS NULL OR people.deleted = 0) "
                "ORDER BY people.name");
    qry.addBindValue(projectId);
    qry.exec();
    while (qry.next()) {
        const QString id = qry.value(0).toString();
        QVariantMap m;
        m.insert("id",   id);
        m.insert("name", qry.value(1).toString());
        out.append(m);
        seen.insert(id);
    }
    DB_UNLOCK;

    // Keep any currently-referenced person who is no longer on the team so an
    // existing assignment still shows (the Widgets team combo does the same).
    for (const QString& id : includeIds) {
        if (id.isEmpty() || seen.contains(id)) continue;
        seen.insert(id);

        DB_LOCK;
        QSqlQuery pq(global_DBObjects.getDb());
        pq.prepare("SELECT name FROM people WHERE id = ?");
        pq.addBindValue(id);
        pq.exec();
        const bool found = pq.next();
        const QString name = found ? pq.value(0).toString() : QString();
        DB_UNLOCK;

        if (found) {
            QVariantMap m;
            m.insert("id",   id);
            m.insert("name", name);
            out.append(m);
        }
    }
    return out;
}

// ── Option lists ─────────────────────────────────────────────────────────────

QStringList DesktopAppController::projectStatusOptions() const
{ return DatabaseObjects::project_status; }
QStringList DesktopAppController::invoicingPeriodOptions() const
{ return DatabaseObjects::invoicing_period; }
QStringList DesktopAppController::statusReportPeriodOptions() const
{ return DatabaseObjects::status_report_period; }
QStringList DesktopAppController::itemTypeOptions() const
{ return DatabaseObjects::item_type; }
QStringList DesktopAppController::itemPriorityOptions() const
{ return DatabaseObjects::item_priority; }
QStringList DesktopAppController::itemStatusOptions() const
{ return DatabaseObjects::item_status; }
QStringList DesktopAppController::fileTypeOptions() const
{ return DatabaseObjects::file_types; }
QStringList DesktopAppController::statusItemCategoryOptions() const
{ return DatabaseObjects::status_item_status; }

// ── Projects ─────────────────────────────────────────────────────────────────

int DesktopAppController::addProject()
{
    auto* src   = global_DBObjects.projectinformationmodel();
    auto* proxy = global_DBObjects.projectinformationmodelproxy();

    const QModelIndex srcIdx = src->newRecord();
    if (!srcIdx.isValid()) return -1;
    if (!src->insertCacheRow(srcIdx.row())) return -1;

    const QString newId = src->data(src->index(srcIdx.row(), 0)).toString();
    if (!newId.isEmpty())
        global_DBObjects.addDefaultPMToProject(newId);

    return proxyRowFromSource(proxy, srcIdx);
}

bool DesktopAppController::deleteProject(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.projectinformationmodelproxy(),
                       global_DBObjects.projectinformationmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getProjectData(int row) const
{
    return proxyRowToMap(global_DBObjects.projectinformationmodelproxy(), row);
}

bool DesktopAppController::applyRowFields(QAbstractItemModel* model, int row,
        std::initializer_list<std::pair<int, QVariant>> fields)
{
    for (const auto& f : fields) {
        if (!model->setData(model->index(row, f.first), f.second)) {
            QString err = global_DBObjects.lastSaveError();
            if (err.isEmpty())
                err = tr("The record could not be saved.");
            emit errorOccurred(tr("Could Not Save"), err);
            return false;
        }
    }
    return true;
}

bool DesktopAppController::saveProject(int row,
        const QString& projectNumber, const QString& projectName,
        const QString& projectStatus, const QString& primaryContactId,
        const QString& clientId, const QString& lastStatusDate,
        const QString& lastInvoiceDate, const QString& invoicingPeriod,
        const QString& statusReportPeriod,
        const QString& budget, const QString& actual,
        const QString& bcwp, const QString& bcws,
        const QString& bac)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.projectinformationmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;

    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    if (!applyRowFields(model, pIdx.row(), {
            { 1, projectNumber},   { 2, projectName},        { 3, lastStatusDate},
            { 4, lastInvoiceDate}, { 5, primaryContactId},   { 6, budget},
            { 7, actual},          { 8, bcwp},               { 9, bcws},
            {10, bac},             {11, invoicingPeriod},    {12, statusReportPeriod},
            {13, clientId},        {14, projectStatus} }))
        return false;

    // The EVM columns (eac, cv, sv, cpi, pct_complete) are computed in the
    // SELECT, not stored, so per-cell setData leaves them stale in the cache.
    // Re-query the row so the recalculated values are picked up everywhere that
    // shares this model — the detail tiles and the project list card alike.
    if (pIdx.isValid()) {
        auto* proxy = global_DBObjects.projectinformationmodelproxy();
        global_DBObjects.projectinformationmodel()->reloadRecord(
            proxy->mapToSource(proxy->index(pIdx.row(), 0)));
    }
    return true;
}

// ── Project notes ────────────────────────────────────────────────────────────

int DesktopAppController::addProjectNote(const QString& projectId)
{
    QVariant fk(projectId);
    auto* src = global_DBObjects.projectnotesmodel();
    const QModelIndex srcIdx = src->newRecord(&fk);
    if (!srcIdx.isValid()) return -1;
    if (!src->insertCacheRow(srcIdx.row())) return -1;

    const QString newId = src->data(src->index(srcIdx.row(), 0)).toString();
    if (!newId.isEmpty())
        global_DBObjects.addDefaultPMToMeeting(newId);

    return proxyRowFromSource(global_DBObjects.projectnotesmodelproxy(), srcIdx);
}

bool DesktopAppController::deleteProjectNote(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.projectnotesmodelproxy(),
                       global_DBObjects.projectnotesmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getProjectNoteData(int row) const
{
    return proxyRowToMap(global_DBObjects.projectnotesmodelproxy(), row);
}

QString DesktopAppController::projectNoteIdAtRow(int row) const
{
    auto* proxy = global_DBObjects.projectnotesmodelproxy();
    if (!proxy || row < 0 || row >= proxy->rowCount()) return {};
    return proxy->data(proxy->index(row, 0)).toString();
}

bool DesktopAppController::saveProjectNote(int row, const QString& title, const QString& date,
                                           const QString& note, bool internalItem)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.projectnotesmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;

    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    return applyRowFields(model, pIdx.row(), {
        {2, title}, {3, date}, {4, note}, {5, internalItem ? "1" : "0"} });
}

// ── Meeting attendees ────────────────────────────────────────────────────────

int DesktopAppController::addAttendee(const QString& noteId)
{
    QVariant fk(noteId);
    return proxyRowFromSource(global_DBObjects.meetingattendeesmodelproxy(),
                              global_DBObjects.meetingattendeesmodel()->newRecord(&fk));
}

bool DesktopAppController::deleteAttendee(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.meetingattendeesmodelproxy(),
                       global_DBObjects.meetingattendeesmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getAttendeeData(int row) const
{
    return proxyRowToMap(global_DBObjects.meetingattendeesmodelproxy(), row);
}

bool DesktopAppController::saveAttendee(int row, const QString& personId)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.meetingattendeesmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;

    const QString safeNote   = model->data(model->index(row, 1)).toString().replace("'", "''");
    const QString safeId     = model->data(model->index(row, 0)).toString().replace("'", "''");
    const QString safePerson = QString(personId).replace("'", "''");
    const QString sql = QString(
        "SELECT COUNT(*) FROM meeting_attendees "
        "WHERE note_id = '%1' AND person_id = '%2' AND id != '%3' AND deleted = 0"
    ).arg(safeNote, safePerson, safeId);
    if (global_DBObjects.execute(sql).toInt() > 0) {
        const QString msg = tr("Attendee already exists.");
        global_DBObjects.setLastSaveError(msg);
        emit errorOccurred(tr("Could Not Save"), msg);
        return false;
    }

    const bool ok = model->setData(model->index(row, 2), personId);
    if (!ok)
        emit errorOccurred(tr("Could Not Save"), global_DBObjects.lastSaveError());
    return ok;
}

// ── Note action items ────────────────────────────────────────────────────────

int DesktopAppController::addNoteActionItem(const QString& noteId, const QString& projectId)
{
    QVariant fk1(noteId);
    QVariant fk2(projectId);
    auto* src = global_DBObjects.notesactionitemsmodel();
    const QModelIndex srcIdx = src->newRecord(&fk1, &fk2);
    if (!srcIdx.isValid()) return -1;
    src->insertCacheRow(srcIdx.row());
    src->refresh();
    return proxyRowFromSource(global_DBObjects.notesactionitemsmodelproxy(), srcIdx);
}

bool DesktopAppController::deleteNoteActionItem(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.notesactionitemsmodelproxy(),
                       global_DBObjects.notesactionitemsmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getNoteActionItemData(int row) const
{ return proxyRowToMap(global_DBObjects.notesactionitemsmodelproxy(), row); }

bool DesktopAppController::saveNoteActionItem(int row, const QString& itemName,
        const QString& itemType, const QString& priority, const QString& status,
        const QString& assignedTo, const QString& identifiedBy,
        const QString& dateIdentified, const QString& dateDue, const QString& description)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.notesactionitemsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    // notesActionItemsModel columns: 2 item_type, 3 item_name, 4 identified_by,
    // 5 date_identified, 6 description, 7 assigned_to, 8 priority, 9 status,
    // 10 date_due (see notesactionitemsmodel.cpp SELECT order).
    return applyRowFields(model, row, {
        {3,  itemName},      {2, itemType},   {8,  priority},
        {9,  status},        {7, assignedTo}, {4,  identifiedBy},
        {5,  dateIdentified}, {10, dateDue},  {6,  description} });
}

// ── People ───────────────────────────────────────────────────────────────────

int DesktopAppController::addPerson()
{
    return proxyRowFromSource(global_DBObjects.peoplemodelproxy(),
                              global_DBObjects.peoplemodel()->newRecord());
}

bool DesktopAppController::deletePerson(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.peoplemodelproxy(),
                       global_DBObjects.peoplemodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getPersonData(int row) const
{ return proxyRowToMap(global_DBObjects.peoplemodelproxy(), row); }

QString DesktopAppController::personIdAtRow(int row) const
{
    auto* proxy = global_DBObjects.peoplemodelproxy();
    if (!proxy || row < 0 || row >= proxy->rowCount()) return {};
    return proxy->data(proxy->index(row, 0)).toString();
}

bool DesktopAppController::savePerson(int row, const QString& name, const QString& email,
                                      const QString& officePhone, const QString& cellPhone,
                                      const QString& clientId, const QString& role)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.peoplemodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    return applyRowFields(model, pIdx.row(), {
        {1, name},      {2, email},    {3, officePhone},
        {4, cellPhone}, {5, clientId}, {6, role} });
}

// ── Clients ──────────────────────────────────────────────────────────────────

int DesktopAppController::addClient()
{
    return proxyRowFromSource(global_DBObjects.clientsmodelproxy(),
                              global_DBObjects.clientsmodel()->newRecord());
}

bool DesktopAppController::deleteClient(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.clientsmodelproxy(),
                       global_DBObjects.clientsmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getClientData(int row) const
{ return proxyRowToMap(global_DBObjects.clientsmodelproxy(), row); }

QString DesktopAppController::clientIdAtProxyRow(int row) const
{ return clientIdAtRow(row); }

bool DesktopAppController::saveClient(int row, const QString& clientName)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.clientsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    const bool ok = model->setData(model->index(row, 1), clientName);
    if (!ok)
        emit errorOccurred(tr("Could Not Save"), global_DBObjects.lastSaveError());
    return ok;
}

// ── Tracker items ────────────────────────────────────────────────────────────

void DesktopAppController::openTrackerItem(const QString& itemId)
{
    // Detail model filtered to the one item (col 0 = id).
    global_DBObjects.actionitemsdetailsmodel()->setFilter(0, itemId);
    global_DBObjects.actionitemsdetailsmodel()->refresh();

    // Keep the per-project team-member list in sync so the Identified By /
    // Assigned To combos resolve names.
    QString projectId;
    if (global_DBObjects.actionitemsdetailsmodel()->rowCount(QModelIndex()) > 0)
        projectId = global_DBObjects.actionitemsdetailsmodel()
                        ->data(global_DBObjects.actionitemsdetailsmodel()->index(0, 14)).toString();

    if (!projectId.isEmpty())
        global_DBObjects.projectteammembersmodel()->setFilter(1, projectId);
    else
        global_DBObjects.projectteammembersmodel()->clearFilter(1);
    global_DBObjects.projectteammembersmodel()->refresh();

    // Comments filtered to the same item (col 1 = item_id).
    global_DBObjects.trackeritemscommentsmodel()->setFilter(1, itemId);
    global_DBObjects.trackeritemscommentsmodel()->refresh();
}

int DesktopAppController::addTrackerItem(const QString& projectId)
{
    QVariant fk(projectId);
    auto* src = global_DBObjects.actionitemsdetailsmodel();
    const QModelIndex srcIdx = src->newRecord(&fk);
    if (!srcIdx.isValid()) return -1;
    src->insertCacheRow(srcIdx.row());

    const QString newId = src->data(src->index(srcIdx.row(), 0)).toString();
    if (!newId.isEmpty())
        openTrackerItem(newId);
    return 0;  // detail model filtered to the new item → row 0
}

bool DesktopAppController::deleteTrackerItemDetail(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.actionitemsdetailsmodelproxy(),
                       global_DBObjects.actionitemsdetailsmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getTrackerItemDetailData(int row) const
{ return proxyRowToMap(global_DBObjects.actionitemsdetailsmodelproxy(), row); }

QString DesktopAppController::allItemIdAtRow(int row) const
{
    auto* proxy = global_DBObjects.allitemsmodelproxy();
    if (!proxy || row < 0 || row >= proxy->rowCount()) return {};
    return proxy->data(proxy->index(row, 0)).toString();
}

bool DesktopAppController::isItemNameUnique(const QString& projectId, const QString& itemId,
                                            const QString& itemName) const
{
    if (itemName.trimmed().isEmpty() || projectId.trimmed().isEmpty()) return true;
    const QString sql = QString(
        "SELECT COUNT(*) FROM item_tracker WHERE project_id = '%1' AND item_name = '%2' "
        "AND id != '%3' AND deleted = 0")
        .arg(projectId.trimmed().replace("'", "''"),
             itemName.trimmed().replace("'", "''"),
             itemId.trimmed().replace("'", "''"));
    return global_DBObjects.execute(sql).toInt() == 0;
}

bool DesktopAppController::isItemNumberUnique(const QString& projectId, const QString& itemId,
                                              const QString& itemNumber) const
{
    if (itemNumber.trimmed().isEmpty() || projectId.trimmed().isEmpty()) return true;
    const QString sql = QString(
        "SELECT COUNT(*) FROM item_tracker WHERE project_id = '%1' AND item_number = '%2' "
        "AND id != '%3' AND deleted = 0")
        .arg(projectId.trimmed().replace("'", "''"),
             itemNumber.trimmed().replace("'", "''"),
             itemId.trimmed().replace("'", "''"));
    return global_DBObjects.execute(sql).toInt() == 0;
}

bool DesktopAppController::saveTrackerItemDetail(int row, const QString& itemId,
        const QString& itemNumber, const QString& itemType, const QString& itemName,
        const QString& description, const QString& identifiedBy, const QString& assignedTo,
        const QString& priority, const QString& status, const QString& dateIdentified,
        const QString& dateDue, bool internalItem)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.actionitemsdetailsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    const QString projectId = model->data(model->index(pIdx.row(), 14)).toString();

    if (!isItemNameUnique(projectId, itemId, itemName)) {
        const QString msg = tr("\"%1\" is already in use").arg(itemName.trimmed());
        global_DBObjects.setLastSaveError(msg);
        emit errorOccurred(tr("Could Not Save"), msg);
        return false;
    }
    if (!isItemNumberUnique(projectId, itemId, itemNumber)) {
        const QString msg = tr("Item number \"%1\" is already in use").arg(itemNumber.trimmed());
        global_DBObjects.setLastSaveError(msg);
        emit errorOccurred(tr("Could Not Save"), msg);
        return false;
    }

    // Status must be written BEFORE assigned_to: TrackerItemsModel::setData
    // auto-advances a "New" item to "Assigned" when assigned_to is set. If status
    // were written after, it would clobber that auto-advance back to "New".
    return applyRowFields(model, pIdx.row(), {
        { 1, itemNumber},    { 2, itemType},   { 3, itemName},
        { 4, identifiedBy},  { 5, dateIdentified}, { 6, description},
        { 8, priority},      { 9, status},     { 7, assignedTo},
        {10, dateDue},       {15, internalItem ? "1" : "0"} });
}

// ── Tracker item comments ────────────────────────────────────────────────────

int DesktopAppController::addComment(const QString& itemId)
{
    // newRecord() only appends an in-memory row (with item_id + defaults set);
    // insertCacheRow() is what assigns a UUID and persists it. Without the
    // insert, the following refresh (here or from QML) discarded the comment —
    // the Add Comment button appeared to do nothing.
    QVariant fk(itemId);
    auto* src = global_DBObjects.trackeritemscommentsmodel();
    const QModelIndex srcIdx = src->newRecord(&fk);
    if (!srcIdx.isValid()) return -1;
    src->insertCacheRow(srcIdx.row());
    src->refresh();
    return proxyRowFromSource(global_DBObjects.trackeritemscommentsmodelproxy(), srcIdx);
}

bool DesktopAppController::deleteComment(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.trackeritemscommentsmodelproxy(),
                       global_DBObjects.trackeritemscommentsmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getCommentData(int row) const
{ return proxyRowToMap(global_DBObjects.trackeritemscommentsmodelproxy(), row); }

bool DesktopAppController::saveComment(int row, const QString& date,
                                       const QString& note, const QString& updatedBy)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.trackeritemscommentsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    return applyRowFields(model, pIdx.row(), {
        {2, date}, {3, note}, {4, updatedBy} });
}

// ── Project team members ─────────────────────────────────────────────────────

int DesktopAppController::addTeamMember(const QString& projectId)
{
    QVariant fk(projectId);
    return proxyRowFromSource(global_DBObjects.projectteammembersmodelproxy(),
                              global_DBObjects.projectteammembersmodel()->newRecord(&fk));
}

bool DesktopAppController::deleteTeamMember(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.projectteammembersmodelproxy(),
                       global_DBObjects.projectteammembersmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getTeamMemberData(int row) const
{ return proxyRowToMap(global_DBObjects.projectteammembersmodelproxy(), row); }

bool DesktopAppController::saveTeamMember(int row, const QString& peopleId,
                                          const QString& role, bool receiveStatusReport)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.projectteammembersmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    // Guard against adding the same person to a project twice. There is a partial
    // unique index on project_people(project_id, people_id) WHERE deleted = 0, so a
    // duplicate would otherwise surface as a raw SQL failure. Column 0 is the row id
    // (empty for the not-yet-saved row we are filling in), column 1 is project_id.
    const QString rowId     = model->index(pIdx.row(), 0).data().toString();
    const QString projectId = model->index(pIdx.row(), 1).data().toString();
    if (!peopleId.isEmpty() && !projectId.isEmpty())
    {
        DB_LOCK;
        QSqlQuery dup(global_DBObjects.getDb());
        dup.prepare("SELECT count(*) FROM project_people "
                    "WHERE project_id = ? AND people_id = ? AND deleted = 0 AND id <> ?");
        dup.addBindValue(projectId);
        dup.addBindValue(peopleId);
        dup.addBindValue(rowId);
        dup.exec();
        const bool exists = dup.next() && dup.value(0).toInt() > 0;
        DB_UNLOCK;

        if (exists)
        {
            emit errorOccurred(tr("Duplicate Team Member"),
                               tr("That person is already a team member on this project."));
            return false;
        }
    }

    // people_id is written first: it is NOT NULL in the schema and carries the
    // unique key, so it is the write that can be rejected. applyRowFields stops at
    // the first failure, so a rejected people_id never lets the later writes insert
    // a row with a null people_id (which would raise a raw SQL constraint error).
    return applyRowFields(model, pIdx.row(), {
        {2, peopleId}, {4, receiveStatusReport ? "1" : "0"}, {5, role} });
}

// ── Project locations ────────────────────────────────────────────────────────

int DesktopAppController::addProjectLocation(const QString& projectId)
{
    QVariant fk(projectId);
    return proxyRowFromSource(global_DBObjects.projectlocationsmodelproxy(),
                              global_DBObjects.projectlocationsmodel()->newRecord(&fk));
}

bool DesktopAppController::deleteProjectLocation(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.projectlocationsmodelproxy(),
                       global_DBObjects.projectlocationsmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getProjectLocationData(int row) const
{ return proxyRowToMap(global_DBObjects.projectlocationsmodelproxy(), row); }

bool DesktopAppController::saveProjectLocation(int row, const QString& locationType,
                                               const QString& description, const QString& path)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.projectlocationsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    return applyRowFields(model, pIdx.row(), {
        {2, locationType}, {3, description}, {4, path} });
}

bool DesktopAppController::setProjectLocationPath(int row, const QString& fileUrlOrPath)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.projectlocationsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    // Setting the path (col 4) makes ProjectLocationsModel auto-detect the file
    // type (col 2) and, when it is still empty, the description (col 3).
    if (!model->setData(model->index(row, 4), localPath(fileUrlOrPath))) {
        QString err = global_DBObjects.lastSaveError();
        if (err.isEmpty()) err = tr("The location could not be saved.");
        emit errorOccurred(tr("Could Not Save"), err);
        return false;
    }
    return true;
}

bool DesktopAppController::addProjectLocationFromUrl(const QString& projectId,
                                                     const QString& fileUrlOrPath)
{
    if (localPath(fileUrlOrPath).isEmpty()) return false;
    const int row = addProjectLocation(projectId);
    if (row < 0) return false;
    return setProjectLocationPath(row, fileUrlOrPath);
}

void DesktopAppController::openProjectLocation(int row)
{
    QAbstractItemModel* model = global_DBObjects.projectlocationsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return;
    const QString path = model->data(model->index(row, 4)).toString();
    if (path.isEmpty()) return;

    // A stored path with a URL scheme — http(s), the ms-office deep links that
    // ProjectLocationsModel writes for Office web documents, mailto, file, … — is
    // handed to the OS as a URL so the browser or the registered Office handler
    // opens it; a bare filesystem path is opened as a local file.
    static const QStringList urlSchemes = {
        "http:", "https:", "ftp:", "mailto:", "file:",
        "ms-word:", "ms-excel:", "ms-powerpoint:", "ms-project:",
        "ms-visio:", "ms-access:", "onenote:"
    };
    for (const QString& s : urlSchemes) {
        if (path.startsWith(s, Qt::CaseInsensitive)) {
            QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
            return;
        }
    }

    if (path.startsWith("www.", Qt::CaseInsensitive)) {
        QDesktopServices::openUrl(QUrl("https://" + path, QUrl::TolerantMode));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

// ── Status report items ──────────────────────────────────────────────────────

int DesktopAppController::addStatusItem(const QString& projectId)
{
    QVariant fk(projectId);
    return proxyRowFromSource(global_DBObjects.statusreportitemsmodelproxy(),
                              global_DBObjects.statusreportitemsmodel()->newRecord(&fk));
}

bool DesktopAppController::deleteStatusItem(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.statusreportitemsmodelproxy(),
                       global_DBObjects.statusreportitemsmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getStatusItemData(int row) const
{ return proxyRowToMap(global_DBObjects.statusreportitemsmodelproxy(), row); }

bool DesktopAppController::saveStatusItem(int row, const QString& category, const QString& description)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.statusreportitemsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    return applyRowFields(model, pIdx.row(), {
        {2, category}, {3, description} });
}

QString DesktopAppController::lastSaveError() const
{
    return global_DBObjects.lastSaveError();
}

// ── Cloud sync ────────────────────────────────────────────────────────────────
//
// Reads the same QSettings the Widgets app writes (organization
// "ProjectNotes"[+profile], application "AppSettings"), so both apps share one
// sync configuration. The engine runs on its own thread to keep the UI responsive.

QString DesktopAppController::syncSetting(const QString& key) const
{
    QSettings s(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    return s.value(key).toString();
}

void DesktopAppController::setSyncSetting(const QString& key, const QVariant& value)
{
    QSettings s(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    s.setValue(key, value);
}

bool    DesktopAppController::syncEnabled() const { return syncSetting("Sync/Enabled") == "true"; }
QString DesktopAppController::syncEmail() const { return syncSetting("Sync/Email"); }
QString DesktopAppController::syncPassword() const { return syncSetting("Sync/Password"); }
QString DesktopAppController::syncEncryptionPhrase() const { return syncSetting("Sync/EncryptionPhrase"); }

void DesktopAppController::setSyncEnabled(bool v)
{ setSyncSetting("Sync/Enabled", v); emit syncSettingsChanged(); }
void DesktopAppController::setSyncEmail(const QString& v)
{ setSyncSetting("Sync/Email", v); emit syncSettingsChanged(); }
void DesktopAppController::setSyncPassword(const QString& v)
{ setSyncSetting("Sync/Password", v); emit syncSettingsChanged(); }
void DesktopAppController::setSyncEncryptionPhrase(const QString& v)
{ setSyncSetting("Sync/EncryptionPhrase", v); emit syncSettingsChanged(); }

QString DesktopAppController::supabaseConnectionInfo() const
{
    const QString projectId = s_testSupabase ? QStringLiteral("lsulnvxgrlpuqtzonner")
                                             : QStringLiteral("nrtjpzkrldwydkbopsml");
    const QString env = s_testSupabase ? tr("Test") : tr("Production");
    return tr("Project ID: %1 (%2)").arg(projectId, env);
}

void DesktopAppController::setSyncProgress(qreal progress, bool hasError)
{
    if (qFuzzyCompare(m_syncProgress, progress) && m_syncHasError == hasError)
        return;
    m_syncProgress = progress;
    m_syncHasError = hasError;
    emit syncProgressChanged();
}

void DesktopAppController::setSubscriptionStatusText(const QString& text)
{
    if (m_subscriptionStatusText == text) return;
    m_subscriptionStatusText = text;
    emit subscriptionStatusChanged();
}

void DesktopAppController::configureSyncApi()
{
    if (!m_syncApi) {
        m_syncApiThread = new QThread(this);
        m_syncApiThread->setObjectName(QStringLiteral("SqliteSyncProThread"));

        m_syncApi = new SqliteSyncPro;   // no parent — lives on the API thread
        m_syncApi->moveToThread(m_syncApiThread);

        connect(m_syncApiThread, &QThread::finished, m_syncApi, &QObject::deleteLater);
        connect(m_syncApi, &SqliteSyncPro::rowChanged,        this, &DesktopAppController::onSyncRowChanged);
        connect(m_syncApi, &SqliteSyncPro::syncCompleted,     this, &DesktopAppController::onSyncComplete);
        connect(m_syncApi, &SqliteSyncPro::syncProgress,      this, &DesktopAppController::onSyncProgress);
        connect(m_syncApi, &SqliteSyncPro::syncStatusUpdated, this, &DesktopAppController::onSyncStatusUpdated);

        m_syncApiThread->start();
    }

    // Supabase endpoint — TEST vs production per --test-supabase.
    const QString supabaseUrl = s_testSupabase
        ? QStringLiteral("https://lsulnvxgrlpuqtzonner.supabase.co")
        : QStringLiteral("https://nrtjpzkrldwydkbopsml.supabase.co");
    const QString supabaseKey = s_testSupabase
        ? QStringLiteral("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImxzdWxudnhncmxwdXF0em9ubmVyIiwicm9sZSI6ImFub24iLCJpYXQiOjE3Nzg1ODY0OTIsImV4cCI6MjA5NDE2MjQ5Mn0.AyEQHLZadhj5r0BNkvPASaMZ0gTr4LAueq0SGVuua3s")
        : QStringLiteral("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im5ydGpwemtybGR3eWRrYm9wc21sIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzM4NTU0NTQsImV4cCI6MjA4OTQzMTQ1NH0.hzzyb5bFKDIFbrJ7Fa8INh57pWIkz52csQ2gQ_L302E");

    m_syncApi->setSyncHostType(1);        // always Supabase
    m_syncApi->setPostgrestUrl(supabaseUrl);
    m_syncApi->setSupabaseKey(supabaseKey);
    m_syncApi->setEmail(syncEmail());
    m_syncApi->setPassword(syncPassword());
    m_syncApi->setEncryptionPhrase(syncEncryptionPhrase());
    // Share the DatabaseObjects write lock so the engine and the UI never write
    // concurrently (mirrors the Widgets app).
    m_syncApi->setDatabaseLock(&db_rwlock);
}

void DesktopAppController::syncNow()
{
    if (!syncEnabled() || !global_DBObjects.isOpen())
        return;

    configureSyncApi();

    SqliteSyncPro* api = m_syncApi;
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!s_developerProfile.isEmpty())
        dataDir += "/" + s_developerProfile;
    const QString dbPath = dataDir + "/ProjectNotes.db";

    setSyncProgress(0.01, false);   // show the indicator immediately

    QMetaObject::invokeMethod(api, [this, api, dbPath]() {
        if (api->isInitialized()) {
            api->retryNow();
            return;
        }
        api->setDatabasePath(dbPath);
        if (!api->initialize()) {
            QMetaObject::invokeMethod(this, [this]() { setSyncProgress(0.0, true); },
                                      Qt::QueuedConnection);
            return;
        }
        if (api->isAuthenticated()) {
            const SubscriptionStatus sub = api->getSubscriptionStatus();
            QMetaObject::invokeMethod(this, [this, api, sub]() {
                QString text;
                if (sub.valid) {
                    const bool isActive =
                        sub.status.compare(QLatin1String("active"),   Qt::CaseInsensitive) == 0 ||
                        sub.status.compare(QLatin1String("trialing"), Qt::CaseInsensitive) == 0;
                    const QString color = isActive ? QStringLiteral("#27ae60") : QStringLiteral("#c0442e");
                    const QString word = sub.status.isEmpty() ? tr("None")
                        : (sub.status.at(0).toUpper() + sub.status.mid(1).toLower());
                    text = tr("Subscription: %1").arg(word);
                    if (sub.hasActiveSubscription && sub.currentPeriodEnd.isValid())
                        text += tr(" (renews %1)").arg(sub.currentPeriodEnd.toString(QStringLiteral("MMM d, yyyy")));
                    Q_UNUSED(color)
                } else {
                    text = tr("Subscription status unavailable");
                }
                setSubscriptionStatusText(text);
                if (sub.valid && !sub.hasActiveSubscription) {
                    emit subscriptionExpired();
                    QMetaObject::invokeMethod(api, [api]() { api->shutdown(); }, Qt::QueuedConnection);
                }
            }, Qt::QueuedConnection);
        }
    }, Qt::QueuedConnection);
}

void DesktopAppController::stopSync()
{
    if (!m_syncApi) return;
    SqliteSyncPro* api = m_syncApi;
    QMetaObject::invokeMethod(api, [api]() { api->shutdown(); }, Qt::QueuedConnection);
    setSyncProgress(-1.0, false);
}

void DesktopAppController::onSyncRowChanged(const QString& tableName, const QString& id)
{
    global_DBObjects.pushRowChange(tableName, id, KeyColumnChange::Update);
}

void DesktopAppController::onSyncComplete(const SyncResult& result)
{
    global_DBObjects.updateDisplayData();
    if (FolderManager* fm = FolderManager::instance())
        fm->reload();   // pick up folder defs/memberships pulled from another device
    m_syncNetworkError = result.hasNetworkError();
    if (result.success) {
        m_syncHasError = false;
        if (SqliteSyncPro* api = m_syncApi)
            QMetaObject::invokeMethod(api, [api, result]() { api->checkSyncStatus(result); },
                                      Qt::QueuedConnection);
    } else {
        m_syncHasError = true;
    }
    emit syncProgressChanged();
}

void DesktopAppController::onSyncProgress(const QString&, int, int)
{
    if (m_syncProgress < 0)
        setSyncProgress(0.01, m_syncHasError);
}

// Mirrors the Widgets app's onSyncStatusUpdated: percentComplete drives a
// percentage indicator, visible while < 100 and hidden at 100; the pending
// push/pull counts feed the detail text.
void DesktopAppController::onSyncStatusUpdated(int percentComplete, qint64 pendingPush, qint64 pendingPull)
{
    m_syncPercent     = percentComplete;
    m_syncPendingPush = pendingPush;
    m_syncPendingPull = pendingPull;

    if (m_syncNetworkError) {
        m_syncProgress = -1.0;          // hide bar; the offline indicator takes over
    } else if (percentComplete >= 100) {
        m_syncProgress = -1.0;          // fully synced — hide
        m_syncHasError = false;
    } else {
        m_syncProgress = percentComplete / 100.0;
    }
    emit syncProgressChanged();
}

QString DesktopAppController::syncDetail() const
{
    if (m_syncNetworkError)
        return tr("Offline — can't reach the sync host");
    if (m_syncProgress >= 0.0 && m_syncPercent < 100)
        return tr("%1% synced · pulling %2, pushing %3 records")
                   .arg(m_syncPercent).arg(m_syncPendingPull).arg(m_syncPendingPush);
    if (!syncEnabled())
        return tr("Sync disabled");
    return tr("Up to date");
}
