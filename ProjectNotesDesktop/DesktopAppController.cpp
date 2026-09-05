// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "DesktopAppController.h"

#include "databaseobjects.h"
#include "vcardparser.h"
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
#include "FileFinderService.h"

#include "pluginmanager.h"
#include "plugin.h"
#include "pythonworker.h"

#include "sqlitesyncpro.h"
#include "syncresult.h"
#include "synclog.h"
// Used by verifySyncSettings() to check the saved credentials and encryption
// phrase directly, without spinning up the sync engine.
#include "authmanager.h"
#include "httpclient.h"
#include "rowencryption.h"

#include "QLogger.h"
#include "version.h"
#include "updatemanager.h"

#include <algorithm>

#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDomDocument>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QFontInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGuiApplication>
#include <QKeySequence>
#include <QPointer>
#include <QScreen>
#include <QSet>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

#include <private/qzipwriter_p.h>

DesktopAppController* DesktopAppController::s_instance = nullptr;
QString DesktopAppController::s_developerProfile;
bool    DesktopAppController::s_testSupabase = false;
bool    DesktopAppController::s_updateChecksEnabled = true;

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

    m_fileFinder = new FileFinderService(this);
    connect(m_fileFinder, &FileFinderService::locationsChanged, this,
            [](int, int) {
        // The worker commits a complete scan in one transaction, so refresh
        // dependent views once rather than once for every discovered location.
        global_DBObjects.projectlocationsmodel()->refresh();
        global_DBObjects.searchresultsmodel()->markDirty();
    });

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

    // Structured logging to the shared logs folder (same destinations the Widgets
    // app configures), so the desktop app writes its own logs and the Log Viewer
    // has current content to show.
    const QString logloc = dataLocation() + "/logs";
    QDir().mkpath(logloc);

    QLoggerManager* logmanager = QLoggerManager::getInstance();
#ifdef QT_DEBUG
    logmanager->addDestination("debugging.log", DEBUGLOG, LogLevel::Debug, logloc, LogMode::OnlyFile);
#endif
    logmanager->addDestination("error.log", ERRORLOG, LogLevel::Error, logloc, LogMode::OnlyFile);
    logmanager->addDestination("console.log", CONSOLELOG, LogLevel::Info, logloc, LogMode::OnlyFile);
    logmanager->addDestination("syncerrors.log", SYNCERRORLOG, LogLevel::Warning, logloc, LogMode::OnlyFile);
    logmanager->resume();

    // Route sync-library failures (raised via SyncLog::error from inside
    // SqliteSyncProLib) into syncerrors.log, mirroring the Widgets app.
    SyncLog::setErrorSink([](const QString& msg) {
        QLog_Warning(SYNCERRORLOG, msg);
    });
}

QString DesktopAppController::dataLocation()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!s_developerProfile.isEmpty())
        base += "/" + s_developerProfile;
    return base;
}

DesktopAppController::~DesktopAppController()
{
    // Stop the finder's worker and close its connection before the shared
    // application database is closed below.
    delete m_fileFinder;
    m_fileFinder = nullptr;

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
// message (external, non-owned references) via errorOccurred() on failure.
// Owned child records are cascade-deleted rather than blocking the delete.
static bool deleteProxyRow(SortFilterProxyModel* proxy, SqlQueryModel* source, int row)
{
    const QModelIndex proxyIdx = proxy->index(row, 0);
    if (!proxyIdx.isValid()) return false;
    return source->deleteRecord(proxy->mapToSource(proxyIdx));
}

// Reapplies a model's persisted column filters (application_settings-backed);
// defined further down alongside applyColumnFilters()/clearColumnFilters().
static void restoreColumnFilters(SqlQueryModel* src);
// Reapplies a model's persisted sort order; defined further down alongside
// applySort()/clearSort().
static void restoreSort(QAbstractItemModel* model);

// ── Database ─────────────────────────────────────────────────────────────────

bool DesktopAppController::openOrCreateDatabase()
{
    if (m_databaseOpen)
        return true;

    const QString dataDir = dataLocation();   // matches AppSettings::dataLocation()
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

    // Keep the sidebar's per-folder snapshots in step with the projects proxy.
    // Connected BEFORE the initial refresh() calls below so their resets (and
    // everything after) invalidate any snapshot cached during the QML load.
    // (FolderManager's own signal is hooked up lazily in rebuildFolderSnapshot.)
    if (auto* projProxy = global_DBObjects.projectinformationmodelproxy()) {
        connect(projProxy, &QAbstractItemModel::modelReset,
                this, &DesktopAppController::invalidateFolderSnapshot);
        connect(projProxy, &QAbstractItemModel::rowsInserted,
                this, &DesktopAppController::invalidateFolderSnapshot);
        connect(projProxy, &QAbstractItemModel::rowsRemoved,
                this, &DesktopAppController::invalidateFolderSnapshot);
        connect(projProxy, &QAbstractItemModel::layoutChanged,
                this, &DesktopAppController::invalidateFolderSnapshot);
        connect(projProxy, &QAbstractItemModel::dataChanged,
                this, &DesktopAppController::invalidateFolderSnapshot);
    }

    global_DBObjects.setGlobalSearches(false);
    global_DBObjects.projectinformationmodel()->refresh();
    global_DBObjects.clientsmodel()->refresh();
    global_DBObjects.peoplemodel()->refresh();

    // Restore any column filters left active in the Filter Editor before the
    // last restart (persisted to application_settings, keyed per table). No-op
    // (and no extra refresh) for models with nothing persisted; allitemsmodel
    // is otherwise loaded lazily, so this only forces it early when needed.
    restoreColumnFilters(global_DBObjects.projectinformationmodel());
    restoreColumnFilters(global_DBObjects.clientsmodel());
    restoreColumnFilters(global_DBObjects.peoplemodel());
    restoreColumnFilters(global_DBObjects.allitemsmodel());

    // restoreColumnFilters() mutates the models directly (it isn't routed
    // through applyColumnFilters()), so it never bumps filterRev on its own.
    // That's normally fine — QML bindings read the model fresh on their first
    // evaluation — but Main.qml's root Component.onCompleted (which calls
    // openOrCreateDatabase(), and lands here) runs synchronously during
    // engine.load(), before window->show(). That means the whole QML tree,
    // including the TopBar's filterActive binding, has already latched its
    // *first* value (against an empty pre-open model) by the time the four
    // restores above run. Without an explicit bump here, a filter restored
    // from a prior session leaves the Filter button unhighlighted until some
    // unrelated filter change elsewhere bumps filterRev, or the user
    // navigates off the initial section ("projects") and back — since a
    // section change is itself a tracked dependency of that binding.
    ++m_filterRev;
    emit filterRevChanged();

    // Restore any persisted sort choice, for every filterable section (not
    // just the four above) — unlike column filters, sorting an unloaded model
    // is cheap (it only sets the pending column/order; the proxy applies it
    // when it next builds its row mapping), so there's no reason to limit it.
    for (const QString& section : { QStringLiteral("projects"), QStringLiteral("items"),
                                     QStringLiteral("people"), QStringLiteral("clients"),
                                     QStringLiteral("statusreport"), QStringLiteral("trackeritems"),
                                     QStringLiteral("team"), QStringLiteral("locations"),
                                     QStringLiteral("notes") })
        restoreSort(modelForSection(section));

    // Same reasoning as the filterRev bump above, for the Sort chip's
    // sortActive binding.
    ++m_sortRev;
    emit sortRevChanged();

    // The sidebar may have cached an empty snapshot while the QML tree was
    // building (pre-open); force a rebuild + rev bump now that data is loaded.
    invalidateFolderSnapshot();

    m_fileFinder->initialize(dbPath, &db_rwlock,
                             QStringLiteral("ProjectNotes") + s_developerProfile);

    m_databaseOpen = true;
    emit databaseReady();
    emit projectManagerChanged();   // picks up any PM configured in a prior session

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

QObject* DesktopAppController::fileFinder() const
{ return m_fileFinder; }

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

// Reads each dropped file (fileUrls, as local file:// URLs) and appends its
// content to any raw vCard text the drop already carried directly (a MIME
// vCard drag, or plain text starting with BEGIN:VCARD). QML's DropArea hands
// urls and text over separately (unlike QMimeData), so this stands in for
// vcardparser.h's extractVCardText() for the QML drop path — the combined
// result still just gets fed to parseVCards(), which only picks out actual
// BEGIN:VCARD…END:VCARD blocks, so unrelated file content is harmless.
static QString combineVCardSources(const QStringList& fileUrls, const QString& text)
{
    QString combined = text;
    for (const QString& fileUrl : fileUrls)
    {
        QFile file(localPath(fileUrl));
        if (file.open(QFile::ReadOnly))
            combined += QString::fromUtf8(file.readAll()) + "\n";
    }
    return combined;
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
{
    global_DBObjects.setProjectManager(personId);
    emit projectManagerChanged();
}

QString DesktopAppController::projectManagerInitials() const
{
    // Bound eagerly by IconRail.qml as part of the always-visible rail, which is
    // constructed before Main.qml's Component.onCompleted calls
    // openOrCreateDatabase() — querying the DB here before then would hit a
    // closed/prepared-on-nothing QSqlQuery and surface a "Database Access
    // Failed" dialog. projectManagerChanged() re-fires once the DB opens (see
    // openOrCreateDatabase) so the real value populates right after.
    if (!m_databaseOpen)
        return QString();

    const QString personId = global_DBObjects.getProjectManager();
    if (personId.isEmpty())
        return QString();

    QString name;
    auto* proxy = global_DBObjects.peoplemodelproxy();
    if (proxy) {
        for (int row = 0; row < proxy->rowCount(); ++row) {
            if (proxy->data(proxy->index(row, 0)).toString() == personId) {
                name = proxy->data(proxy->index(row, 1)).toString();
                break;
            }
        }
    }
    if (name.isEmpty())
        return QString();

    const QStringList parts = name.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return QString();

    QString initials = parts.first().left(1);
    if (parts.size() > 1)
        initials += parts.last().left(1);
    return initials.toUpper();
}

int DesktopAppController::lastProjectDetailTab(const QString& projectId) const
{ return global_DBObjects.getLastProjectDetailTab(projectId); }
void DesktopAppController::setLastProjectDetailTab(const QString& projectId, int index)
{ global_DBObjects.setLastProjectDetailTab(projectId, index); }

int DesktopAppController::projectDetailHeaderHeight() const
{ return global_DBObjects.getProjectDetailHeaderHeight(); }
void DesktopAppController::setProjectDetailHeaderHeight(int height)
{ global_DBObjects.setProjectDetailHeaderHeight(height); }

int DesktopAppController::projectSidebarWidth() const
{ return global_DBObjects.getProjectSidebarWidth(); }
void DesktopAppController::setProjectSidebarWidth(int width)
{ global_DBObjects.setProjectSidebarWidth(width); }

int DesktopAppController::uiZoomPercent() const
{ return global_DBObjects.getUiZoomPercent(); }
void DesktopAppController::setUiZoomPercent(int percent)
{ global_DBObjects.setUiZoomPercent(percent); }

QVariantMap DesktopAppController::windowGeometry() const
{
    QVariantMap result;

    const QString w = global_DBObjects.loadLocalParameter("UI:QMLMainWindow:Width");
    const QString h = global_DBObjects.loadLocalParameter("UI:QMLMainWindow:Height");
    if (w.isEmpty() || h.isEmpty())
        return result;   // nothing saved yet — caller keeps its built-in defaults

    const int x = global_DBObjects.loadLocalParameter("UI:QMLMainWindow:X").toInt();
    const int y = global_DBObjects.loadLocalParameter("UI:QMLMainWindow:Y").toInt();
    const int width = w.toInt();
    const int height = h.toInt();

    // Only restore the saved position if it still lands on a connected screen;
    // otherwise fall back to Qt's default placement (e.g. an external monitor
    // the window was last on has since been unplugged). Size is kept either way.
    const QRect target(x, y, width, height);
    bool onScreen = false;
    for (QScreen* screen : QGuiApplication::screens()) {
        if (screen->availableGeometry().intersects(target)) {
            onScreen = true;
            break;
        }
    }

    result["x"]         = onScreen ? x : -1;
    result["y"]         = onScreen ? y : -1;
    result["width"]     = width;
    result["height"]    = height;
    result["maximized"] = global_DBObjects.loadLocalParameter("UI:QMLMainWindow:Maximized") == "1";
    result["valid"]     = true;
    return result;
}

void DesktopAppController::saveWindowGeometry(int x, int y, int width, int height, bool maximized)
{
    global_DBObjects.saveLocalParameter("UI:QMLMainWindow:X", QString::number(x));
    global_DBObjects.saveLocalParameter("UI:QMLMainWindow:Y", QString::number(y));
    global_DBObjects.saveLocalParameter("UI:QMLMainWindow:Width", QString::number(width));
    global_DBObjects.saveLocalParameter("UI:QMLMainWindow:Height", QString::number(height));
    global_DBObjects.saveLocalParameter("UI:QMLMainWindow:Maximized", maximized ? "1" : "0");
}

// ── View options ─────────────────────────────────────────────────────────────

// setGlobalSearches() applies the new filters to the shared models but only
// marks them dirty — the Widgets app re-opens its current page afterwards,
// which is what re-queries them. QML pages bind straight to the models, so a
// list already on screen would keep its stale rows until the user navigated
// away and back. Re-query the models that have been loaded; the rest stay
// dirty and pick the filters up when a page first loads them.
static void refreshLoadedModels(std::initializer_list<SqlQueryModel*> models)
{
    for (SqlQueryModel* m : models)
        if (m && m->isLoaded())
            m->refresh();
}

bool DesktopAppController::showClosedProjects() const
{ return global_DBObjects.getShowClosedProjects(); }
void DesktopAppController::setShowClosedProjects(bool v)
{
    global_DBObjects.setShowClosedProjects(v);
    global_DBObjects.setGlobalSearches(true);
    // Models whose project-status filter setGlobalSearches() just changed.
    refreshLoadedModels({ global_DBObjects.projectinformationmodel(),
                          global_DBObjects.allitemsmodel(),
                          global_DBObjects.searchresultsmodel() });
    emit viewOptionsChanged();
}

bool DesktopAppController::showInternalItems() const
{ return global_DBObjects.getShowInternalItems(); }
void DesktopAppController::setShowInternalItems(bool v)
{
    global_DBObjects.setShowInternalItems(v);
    global_DBObjects.setGlobalSearches(true);
    // Models whose internal_item filter setGlobalSearches() just changed.
    refreshLoadedModels({ global_DBObjects.projectnotesmodel(),
                          global_DBObjects.actionitemsdetailsmeetingsmodel(),
                          global_DBObjects.notesactionitemsmodel(),
                          global_DBObjects.actionitemprojectnotesmodel(),
                          global_DBObjects.trackeritemsmodel(),
                          global_DBObjects.allitemsmodel(),
                          global_DBObjects.actionitemsdetailsmodel(),
                          global_DBObjects.searchresultsmodel() });
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

QString DesktopAppController::getQuickSearch(QAbstractItemModel* model) const
{
    if (auto* proxy = qobject_cast<SortFilterProxyModel*>(model))
        return proxy->quickSearch();
    return {};
}

// ── Clipboard ────────────────────────────────────────────────────────────────

QString DesktopAppController::clipboardPlainText() const
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    return clipboard ? clipboard->text() : QString();
}

void DesktopAppController::copyTextToClipboard(const QString& text) const
{
    if (QClipboard* clipboard = QGuiApplication::clipboard())
        clipboard->setText(text);
}

// ── Platform font metrics ────────────────────────────────────────────────────

// A QFont that carries a point size has no pixel size of its own (pixelSize()
// returns -1) until it is resolved against the paint device — QFontInfo does
// that resolution, which is what makes this comparable with the logical-pixel
// sizes QML's font.pixelSize expects.
static int fontPixelSize(const QFont& font)
{
    const int px = QFontInfo(font).pixelSize();
    return px > 0 ? px : QFontInfo(QGuiApplication::font()).pixelSize();
}

int DesktopAppController::menuFontPixelSize() const
{
    // QApplication (not QGuiApplication) is what maps a widget class name onto
    // the platform theme's matching font — "QMenu" resolves to QPlatformTheme::
    // MenuFont. The app object is a QApplication (see main.cpp), so this is
    // available; on a platform with no distinct menu font it hands back the
    // application font, which is the fallback we want anyway.
    return fontPixelSize(QApplication::font("QMenu"));
}

// ── Keyboard shortcuts ────────────────────────────────────────────────────────

QString DesktopAppController::nativeShortcutText(const QString& portableSequence) const
{
    if (portableSequence.isEmpty())
        return {};
    return QKeySequence(portableSequence).toString(QKeySequence::NativeText);
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

QVariantList DesktopAppController::columnDistinctValues(QAbstractItemModel* model,
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

    // Foreign-key columns (e.g. client_id) store the id as the filterable
    // value, but the Widgets dialog shows the id resolved through its lookup
    // table's delegate (e.g. client_name) — reproduce that here explicitly
    // since QML has no per-column paint delegate to fall back on.
    const QString lookupTable  = src->getLookupTable(col);
    const QString lookupFkCol  = src->getLookupFkColumnName(col);
    const QString lookupValCol = src->getLookupValueColumnName(col);

    QVariantList values;
    QSet<QString> seen;
    QSqlQuery q(src->getDBOs()->getDb());
    if (q.exec(sql)) {
        while (q.next() && values.size() < 500) {
            QVariant raw = q.value(0);
            src->reformatValue(raw, type);   // match the grid's display formatting
            const QString v = raw.toString().trimmed();
            if (v.isEmpty() || seen.contains(v)) continue;
            seen.insert(v);

            QString label = v;
            if (!lookupTable.isEmpty()) {
                QSqlQuery lq(src->getDBOs()->getDb());
                lq.prepare(QString("SELECT %1 FROM %2 WHERE %3 = ?")
                               .arg(lookupValCol, lookupTable, lookupFkCol));
                lq.addBindValue(v);
                if (lq.exec() && lq.next())
                    label = lq.value(0).toString();
            }

            QVariantMap m;
            m["value"] = v;
            m["label"] = label;
            values.append(m);
        }
    } else {
        qWarning() << "columnDistinctValues query failed:" << q.lastError().text() << sql;
    }

    std::sort(values.begin(), values.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("label").toString().compare(
                   b.toMap().value("label").toString(), Qt::CaseInsensitive) < 0;
    });
    return values;
}

// Scope string used to key a model's persisted UI state (column filters, sort
// order) in application_settings. Ordinarily just the table name — but
// global_DBObjects.trackeritemsmodel() (a project's own Tracker Items tab) and
// .allitemsmodel() (the master cross-project Items list) are two separate
// TrackerItemsModel instances that both report tablename() == "item_tracker".
// Keying purely off tablename() would make filtering/sorting one silently
// overwrite the other's persisted state. Disambiguate the project-scoped one.
static QString settingsScopeForModel(SqlQueryModel* src)
{
    QString scope = src->tablename();
    if (src == global_DBObjects.trackeritemsmodel())
        scope += ":project";
    return scope;
}

// application_settings key each model's column-filter spec is persisted under,
// so the Filter Editor's selections survive an app restart.
static QString columnFilterSettingKey(SqlQueryModel* src)
{
    return "UI:ColumnFilters:" + settingsScopeForModel(src);
}

static void applyFilterSpecsToModel(SqlQueryModel* src, const QVariantList& specs)
{
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
}

void DesktopAppController::applyColumnFilters(QAbstractItemModel* model,
                                              const QVariantList& specs)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return;

    applyFilterSpecsToModel(src, specs);
    src->activateUserFilter(QString());   // empty name → skip SqlQueryModel's own (QSettings-backed) persistence; we persist to application_settings below instead

    const QByteArray json = QJsonDocument(QJsonArray::fromVariantList(specs)).toJson(QJsonDocument::Compact);
    global_DBObjects.saveParameter(columnFilterSettingKey(src), QString::fromUtf8(json));

    ++m_filterRev;
    emit filterRevChanged();
}

void DesktopAppController::clearColumnFilters(QAbstractItemModel* model)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return;
    src->clearAllUserSearches();
    src->deactivateUserFilter(QString());
    global_DBObjects.saveParameter(columnFilterSettingKey(src), QString());

    ++m_filterRev;
    emit filterRevChanged();
}

QVariantList DesktopAppController::activeColumnFilters(QAbstractItemModel* model)
{
    QVariantList specs;
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return specs;

    const int cols = src->columnCount();
    for (int c = 0; c < cols; ++c) {
        if (!src->hasUserFilters(c)) continue;

        QVariantMap spec;
        spec["field"] = src->getColumnName(c);
        spec["values"] = src->getUserFilter(c);
        const QVariant search = src->getUserSearchString(c);
        spec["search"] = search.isValid() ? search.toString() : QString();

        QVariant start, end;
        src->getUserSearchRange(c, start, end);
        spec["rangeStart"] = start.isValid() ? start.toString() : QString();
        spec["rangeEnd"]   = end.isValid()   ? end.toString()   : QString();

        specs.append(spec);
    }
    return specs;
}

void DesktopAppController::applyQuickFilter(QAbstractItemModel* model, const QString& field,
                                            const QVariantList& values)
{
    QVariantList specs = activeColumnFilters(model);

    // Remove whatever's currently set for this field (if any) — either to
    // replace it below, or, if it already matched exactly, to leave it
    // removed (toggle-off: clicking an already-active Quick Filter again
    // clears just that one).
    bool wasSameValues = false;
    for (int i = specs.size() - 1; i >= 0; --i) {
        const QVariantMap spec = specs.at(i).toMap();
        if (spec.value("field").toString() != field) continue;
        if (spec.value("values").toList() == values)
            wasSameValues = true;
        specs.removeAt(i);
    }

    if (!wasSameValues) {
        QVariantMap spec;
        spec["field"] = field;
        spec["values"] = values;
        spec["search"] = QString();
        spec["rangeStart"] = QString();
        spec["rangeEnd"] = QString();
        specs.append(spec);
    }

    applyColumnFilters(model, specs);   // also bumps filterRev + persists
}

bool DesktopAppController::hasActiveColumnFilters(QAbstractItemModel* model) const
{
    SqlQueryModel* src = sourceModelOf(model);
    return src && src->hasUserFilters();
}

QAbstractItemModel* DesktopAppController::modelForSection(const QString& section) const
{
    if (section == "projects")     return projectsListModel();
    if (section == "items")        return allItemsModel();
    if (section == "people")       return peopleModel();
    if (section == "clients")      return clientsModel();
    if (section == "statusreport") return statusReportItemsModel();
    if (section == "trackeritems") return projectTrackerItemsModel();
    if (section == "team")         return projectTeamMembersModel();
    if (section == "locations")    return projectLocationsModel();
    if (section == "notes")        return projectNotesModel();
    return nullptr;
}

// ── Sort ─────────────────────────────────────────────────────────────────────

QVariantList DesktopAppController::sortColumns(QAbstractItemModel* model) const
{
    QVariantList out;
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return out;

    const int cols = src->columnCount();
    for (int c = 1; c < cols; ++c) {          // column 0 is the hidden id
        if (src->getType(c) == SqlQueryModel::DBHtml) continue;   // long free text
        QVariantMap m;
        m["field"] = src->getColumnName(c);
        m["label"] = src->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
        out.append(m);
    }
    return out;
}

// application_settings key a model's sort order is persisted under — same
// scope resolver the column filters use (see settingsScopeForModel above),
// so the project-tracker/master-items key collision fix covers sort too.
static QString sortSettingKey(SqlQueryModel* src)
{
    return "UI:SortOrder:" + settingsScopeForModel(src);
}

void DesktopAppController::applySort(QAbstractItemModel* model, const QString& field, bool descending)
{
    auto* proxy = qobject_cast<SortFilterProxyModel*>(model);
    SqlQueryModel* src = sourceModelOf(model);
    if (!proxy || !src) return;

    const int col = src->getColumnNumber(field);
    if (col < 0) return;

    proxy->sort(col, descending ? Qt::DescendingOrder : Qt::AscendingOrder);

    QVariantMap spec;
    spec["field"] = field;
    spec["descending"] = descending;
    const QByteArray json = QJsonDocument(QJsonObject::fromVariantMap(spec)).toJson(QJsonDocument::Compact);
    global_DBObjects.saveParameter(sortSettingKey(src), QString::fromUtf8(json));

    ++m_sortRev;
    emit sortRevChanged();
}

void DesktopAppController::clearSort(QAbstractItemModel* model)
{
    auto* proxy = qobject_cast<SortFilterProxyModel*>(model);
    SqlQueryModel* src = sourceModelOf(model);
    if (!proxy || !src) return;

    proxy->sort(-1);   // restores the model's base ORDER BY
    global_DBObjects.saveParameter(sortSettingKey(src), QString());

    ++m_sortRev;
    emit sortRevChanged();
}

QVariantMap DesktopAppController::activeSort(QAbstractItemModel* model) const
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return {};
    const QString json = global_DBObjects.loadParameter(sortSettingKey(src));
    if (json.isEmpty()) return {};
    return QJsonDocument::fromJson(json.toUtf8()).object().toVariantMap();
}

// Reapply a model's persisted sort (if any) after it's been created. Unlike
// restoreColumnFilters, sort()ing an unloaded model is cheap — it just sets
// the pending column/order, which the proxy applies when it next builds its
// row mapping — so every filterable section is restored here, not only the
// four models restoreColumnFilters forces an early query for.
static void restoreSort(QAbstractItemModel* model)
{
    auto* proxy = qobject_cast<SortFilterProxyModel*>(model);
    SqlQueryModel* src = sourceModelOf(model);
    if (!proxy || !src) return;

    const QString json = global_DBObjects.loadParameter(sortSettingKey(src));
    if (json.isEmpty()) return;

    const QVariantMap spec = QJsonDocument::fromJson(json.toUtf8()).object().toVariantMap();
    const int col = src->getColumnNumber(spec.value("field").toString());
    if (col < 0) return;

    proxy->sort(col, spec.value("descending").toBool() ? Qt::DescendingOrder : Qt::AscendingOrder);
}

// Reapply a model's persisted column filters (if any) after it's been created.
// Called once per filterable model right after the database opens, so grids
// stay filtered across an app restart the same way the Filter Editor left them.
static void restoreColumnFilters(SqlQueryModel* src)
{
    const QString json = global_DBObjects.loadParameter(columnFilterSettingKey(src));
    if (json.isEmpty())
        return;

    const QVariantList specs = QJsonDocument::fromJson(json.toUtf8()).array().toVariantList();
    if (specs.isEmpty())
        return;

    applyFilterSpecsToModel(src, specs);
    src->activateUserFilter(QString());
}

void DesktopAppController::refreshModel(QAbstractItemModel* model)
{
    if (SqlQueryModel* src = sourceModelOf(model))
        src->refresh();
}

// Special folderProjects() key for "projects that belong to no folder" — the
// sidebar's "Not Categorized" group (ProjectSidebar.qml). Mirrors the ""
// (All Projects) convention below; not a real FolderManager folder id.
static const QString kUncategorizedFolderKey = QStringLiteral("__uncategorized__");

QVariantList DesktopAppController::folderProjects(const QString& folderId)
{
    if (!m_folderSnapshotValid)
        rebuildFolderSnapshot();
    return m_folderSnapshot.value(folderId);
}

void DesktopAppController::rebuildFolderSnapshot()
{
    m_folderSnapshot.clear();

    // The sidebar builds during the QML load, BEFORE Main.qml's
    // Component.onCompleted opens the database — the proxy doesn't exist yet.
    // Don't cache that empty result as valid, or the sidebar stays blank until
    // the next invalidation (which used to be the startup sync completing,
    // minutes later — or never, with sync off).
    auto* proxy = global_DBObjects.projectinformationmodelproxy();
    if (!proxy)
        return;
    m_folderSnapshotValid = true;

    FolderManager* fm = FolderManager::instance();
    if (fm && !m_folderMgrConnected) {
        // FolderManager is a QML singleton created after openDatabase(), so the
        // membership hookup happens lazily on the first snapshot build.
        connect(fm, &FolderManager::foldersChanged,
                this, &DesktopAppController::invalidateFolderSnapshot);
        m_folderMgrConnected = true;
    }

    // One pass over the proxy's currently-visible rows (which already reflect
    // the active quick-search and column filters). Each row lands in the ""
    // (All Projects) list plus every folder it's a member of.
    const int rows = proxy->rowCount();
    for (int r = 0; r < rows; ++r) {
        const QString pid = proxy->data(proxy->index(r, 0)).toString();
        // A row the New Project button only staged has no id and isn't a project
        // yet — keep it out of the sidebar (and its counts) until it's written.
        if (pid.isEmpty())
            continue;

        QVariantMap row;
        row.insert(QStringLiteral("id"),             pid);
        row.insert(QStringLiteral("project_number"), proxy->data(proxy->index(r, 1)).toString());
        row.insert(QStringLiteral("project_name"),   proxy->data(proxy->index(r, 2)).toString());
        row.insert(QStringLiteral("project_status"), proxy->data(proxy->index(r, 14)).toString());

        m_folderSnapshot[QString()].append(row);
        if (fm) {
            const QStringList memberOf = fm->foldersForProject(pid);
            for (const QString& fid : memberOf)
                m_folderSnapshot[fid].append(row);
            if (memberOf.isEmpty())
                m_folderSnapshot[kUncategorizedFolderKey].append(row);
        }
    }
}

void DesktopAppController::invalidateFolderSnapshot()
{
    m_folderSnapshotValid = false;
    if (m_sidebarRevPending)
        return;
    // Coalesce bursts (a model reset fires rowsRemoved+rowsInserted+reset, a
    // sync cycle touches many rows) into one rev bump per event-loop turn.
    m_sidebarRevPending = true;
    QTimer::singleShot(0, this, [this]() {
        m_sidebarRevPending = false;
        ++m_sidebarRev;
        emit sidebarRevChanged();
    });
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
    SqlQueryModel* src = sourceModelOf(model);
    return pluginMenusForTable(src ? src->tablename() : QString());
}

QVariantList DesktopAppController::pluginMenusForTable(const QString& table)
{
    QVariantList out;
    m_pluginMenuCache.clear();
    if (!m_pluginManager || table.isEmpty())
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

QString DesktopAppController::tableNameForModel(QAbstractItemModel* model)
{
    SqlQueryModel* src = sourceModelOf(model);
    return src ? src->tablename() : QString();
}

void DesktopAppController::runPluginMenu(QAbstractItemModel* model,
                                         const QString& recordId, int index)
{
    SqlQueryModel* src = sourceModelOf(model);
    runPluginMenuForTable(src ? src->tablename() : QString(), recordId, index);
}

void DesktopAppController::runPluginMenuForTable(const QString& table,
                                                 const QString& recordId, int index)
{
    if (index < 0 || index >= m_pluginMenuCache.size())
        return;
    const PluginMenuRef& ref = m_pluginMenuCache.at(index);
    if (!ref.plugin || table.isEmpty())
        return;

    // Export just the selected record (col 0 = id), scoped by the menu's
    // tablefilter, then hand the XML to the plugin — mirrors slotPluginMenu.
    SqlQueryModel* exportModel = global_DBObjects.createExportObject(table);
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

QVariantList DesktopAppController::globalPluginMenus()
{
    QVariantList out;
    m_globalPluginMenuCache.clear();
    if (!m_pluginManager)
        return out;

    // A menu is "global" (Plugins > Settings / Utilities / etc. in the Widgets
    // menu bar) when its dataexport is empty — same rule buildPluginMenu() uses
    // to skip record menus, inverted.
    for (Plugin* p : m_pluginManager->plugins()) {
        if (!p || !p->loaded())
            continue;
        for (const PluginMenu& m : p->pythonplugin().menus()) {
            if (!m.dataexport().isEmpty())
                continue;

            m_globalPluginMenuCache.append({ p, m.functionname(), QString(), m.parameter() });

            QVariantMap entry;
            entry["title"]   = m.menutitle();
            entry["submenu"] = m.submenu();
            entry["index"]   = m_globalPluginMenuCache.size() - 1;
            out.append(entry);
        }
    }
    return out;
}

void DesktopAppController::runGlobalPluginMenu(int index)
{
    if (index < 0 || index >= m_globalPluginMenuCache.size())
        return;
    const PluginMenuRef& ref = m_globalPluginMenuCache.at(index);
    if (!ref.plugin)
        return;

    ref.plugin->callMethod(ref.functionname, ref.parameter);
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

void DesktopAppController::ensureAllItemsLoaded()
{
    // Page-activation path (vs. the context menu's explicit refreshAllItems):
    // load lazily on first visit, and afterwards only re-query when a write
    // marked the model dirty — a clean revisit keeps the cached rows, so no
    // model reset and no delegate rebuild.
    auto* model = global_DBObjects.allitemsmodel();
    if (model->rowCount(QModelIndex()) == 0)
        model->refresh();
    else
        model->refreshIfDirty();
}
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

// The *ForId lookups below use the source model's hash-backed findValue()/
// findIndex() (O(1) after the first call per column) instead of scanning the
// proxy row-by-row — QML list delegates call these once per visible row, and
// the old linear scans made opening a large list O(rows × lookup-table rows).
// Row results are mapped back to proxy rows, which is what callers index.
// Resolving on the source also means an active quick-search filter can no
// longer hide a name from an unrelated list's lookup.

int DesktopAppController::projectRowForId(const QString& projectId) const
{
    auto* proxy = global_DBObjects.projectinformationmodelproxy();
    auto* src   = global_DBObjects.projectinformationmodel();
    if (!proxy || !src || projectId.isEmpty()) return -1;
    QVariant key(projectId);
    const QModelIndex srcIdx = src->findIndex(key, 0);
    if (!srcIdx.isValid()) return -1;
    const QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
    return proxyIdx.isValid() ? proxyIdx.row() : -1;
}

QString DesktopAppController::projectNumberForId(const QString& projectId) const
{
    auto* src = global_DBObjects.projectinformationmodel();
    if (!src || projectId.isEmpty()) return {};
    QVariant key(projectId);
    return src->findValue(key, 0, 1).toString();  // col 1 = project_number
}

QString DesktopAppController::projectNameForId(const QString& projectId) const
{
    auto* src = global_DBObjects.projectinformationmodel();
    if (!src || projectId.isEmpty()) return {};
    QVariant key(projectId);
    return src->findValue(key, 0, 2).toString();  // col 2 = project_name
}

QString DesktopAppController::clientNameForId(const QString& clientId) const
{
    auto* src = global_DBObjects.clientsmodel();
    if (!src || clientId.isEmpty()) return {};
    QVariant key(clientId);
    return src->findValue(key, 0, 1).toString();  // col 1 = client_name
}

int DesktopAppController::clientRowForId(const QString& clientId) const
{
    auto* proxy = global_DBObjects.clientsmodelproxy();
    auto* src   = global_DBObjects.clientsmodel();
    if (!proxy || !src || clientId.isEmpty()) return -1;
    QVariant key(clientId);
    const QModelIndex srcIdx = src->findIndex(key, 0);
    if (!srcIdx.isValid()) return -1;
    const QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
    return proxyIdx.isValid() ? proxyIdx.row() : -1;
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
    auto* src   = global_DBObjects.peoplemodel();
    if (!proxy || !src || peopleId.isEmpty()) return -1;
    QVariant key(peopleId);
    const QModelIndex srcIdx = src->findIndex(key, 0);
    if (!srcIdx.isValid()) return -1;
    const QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
    return proxyIdx.isValid() ? proxyIdx.row() : -1;
}

QString DesktopAppController::peopleIdAtRow(int row) const
{
    auto* proxy = global_DBObjects.peoplemodelproxy();
    if (!proxy || row < 0 || row >= proxy->rowCount()) return {};
    return proxy->data(proxy->index(row, 0)).toString();
}

QString DesktopAppController::peopleNameForId(const QString& personId) const
{
    auto* src = global_DBObjects.peoplemodel();
    if (!src || personId.isEmpty()) return {};
    QVariant key(personId);
    return src->findValue(key, 0, 1).toString();  // col 1 = name
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

QVariantList DesktopAppController::projectList() const
{
    QVariantList out;
    auto* proxy = global_DBObjects.projectinformationmodelproxy();
    if (!proxy) return out;
    for (int row = 0; row < proxy->rowCount(); ++row) {
        const QVariantMap m = proxyRowToMap(proxy, row);
        QVariantMap e;
        e.insert("id",   m.value("id").toString());
        e.insert("name", (m.value("project_number").toString() + "  "
                           + m.value("project_name").toString()).trimmed());
        out.append(e);
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

QVariantList DesktopAppController::notesForProject(const QString& projectId) const
{
    QVariantList out;
    if (projectId.isEmpty()) return out;

    DB_LOCK;
    QSqlQuery qry(global_DBObjects.getDb());
    qry.prepare("SELECT id, note_title, "
                "strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) "
                "FROM project_notes "
                "WHERE project_id = ? AND (deleted IS NULL OR deleted = 0) "
                "ORDER BY note_date DESC");
    qry.addBindValue(projectId);
    qry.exec();
    while (qry.next()) {
        QVariantMap m;
        m.insert("id",    qry.value(0).toString());
        m.insert("title", qry.value(1).toString());
        m.insert("date",  qry.value(2).toString());
        out.append(m);
    }
    DB_UNLOCK;
    return out;
}

QVariantList DesktopAppController::recentCommentsForItem(const QString& itemId, int limit) const
{
    QVariantList out;
    if (itemId.isEmpty() || limit <= 0) return out;

    DB_LOCK;
    QSqlQuery qry(global_DBObjects.getDb());
    // lastupdated_date is normally an epoch int, but TrackerItemCommentsModel::
    // setData() re-stamps it as a "MM/dd/yyyy" string on every edit — COALESCE
    // falls back to the raw stored value when the epoch conversion comes back
    // NULL, so an edited comment's date still displays instead of going blank.
    qry.prepare("SELECT update_note, updated_by, "
                "COALESCE(strftime('%m/%d/%Y', datetime(lastupdated_date, 'unixepoch')), "
                "         lastupdated_date) "
                "FROM item_tracker_updates "
                "WHERE item_id = ? AND (deleted IS NULL OR deleted = 0) "
                "ORDER BY lastupdated_date DESC LIMIT ?");
    qry.addBindValue(itemId);
    qry.addBindValue(limit);
    qry.exec();
    while (qry.next()) {
        QVariantMap m;
        m.insert("note", qry.value(0).toString());
        // Safe to call while DB_LOCK is held: peopleNameForId() reads from the
        // people model's in-memory cache, it doesn't run its own query.
        m.insert("by",   peopleNameForId(qry.value(1).toString()));
        m.insert("date", qry.value(2).toString());
        out.append(m);
    }
    DB_UNLOCK;
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

// Stage a new project row in the model cache without writing it. It is INSERTed
// by saveProject() once the detail page has the two fields the schema insists on
// (project_number/project_name are NOT NULL *and* carry partial unique indexes,
// so a blank row cannot be written and a second blank row could not be unique).
// Same shape as addPerson()/addClient(); Main.qml discards the staged row again
// if the page is left before it becomes a real record.
int DesktopAppController::addProject()
{
    return proxyRowFromSource(global_DBObjects.projectinformationmodelproxy(),
                              global_DBObjects.projectinformationmodel()->newRecord());
}

QString DesktopAppController::nextProjectNumber() const
{
    return global_DBObjects.projectinformationmodel()->nextAvailableProjectNumber();
}

bool DesktopAppController::discardNewProject(int row)
{
    auto* proxy = global_DBObjects.projectinformationmodelproxy();
    auto* src   = global_DBObjects.projectinformationmodel();
    if (!proxy || row < 0 || row >= proxy->rowCount()) return false;

    const QModelIndex srcIdx = proxy->mapToSource(proxy->index(row, 0));
    // Only ever drops a row that was never written (no id) — a saved project has
    // to go through deleteProject() and its reference checks.
    if (!srcIdx.isValid() || !src->isNewRecord(srcIdx)) return false;

    src->removeCacheRecord(srcIdx);
    return true;
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

bool DesktopAppController::isProjectTeamMember(const QString& projectId, const QString& peopleId) const
{
    if (projectId.isEmpty() || peopleId.isEmpty()) return true;
    DB_LOCK;
    QSqlQuery q(global_DBObjects.getDb());
    q.prepare("SELECT count(*) FROM project_people "
              "WHERE project_id = ? AND people_id = ? AND deleted = 0");
    q.addBindValue(projectId);
    q.addBindValue(peopleId);
    q.exec();
    const bool exists = q.next() && q.value(0).toInt() > 0;
    DB_UNLOCK;
    return exists;
}

bool DesktopAppController::addPersonToProjectTeam(const QString& projectId, const QString& peopleId)
{
    if (projectId.isEmpty() || peopleId.isEmpty()) return true;
    if (isProjectTeamMember(projectId, peopleId)) return true;

    // Mirrors addTeamMember()+saveTeamMember() (below), but writes straight
    // against the *source* projectteammembersmodel rather than through
    // projectteammembersmodelproxy(): that proxy is filtered to whichever
    // project happens to be open on screen (setProjectFilter), which may not
    // be `projectId` here, and addTeamMember/saveTeamMember's row argument
    // would then resolve against the wrong proxy row.
    auto* src = global_DBObjects.projectteammembersmodel();
    QVariant fk(projectId);
    const QModelIndex srcIdx = src->newRecord(&fk);
    if (!srcIdx.isValid()) return false;

    // people_id (col 2) first: it is NOT NULL / carries the unique key, so it
    // is the write that can be rejected, mirroring saveTeamMember's ordering.
    return applyRowFields(src, srcIdx.row(), { {2, peopleId}, {4, "0"}, {5, ""} });
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
    auto* proxy = global_DBObjects.projectinformationmodelproxy();
    if (row < 0 || row >= proxy->rowCount()) return false;

    const QPersistentModelIndex pIdx(proxy->index(row, 0));
    if (!pIdx.isValid()) return false;

    // A row addProject() only staged still has no id, and cannot be written one
    // column at a time: the first setData() would try to INSERT it while the
    // other required column is still null. Write the whole row at once instead.
    const QModelIndex srcIdx = proxy->mapToSource(proxy->index(row, 0));
    if (srcIdx.isValid() && global_DBObjects.projectinformationmodel()->isNewRecord(srcIdx))
        return insertStagedProject(srcIdx.row(),
                { { 1, projectNumber},   { 2, projectName},        { 3, lastStatusDate},
                  { 4, lastInvoiceDate}, { 5, primaryContactId},   { 6, budget},
                  { 7, actual},          { 8, bcwp},               { 9, bcws},
                  {10, bac},             {11, invoicingPeriod},    {12, statusReportPeriod},
                  {13, clientId},        {14, projectStatus} });

    if (!applyRowFields(proxy, pIdx.row(), {
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
        global_DBObjects.projectinformationmodel()->reloadRecord(
            proxy->mapToSource(proxy->index(pIdx.row(), 0)));
    }
    return true;
}

// Write a staged project row (see addProject) as one INSERT, then finish what
// ProjectsModel::setData() would have done for a row it inserted itself: give
// the project its default project manager.
bool DesktopAppController::insertStagedProject(int srcRow,
        const QVector<QPair<int, QVariant>>& fields)
{
    auto* src = global_DBObjects.projectinformationmodel();
    m_lastCreatedProjectId.clear();

    if (!src->insertStagedRow(srcRow, fields)) {
        QString err = global_DBObjects.lastSaveError();
        if (err.isEmpty())
            err = tr("The project could not be saved.");
        emit errorOccurred(tr("Could Not Save"), err);
        return false;
    }

    const QString newId = src->data(src->index(srcRow, 0)).toString();
    if (!newId.isEmpty())
        global_DBObjects.addDefaultPMToProject(newId);
    m_lastCreatedProjectId = newId;

    // Pick up the computed EVM columns, as the update path above does.
    src->reloadRecord(src->index(srcRow, 0));
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

    // newRecord()/insertCacheRow() just appends to the cache — it doesn't honor
    // the model's "note_date desc" base order. The list is always newest-first,
    // so re-run the query and relocate the new (today-dated) note to find where
    // that puts it, instead of leaving it wherever it landed in the cache.
    src->refresh();
    QVariant idLookup(newId);
    return proxyRowFromSource(global_DBObjects.projectnotesmodelproxy(), src->findIndex(idLookup, 0));
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

// Duplicate a note — Widgets parity (ProjectNotesModel::copyRecord): keeps the
// project and title, resets the date to now, leaves the note body blank, and
// copies the attendee list. Action items are intentionally NOT copied. The
// notes model is already filtered to the open project (see openProject), so
// the copy just needs to find the source row for noteId.
int DesktopAppController::copyProjectNote(const QString& noteId)
{
    global_DBObjects.setLastSaveError("");
    if (noteId.isEmpty())
        return -1;

    auto* src = global_DBObjects.projectnotesmodel();
    if (!src) return -1;

    QVariant key(noteId);
    const QModelIndex srcIdx = src->findIndex(key, 0);
    if (!srcIdx.isValid()) return -1;

    const QModelIndex newIdx = src->copyRecord(srcIdx);
    if (!newIdx.isValid()) return -1;

    // Same reasoning as addProjectNote(): the copy is appended to the cache
    // with today's date, so re-run the query to put it in its newest-first
    // spot rather than leaving it at the append position.
    const QString newId = src->data(src->index(newIdx.row(), 0)).toString();
    src->refresh();
    QVariant idLookup(newId);
    return proxyRowFromSource(global_DBObjects.projectnotesmodelproxy(), src->findIndex(idLookup, 0));
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

int DesktopAppController::addPeopleFromVCardDrop(const QStringList& fileUrls, const QString& text)
{
    const QList<VCardContact> contacts = parseVCards(combineVCardSources(fileUrls, text));
    if (contacts.isEmpty())
    {
        emit errorOccurred(tr("No Contacts Found"),
            tr("The dropped item didn't contain any recognizable vCard contacts."));
        return 0;
    }

    for (const VCardContact& contact : contacts)
    {
        const QString clientId = findOrCreateClient(&global_DBObjects, contact.company);
        findOrCreatePerson(&global_DBObjects, contact, clientId);
    }

    // findOrCreatePerson() writes through the unfiltered people model so the
    // insert can't be scoped out by whatever filter the visible People list
    // currently has active — refresh the model actually bound to peopleModel()
    // so the new rows show up immediately rather than waiting for the next
    // dirty-triggered reload.
    global_DBObjects.peoplemodel()->refresh();

    emit infoOccurred(tr("Contacts Added"), contacts.size() == 1
        ? tr("1 contact was added.")
        : tr("%1 contacts were added.").arg(contacts.size()));
    return contacts.size();
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

// ── Move a tracker item to a different project ───────────────────────────────

QVariantMap DesktopAppController::checkTrackerItemMove(const QString& itemId,
                                                        const QString& newProjectId) const
{
    QVariantMap result;
    result["valid"] = false;
    if (itemId.isEmpty() || newProjectId.isEmpty()) return result;

    // Same single-row load openTrackerItem() uses: filter the shared detail
    // model down to this one item (col 0 = id).
    auto* src = global_DBObjects.actionitemsdetailsmodel();
    src->setFilter(0, itemId);
    src->refresh();
    if (src->rowCount(QModelIndex()) == 0) return result;

    const QString currentProjectId = src->data(src->index(0, 14)).toString();
    if (currentProjectId == newProjectId) return result;  // dropped on its own project

    const QString currentNumber = src->data(src->index(0, 1)).toString();
    const QString assignedTo    = src->data(src->index(0, 7)).toString();
    const QString identifiedBy  = src->data(src->index(0, 4)).toString();
    const QString noteId        = src->data(src->index(0, 13)).toString();

    result["valid"]       = true;
    result["projectName"] = projectNameForId(newProjectId);
    result["oldNumber"]   = currentNumber;

    const bool numberFree = isItemNumberUnique(newProjectId, itemId, currentNumber);
    const QString newNumber = numberFree ? currentNumber : src->getNextItemNumber(newProjectId).toString();
    result["newNumber"]    = newNumber;
    result["willRenumber"] = !numberFree;

    QVariantList membersToAdd;
    for (const QString& personId : { identifiedBy, assignedTo }) {
        if (personId.isEmpty() || isProjectTeamMember(newProjectId, personId)) continue;
        bool already = false;
        for (const QVariant& m : membersToAdd)
            if (m.toMap().value("id").toString() == personId) { already = true; break; }
        if (already) continue;
        QVariantMap m;
        m["id"]   = personId;
        m["name"] = peopleNameForId(personId);
        membersToAdd.append(m);
    }
    result["membersToAdd"] = membersToAdd;

    result["willClearMeeting"] = false;
    if (!noteId.isEmpty()) {
        const QString title = global_DBObjects.execute(
            QString("SELECT note_title FROM project_notes WHERE id = '%1' AND deleted = 0")
                .arg(QString(noteId).replace("'", "''")));
        if (!title.isEmpty()) {
            result["willClearMeeting"] = true;
            result["meetingTitle"]     = title;
        }
    }

    return result;
}

bool DesktopAppController::moveTrackerItem(const QString& itemId, const QString& newProjectId,
                                            const QString& newNoteId)
{
    global_DBObjects.setLastSaveError("");
    if (itemId.isEmpty() || newProjectId.isEmpty()) return false;

    auto* src = global_DBObjects.actionitemsdetailsmodel();
    src->setFilter(0, itemId);
    src->refresh();
    if (src->rowCount(QModelIndex()) == 0) return false;

    const QString currentProjectId = src->data(src->index(0, 14)).toString();
    const QString currentNumber    = src->data(src->index(0, 1)).toString();
    const QString assignedTo       = src->data(src->index(0, 7)).toString();
    const QString identifiedBy     = src->data(src->index(0, 4)).toString();

    QAbstractItemModel* proxy = global_DBObjects.actionitemsdetailsmodelproxy();
    if (proxy->rowCount() == 0) return false;

    if (currentProjectId == newProjectId) {
        // Same project: only the meeting link may be changing (newNoteId is
        // caller-supplied — the Move To… dialog guarantees it belongs to this
        // project, or is empty for "no meeting").
        if (!applyRowFields(proxy, 0, { {13, newNoteId} })) return false;
    } else {
        const QString newNumber = isItemNumberUnique(newProjectId, itemId, currentNumber)
            ? currentNumber : src->getNextItemNumber(newProjectId).toString();

        // item_number + project_id form a composite unique key (see
        // addUniqueKeys() in trackeritemsmodel.cpp), but applyRowFields()
        // commits one setData() per field, and each setData() validates the
        // OTHER key column against the row's cached value at that moment —
        // not the value another field in this same batch is about to take.
        // Setting {item_number, project_id} directly can therefore hit a
        // false collision at the halfway point even though the starting pair
        // and the final pair are each genuinely unique: e.g. moving an item
        // out of project A (which happens to also hold a *different* item
        // already numbered newNumber) into project B — writing item_number
        // first collides against A's own row; writing project_id first
        // collides if B already holds this item's *old* number. Parking the
        // row under a disposable UUID item_number first guarantees every
        // intermediate state is unique, so project_id and the real newNumber
        // can then land safely in either order.
        const QString tempNumber = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!applyRowFields(proxy, 0, { {1, tempNumber} }))
            return false;
        if (!applyRowFields(proxy, 0, { {13, newNoteId}, {14, newProjectId} }))
            return false;
        if (!applyRowFields(proxy, 0, { {1, newNumber} }))
            return false;

        for (const QString& personId : { identifiedBy, assignedTo })
            if (!personId.isEmpty())
                addPersonToProjectTeam(newProjectId, personId);
        refreshTeamMembers();
    }

    global_DBObjects.actionitemsdetailsmodel()->refresh();
    global_DBObjects.notesactionitemsmodel()->refresh();
    global_DBObjects.trackeritemsmodel()->refresh();
    refreshAllItems();
    return true;
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

int DesktopAppController::addTeamMembersFromVCardDrop(const QString& projectId,
                                    const QStringList& fileUrls, const QString& text)
{
    if (projectId.isEmpty()) return 0;

    const QList<VCardContact> contacts = parseVCards(combineVCardSources(fileUrls, text));
    if (contacts.isEmpty())
    {
        emit errorOccurred(tr("No Contacts Found"),
            tr("The dropped item didn't contain any recognizable vCard contacts."));
        return 0;
    }

    for (const VCardContact& contact : contacts)
    {
        const QString clientId = findOrCreateClient(&global_DBObjects, contact.company);
        const QString personId = findOrCreatePerson(&global_DBObjects, contact, clientId);
        if (!personId.isEmpty())
            addPersonToProjectTeam(projectId, personId);
    }

    // Mirrors moveTrackerItem()'s membersToAdd handling: findOrCreatePerson()
    // writes through the unfiltered people model, and addPersonToProjectTeam()
    // writes straight against the source team-members model rather than
    // whatever project the visible proxy is currently filtered to — refresh
    // both so the drop's results show up immediately on screen.
    global_DBObjects.peoplemodel()->refresh();
    refreshTeamMembers();

    emit infoOccurred(tr("Contacts Added"), contacts.size() == 1
        ? tr("1 contact was added to the team.")
        : tr("%1 contacts were added to the team.").arg(contacts.size()));
    return contacts.size();
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

    // path (col 4) must be written BEFORE location_type (col 2): writing the path
    // makes ProjectLocationsModel::setData re-derive the file type from it (see
    // setProjectLocationPath above) and overwrite col 2 unconditionally, even when
    // the path is unchanged. If type were written first, that re-derivation would
    // immediately clobber the user's explicit type selection.
    return applyRowFields(model, pIdx.row(), {
        {4, path}, {2, locationType}, {3, description} });
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

// ── Duplicate a tracker item ──────────────────────────────────────────────────

QString DesktopAppController::copyTrackerItem(const QString& itemId)
{
    global_DBObjects.setLastSaveError("");
    if (itemId.isEmpty())
        return {};

    // Filter the detail model down to just this item (row 0 becomes the source),
    // then duplicate it. TrackerItemsModel::copyRecord renames the item to
    // "Copy of …", assigns the next item number in the project, gives it a fresh
    // id, and persists it (insertCacheRow) — same as the Widgets Copy Item action.
    openTrackerItem(itemId);
    auto* src = global_DBObjects.actionitemsdetailsmodel();
    if (!src || src->rowCount(QModelIndex()) < 1)
        return {};

    const QModelIndex newIdx = src->copyRecord(src->index(0, 0));
    if (!newIdx.isValid())
        return {};

    const QString newId = src->data(src->index(newIdx.row(), 0)).toString();

    // Refresh the lists that show items so the copy appears immediately: the
    // project Tracker tab, the master Items list, and a meeting's action items
    // (all three are separate models over item_tracker).
    global_DBObjects.trackeritemsmodel()->refresh();
    global_DBObjects.allitemsmodel()->refresh();
    global_DBObjects.notesactionitemsmodel()->refresh();

    return newId;
}

// ── Duplicate any record ──────────────────────────────────────────────────────

// The two join tables a straight copy can never succeed on: both carry a
// composite unique key of (parent, person), so the copy always clashes with the
// row it came from. The Widgets TableView context menu hides Copy for exactly
// these two (see TableView::contextMenuEvent).
static bool tableIsCopyable(const QString& table)
{
    const QString t = table.toLower();
    return !t.isEmpty()
        && t != QLatin1String("project_people")
        && t != QLatin1String("meeting_attendees");
}

// Re-query every live model that shows |table|, so a record copied through a
// throwaway export model appears in the on-screen lists straight away.
static void refreshModelsForTable(const QString& table)
{
    const QString t = table.toLower();
    if (t == QLatin1String("projects")) {
        if (auto* m = global_DBObjects.projectinformationmodel()) m->refresh();
    } else if (t == QLatin1String("people")) {
        if (auto* m = global_DBObjects.peoplemodel()) m->refresh();
    } else if (t == QLatin1String("clients")) {
        if (auto* m = global_DBObjects.clientsmodel()) m->refresh();
    } else if (t == QLatin1String("item_tracker")) {
        // Three models show tracker items: the project tab, the all-items master
        // list, and the notes-page action items.
        if (auto* m = global_DBObjects.trackeritemsmodel()) m->refresh();
        if (auto* m = global_DBObjects.allitemsmodel()) m->refresh();
        if (auto* m = global_DBObjects.notesactionitemsmodel()) m->refresh();
    } else if (t == QLatin1String("project_notes")) {
        if (auto* m = global_DBObjects.projectnotesmodel()) m->refresh();
    } else if (t == QLatin1String("project_locations")) {
        if (auto* m = global_DBObjects.projectlocationsmodel()) m->refresh();
    } else if (t == QLatin1String("status_report_items")) {
        if (auto* m = global_DBObjects.statusreportitemsmodel()) m->refresh();
    } else if (t == QLatin1String("item_tracker_updates")) {
        if (auto* m = global_DBObjects.trackeritemscommentsmodel()) m->refresh();
    }
}

bool DesktopAppController::canDuplicateTable(const QString& table)
{
    if (!tableIsCopyable(table))
        return false;

    // Whether a table has a model to copy with is fixed for the run, but this is
    // asked once per context-menu open, so probe each table only the first time
    // rather than building and tearing down a model on every right-click.
    static QHash<QString, bool> cache;
    const QString key = table.toLower();
    const auto hit = cache.constFind(key);
    if (hit != cache.constEnd())
        return *hit;

    SqlQueryModel* probe = global_DBObjects.createExportObject(key);
    const bool ok = probe && !probe->isReadOnly();
    delete probe;
    cache.insert(key, ok);
    return ok;
}

bool DesktopAppController::canDuplicateModel(QAbstractItemModel* model)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src || src->isReadOnly())
        return false;
    return canDuplicateTable(src->tablename());
}

QString DesktopAppController::duplicateRecord(QAbstractItemModel* model, const QString& recordId)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src)
        return {};
    return duplicateRecordInTable(src->tablename(), recordId);
}

QString DesktopAppController::duplicateRecordInTable(const QString& table, const QString& recordId)
{
    global_DBObjects.setLastSaveError("");
    if (recordId.isEmpty() || !tableIsCopyable(table))
        return {};

    // Copy through a throwaway model of the table's own class, narrowed to the
    // one record (same recipe as runPluginMenuForTable). Going through the
    // table's canonical model rather than whichever list the row was clicked in
    // matters: only TrackerItemsModel knows how to renumber a copied tracker
    // item, yet items are also shown by NotesActionItemsModel, and a search hit
    // may not be loaded in any live model at all. It also keeps a failed copy's
    // half-built row out of the models the UI is bound to.
    SqlQueryModel* copyModel = global_DBObjects.createExportObject(table);
    if (!copyModel)
        return {};

    copyModel->setFilter(0, recordId);
    copyModel->refresh();

    QString newId;
    if (copyModel->rowCount(QModelIndex()) > 0) {
        const QModelIndex newIdx = copyModel->copyRecord(copyModel->index(0, 0));
        // copyRecord() returns the staged cache row even when the INSERT was
        // rejected; the id column is only filled in once the row is written, so
        // an empty id is the failure signal.
        if (newIdx.isValid())
            newId = copyModel->data(copyModel->index(newIdx.row(), 0)).toString();
    }

    delete copyModel;

    if (newId.isEmpty()) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Cannot Duplicate"),
                           err.isEmpty() ? tr("This record could not be copied.") : err);
        return {};
    }

    refreshModelsForTable(table);
    return newId;
}

// ── Help ▸ maintenance actions ────────────────────────────────────────────────

QString DesktopAppController::appVersion() const
{
    return QStringLiteral("%1.%2.%3")
        .arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH);
}

QString DesktopAppController::buildTimestamp() const
{
    // Same __DATE__ " " __TIME__ source as the Widgets AboutDialog's BUILDV;
    // evaluated here (rather than a file-scope const) so it reflects this
    // translation unit's compile time.
    return QStringLiteral(__DATE__ " " __TIME__);
}

QString DesktopAppController::qtRuntimeVersion() const
{
    return QString::fromLatin1(qVersion());
}

// Create the shared UpdateManager once and translate its signals into the
// controller's QML-facing signals. Reusing UpdateManager gives the QML app the
// same battle-tested unattended install/relaunch flow the Widgets app ships:
// a silent NSIS install (/waitpid + /relaunch) on Windows and a detached
// swap-and-relaunch helper on macOS. (Linux has no installer asset — it updates
// via Flatpak — so a check there reports "download manually".)
void DesktopAppController::ensureUpdater()
{
    if (m_updater)
        return;
    m_updater = new UpdateManager(nullptr);   // no QWidget parent needed
    // The QML shell renders its own themed dialogs, so suppress UpdateManager's
    // native QProgressDialog/QMessageBox and consume its signals instead.
    m_updater->setUseNativeUi(false);

    connect(m_updater, &UpdateManager::updateAvailable, this,
            [this](const QString& version, const QString& notes, const QUrl& assetUrl) {
                m_pendingAssetUrl = assetUrl.toString();
                emit updateAvailable(version, notes);
            });
    connect(m_updater, &UpdateManager::upToDate, this, [this]() {
        if (!m_silentCheck) emit upToDate(appVersion());
    });
    connect(m_updater, &UpdateManager::checkFailed, this, [this](const QString& err) {
        if (!m_silentCheck) emit updateCheckFailed(err);
    });
    connect(m_updater, &UpdateManager::downloadProgress, this,
            [this](qint64 received, qint64 total) {
                emit updateDownloadProgress(total > 0
                    ? int((received * 100) / total) : -1);
            });
    connect(m_updater, &UpdateManager::updateError, this, [this](const QString& msg) {
        emit updateInstallFailed(msg);
    });
    connect(m_updater, &UpdateManager::installerLaunched, this, [this]() {
        emit quitForUpdate();   // shell calls Qt.quit() so the installer can proceed
    });
}

void DesktopAppController::checkForUpdates()
{
    if (!s_updateChecksEnabled)
        return;
    m_silentCheck = false;
    ensureUpdater();
    m_updater->checkForUpdates(/*silent=*/false);
}

void DesktopAppController::checkForUpdatesSilent()
{
    if (!s_updateChecksEnabled)
        return;
    m_silentCheck = true;
    ensureUpdater();
    m_updater->checkForUpdates(/*silent=*/true);
}

void DesktopAppController::installUpdate()
{
    if (!m_updater || m_pendingAssetUrl.isEmpty()) {
        emit updateInstallFailed(tr("No update is available to install."));
        return;
    }
    emit updateDownloadStarted();
    m_updater->downloadAndInstall(QUrl(m_pendingAssetUrl));
}

void DesktopAppController::cancelUpdateDownload()
{
    if (m_updater)
        m_updater->cancelDownload();
}

void DesktopAppController::sendLogsToSupport()
{
    const QString supportEmail = QStringLiteral("admin@projectnotespro.com");
    const QString logDir = dataLocation() + "/logs";

    QDir dir(logDir);
    const QFileInfoList logFiles =
        dir.entryInfoList({ QStringLiteral("*.log") }, QDir::Files, QDir::Name);
    if (logFiles.isEmpty()) {
        emit infoOccurred(tr("Send Logs to Support"),
            tr("No log files were found to send.\n\nLog files are stored in:\n%1")
                .arg(QDir::toNativeSeparators(logDir)));
        return;
    }

    // Prefer the Desktop; fall back to the temp dir if it isn't writable.
    QString destDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (destDir.isEmpty() || !QFileInfo(destDir).isWritable())
        destDir = QDir::tempPath();

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString zipPath = QDir(destDir).filePath(
        QStringLiteral("ProjectNotes-logs-%1.zip").arg(stamp));

    QZipWriter zip(zipPath);
    if (zip.status() != QZipWriter::NoError) {
        emit errorOccurred(tr("Send Logs to Support"),
            tr("Could not create the log archive."));
        return;
    }
    zip.setCompressionPolicy(QZipWriter::AlwaysCompress);
    bool addedAny = false;
    for (const QFileInfo& fi : logFiles) {
        QFile in(fi.absoluteFilePath());
        if (!in.open(QIODevice::ReadOnly))
            continue;   // skip a locked/unreadable log rather than aborting
        zip.addFile(fi.fileName(), in.readAll());
        in.close();
        if (zip.status() == QZipWriter::NoError)
            addedAny = true;
    }
    zip.close();

    if (!addedAny || zip.status() != QZipWriter::NoError) {
        QLog_Error(SYNCERRORLOG, QString("Support bundle creation failed for %1").arg(zipPath));
        emit errorOccurred(tr("Send Logs to Support"),
            tr("Could not create the log archive."));
        return;
    }

    // mailto can't carry attachments, so the body gives the path to attach.
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("subject"),
        tr("ProjectNotes Support Logs (v%1)").arg(appVersion()));
    query.addQueryItem(QStringLiteral("body"),
        tr("Please describe the problem you are seeing below.\n\n"
           "IMPORTANT: attach the log archive located at:\n%1\n\n"
           "--- describe your issue here ---\n")
            .arg(QDir::toNativeSeparators(zipPath)));
    QUrl mailUrl;
    mailUrl.setScheme(QStringLiteral("mailto"));
    mailUrl.setPath(supportEmail);
    mailUrl.setQuery(query);
    QDesktopServices::openUrl(mailUrl);

    emit infoOccurred(tr("Send Logs to Support"),
        tr("A log archive was created here:\n\n%1\n\n"
           "An email to %2 has been started. Please attach that zip file to the "
           "email and send it.")
            .arg(QDir::toNativeSeparators(zipPath), supportEmail));
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

// Each setter is a no-op when the value is unchanged: the Settings screen
// commits every sync field whenever it loses focus (or the user navigates), so
// without the guard simply visiting the page would mark the settings unverified
// and trigger a pointless round trip to the host.
void DesktopAppController::setSyncEnabled(bool v)
{
    if (syncEnabled() == v) return;
    setSyncSetting("Sync/Enabled", v);
    // Switching sync on is the moment the stored credentials start to matter.
    if (v) setSyncSettingsUnverified(true);
    emit syncSettingsChanged();
}
void DesktopAppController::setSyncEmail(const QString& v)
{
    if (syncEmail() == v) return;
    setSyncSetting("Sync/Email", v);
    setSyncSettingsUnverified(true);
    emit syncSettingsChanged();
}
void DesktopAppController::setSyncPassword(const QString& v)
{
    if (syncPassword() == v) return;
    setSyncSetting("Sync/Password", v);
    setSyncSettingsUnverified(true);
    emit syncSettingsChanged();
}
void DesktopAppController::setSyncEncryptionPhrase(const QString& v)
{
    if (syncEncryptionPhrase() == v) return;
    setSyncSetting("Sync/EncryptionPhrase", v);
    setSyncSettingsUnverified(true);
    emit syncSettingsChanged();
}

QString DesktopAppController::supabaseUrl()
{
    return s_testSupabase ? QStringLiteral("https://lsulnvxgrlpuqtzonner.supabase.co")
                          : QStringLiteral("https://nrtjpzkrldwydkbopsml.supabase.co");
}

QString DesktopAppController::supabaseAnonKey()
{
    return s_testSupabase
        ? QStringLiteral("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImxzdWxudnhncmxwdXF0em9ubmVyIiwicm9sZSI6ImFub24iLCJpYXQiOjE3Nzg1ODY0OTIsImV4cCI6MjA5NDE2MjQ5Mn0.AyEQHLZadhj5r0BNkvPASaMZ0gTr4LAueq0SGVuua3s")
        : QStringLiteral("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im5ydGpwemtybGR3eWRrYm9wc21sIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzM4NTU0NTQsImV4cCI6MjA4OTQzMTQ1NH0.hzzyb5bFKDIFbrJ7Fa8INh57pWIkz52csQ2gQ_L302E");
}

QString DesktopAppController::supabaseConnectionInfo() const
{
    const QString projectId = s_testSupabase ? QStringLiteral("lsulnvxgrlpuqtzonner")
                                             : QStringLiteral("nrtjpzkrldwydkbopsml");
    const QString env = s_testSupabase ? tr("Test") : tr("Production");
    return tr("Project ID: %1 (%2)").arg(projectId, env);
}

void DesktopAppController::setSyncSettingsUnverified(bool unverified)
{
    if (m_syncSettingsUnverified == unverified)
        return;
    m_syncSettingsUnverified = unverified;
    emit syncSettingsUnverifiedChanged();
}

// Sampling a handful of rows is enough to judge the phrase: the pull decrypts
// every row with the same key, so if any row opens the phrase is right, and if
// none of them do it isn't.
static constexpr int kVerifySampleRows = 25;

void DesktopAppController::verifySyncSettings()
{
    if (m_syncVerifyInProgress)
        return;

    const QString email  = syncEmail();
    const QString passwd = syncPassword();
    const QString phrase = syncEncryptionPhrase();

    // Nothing to check against — sync is off, or the account was never filled in.
    if (!syncEnabled() || email.isEmpty() || passwd.isEmpty()) {
        setSyncSettingsUnverified(false);
        emit syncSettingsVerified(QStringLiteral("skipped"), QString());
        return;
    }

    m_syncVerifyInProgress = true;
    emit syncVerifyInProgressChanged();

    const QString url     = supabaseUrl();
    const QString anonKey = supabaseAnonKey();
    QPointer<DesktopAppController> self(this);

    // Its own thread: HttpClient/AuthManager block on a nested event loop, and
    // the sync engine's thread may well be mid-cycle. The verdict is posted back
    // through qApp and the guard is only dereferenced on the GUI thread, so a
    // controller torn down mid-check just drops the answer.
    QThread* worker = QThread::create([self, url, anonKey, email, passwd, phrase]() {
        HttpClient http;
        http.setBaseUrl(url + QStringLiteral("/rest/v1"));
        http.setApiKey(anonKey);
        http.setTimeoutMs(15000);

        bool    unreachable = false;
        QString authError;
        AuthManager auth;
        QObject::connect(&auth, &AuthManager::networkError, &auth,
                         [&unreachable](const QString&) { unreachable = true; });
        QObject::connect(&auth, &AuthManager::authenticationFailed, &auth,
                         [&authError](const QString& reason) { authError = reason; });

        QString status;
        QString detail;
        if (!auth.login(&http, url + QStringLiteral("/auth/v1/token?grant_type=password"),
                        email, passwd)) {
            // AuthManager reports an unreachable host (HTTP status 0) separately
            // from a rejected login, so an outage is never blamed on the password.
            status = unreachable ? QStringLiteral("offline") : QStringLiteral("credentials");
            detail = unreachable ? QString() : authError;
        } else {
            http.setAuthToken(auth.token());

            // "sync_data" is SqliteSyncPro's default postgres table and the one
            // this app syncs to (it never calls setPostgresTableName). Row-level
            // security scopes it to the signed-in account, so this reads only
            // this user's own rows.
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("select"), QStringLiteral("jsonrowdata"));
            query.addQueryItem(QStringLiteral("limit"), QString::number(kVerifySampleRows));
            const QByteArray response = http.get(QStringLiteral("sync_data"), query);

            if (!http.wasSuccessful()) {
                // Signing in worked but the read didn't. Report it as unchecked
                // rather than blaming the phrase for a server-side problem.
                status = QStringLiteral("offline");
                detail = http.lastError();
            } else {
                const QByteArray key = phrase.isEmpty()
                    ? QByteArray()
                    : QCryptographicHash::hash(phrase.toUtf8(), QCryptographicHash::Sha256);

                int encryptedRows = 0;
                int decryptedRows = 0;
                const QJsonArray rows = QJsonDocument::fromJson(response).array();
                for (const QJsonValue& row : rows) {
                    const QJsonValue data = row.toObject().value(QStringLiteral("jsonrowdata"));
                    // A plain JSON object was pushed before encryption was turned
                    // on; only the "enc:…" strings say anything about the phrase.
                    if (!data.isString())
                        continue;
                    ++encryptedRows;
                    if (!key.isEmpty() && !RowEncryption::decrypt(data.toString(), key).isEmpty())
                        ++decryptedRows;
                }
                // No encrypted rows yet (a brand-new account) proves nothing, so
                // only an outright failure to open any of them is an error.
                status = (encryptedRows > 0 && decryptedRows == 0)
                             ? QStringLiteral("encryption") : QStringLiteral("ok");
            }
        }

        QMetaObject::invokeMethod(qApp, [self, status, detail]() {
            if (self)
                self->finishSyncVerification(status, detail);
        }, Qt::QueuedConnection);
    });
    worker->setObjectName(QStringLiteral("SyncSettingsVerify"));
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void DesktopAppController::finishSyncVerification(const QString& status, const QString& detail)
{
    m_syncVerifyInProgress = false;
    emit syncVerifyInProgressChanged();

    // One check per round of edits, whatever the verdict: the user has been told,
    // and re-asking on every navigation (especially while offline) helps nobody.
    // Editing a field marks the settings unverified again.
    setSyncSettingsUnverified(false);

    QString message;
    if (status == QLatin1String("credentials")) {
        message = tr("The sync host rejected your sync email and password.\n\n"
                     "Cloud sync will not run until they are corrected. Check the "
                     "Sync Email and Sync Password fields under Cloud Sync.");
    } else if (status == QLatin1String("encryption")) {
        message = tr("Your encryption phrase does not match the one this account's "
                     "data was encrypted with.\n\n"
                     "Records synced from your other devices cannot be decrypted and "
                     "will be skipped. Check the Encryption Phrase field under Cloud Sync.");
    } else if (status == QLatin1String("offline")) {
        message = tr("Your cloud sync settings were saved, but the sync host could not "
                     "be reached to check them.\n\n"
                     "They will be checked again the next time sync runs.");
    }

    if (!message.isEmpty()) {
        QLog_Warning(SYNCERRORLOG,
            QString("Cloud sync settings check reported '%1'%2")
                .arg(status, detail.isEmpty() ? QString() : QStringLiteral(": ") + detail));
    }

    emit syncSettingsVerified(status, message);
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

    m_syncApi->setSyncHostType(1);        // always Supabase
    m_syncApi->setPostgrestUrl(supabaseUrl());
    m_syncApi->setSupabaseKey(supabaseAnonKey());
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

    m_syncSessionActive = true;     // keep the bar up until we reach 100%
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

void DesktopAppController::syncAll()
{
    if (!syncEnabled() || !global_DBObjects.isOpen())
        return;

    configureSyncApi();

    SqliteSyncPro* api = m_syncApi;
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!s_developerProfile.isEmpty())
        dataDir += "/" + s_developerProfile;
    const QString dbPath = dataDir + "/ProjectNotes.db";

    m_syncSessionActive = true;     // keep the bar up until we reach 100%
    setSyncProgress(0.01, false);   // show the indicator immediately

    QMetaObject::invokeMethod(api, [this, api, dbPath]() {
        if (!api->isInitialized()) {
            api->setDatabasePath(dbPath);
            if (!api->initialize()) {
                QMetaObject::invokeMethod(this, [this]() { setSyncProgress(0.0, true); },
                                          Qt::QueuedConnection);
                return;
            }
        }
        api->syncAll();
    }, Qt::QueuedConnection);
}

void DesktopAppController::showSyncStats()
{
    configureSyncApi();
    if (m_syncApi)
        m_syncApi->showStats(true);
}

void DesktopAppController::stopSync()
{
    if (!m_syncApi) return;
    SqliteSyncPro* api = m_syncApi;
    QMetaObject::invokeMethod(api, [api]() { api->shutdown(); }, Qt::QueuedConnection);
    m_syncSessionActive = false;
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
    if (m_syncNetworkError) {
        // Offline: hide the bar and let the offline indicator take over. A failed
        // cycle doesn't run checkSyncStatus, so clear the session here directly.
        m_syncSessionActive = false;
        m_syncProgress = -1.0;
    }
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
    // A cycle is actively transferring — we're in a sync session. Keep the bar
    // visible (onSyncStatusUpdated fills in the real database percentage after
    // the cycle); show a sliver until that first real percentage arrives.
    m_syncSessionActive = true;
    if (m_syncProgress < 0.0)
        m_syncProgress = 0.01;
    emit syncProgressChanged();
}

// Mirrors the Widgets app's onSyncStatusUpdated: percentComplete is the % of the
// database copied. The bar stays visible showing that percentage for the whole
// sync session and only hides once the database reports 100% (or the network
// drops, where the offline indicator takes over). The pending push/pull counts
// feed the detail text.
void DesktopAppController::onSyncStatusUpdated(int percentComplete, qint64 pendingPush, qint64 pendingPull)
{
    m_syncPercent     = percentComplete;
    m_syncPendingPush = pendingPush;
    m_syncPendingPull = pendingPull;

    if (m_syncNetworkError) {
        m_syncSessionActive = false;    // offline indicator takes over
        m_syncProgress = -1.0;
    } else if (percentComplete >= 100) {
        m_syncSessionActive = false;    // database fully synced — hide the bar
        m_syncProgress = -1.0;
        m_syncHasError = false;
    } else {
        m_syncSessionActive = true;     // still copying — keep the bar up
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
