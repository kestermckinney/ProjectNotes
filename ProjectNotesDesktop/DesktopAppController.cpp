// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "modelviewstate.h"
#include "legacywidgetsettings.h"
#include "DesktopAppController.h"

#include "databaseobjects.h"
#include "vcardparser.h"
#include "sortfilterproxymodel.h"
#include "FolderManager.h"
#include "SecurityScopedBookmarks.h"
#include "projectsmodel.h"
#include "projectnotesmodel.h"
#include "meetingattendeesmodel.h"
#include "notesactionitemsmodel.h"
#include "contactsmodel.h"
#include "companiesmodel.h"
#include "trackeritemsmodel.h"
#include "trackeritemcommentsmodel.h"
#include "projectteammembersmodel.h"
#include "projectlocationsmodel.h"
#include "statusreportitemsmodel.h"
#include "searchresultsmodel.h"
#include "FileFinderService.h"
#include "officedeeplink.h"
#include "ProjectNotesEmail/CommunicationRepository.h"
#include "ProjectNotesEmail/ArtifactStore.h"
#include "ProjectNotesEmail/CommunicationsController.h"
#include "ProjectNotesEmail/MeetingNotesPreparationFactory.h"
#include "ProjectNotesEmail/MeetingNotesReportPreparationFactory.h"
#include "ProjectNotesEmail/ProjectReportPreparationFactory.h"
#include "ProjectNotesEmail/ProjectEmailPreparationFactory.h"
#include "ProjectNotesEmail/LocationAttachmentDownloader.h"
#include "ProjectNotesEmail/RecipientSelectionModel.h"
#include "ProjectNotesEmail/RecipientAudienceResolver.h"
#include "ProjectNotesEmail/AudiencePresetStore.h"
#include "ProjectNotesEmail/EmailSettingsStore.h"
#include "ProjectNotesEmail/TransientCacheCleaner.h"
#include "ProjectNotesEmail/backends/GraphEmailBackend.h"
#include "ProjectNotesEmail/backends/ICloudEmailBackend.h"
#include "ProjectNotesEmail/EmlWriter.h"
#include "ProjectNotesEmail/backends/EmlFileEmailBackend.h"
#include "ProjectNotesEmail/backends/ThunderbirdEmailBackend.h"
#include "ProjectNotesEmailRendering/TextDocumentReportRenderer.h"
#include "ProjectNotesPlugins/PluginRegistry.h"
#include "ProjectNotesPlugins/PublishedSchema.h"
#include "ProjectNotesMeetings/MeetingService.h"
#include "ProjectNotesMeetings/backends/GraphMeetingBackend.h"
#include "ProjectNotesMeetings/backends/ICloudMeetingBackend.h"
#include "ProjectNotesMeetings/backends/IcsFileMeetingBackend.h"
#include "ProjectNotesMeetings/backends/ThunderbirdMeetingBackend.h"
#if defined(Q_OS_DARWIN)
#include "ProjectNotesEmail/backends/MacMailBackend.h"
#include "ProjectNotesMeetings/backends/MacCalendarBackend.h"
#endif
#if defined(Q_OS_WIN)
#include "ProjectNotesEmail/backends/OutlookLauncher.h"
#include "ProjectNotesMeetings/backends/OutlookClassicMeetingBackend.h"
#endif
#include "ProjectNotesIntegrations/MeetingTemplateStore.h"
#include "ProjectNotesIntegrations/Office365Service.h"
#include "ProjectNotesContacts/ContactSyncSettingsModel.h"
#include "ProjectNotesIntegrations/QtNetworkHttpTransport.h"
#include "ProjectNotesIntegrations/TemplateEditorModel.h"
#include "ProjectNotesIntegrations/MeetingTemplateEditorModel.h"
#include "Office365SettingsModel.h"

#include <QDesktopServices>

namespace {

// Every generated report starts from the project team, excluding the project
// manager, before any saved audience preset is applied.
PN::Comm::AudienceRule projectTeamAudienceRule()
{
    PN::Comm::AudienceRule rule;
    rule.source = PN::Comm::ContactSource::ProjectTeam;
    rule.excludeProjectManager = true;
    return rule;
}

PN::Comm::AudienceRule reviewPresetRule(PN::Comm::Workflow workflow,
                                      const PN::Comm::CommunicationSnapshot &snapshot,
                                      const PN::Comm::AudiencePreset &preset)
{
    auto rule = preset.rule;
    if (workflow != PN::Comm::Workflow::ProjectEmail) return rule;
    // Saved report audiences default to team members excluding the manager.
    // Project emails can explicitly select a manager or meeting guest too;
    // retain those IDs and their overrides without changing report behavior.
    rule.source = PN::Comm::ContactSource::ChosenContacts;
    rule.excludeProjectManager = false;
    for (const auto &contact : snapshot.contacts) {
        bool available = contact.projectTeamMember && contact.id != snapshot.projectManagerId;
        for (const auto &overrideValue : preset.overrides)
            available |= overrideValue.contactId == contact.id;
        if (available) rule.chosenContactIds.append(contact.id);
    }
    rule.chosenContactIds.removeDuplicates();
    return rule;
}

} // namespace

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
#include <utility>
#include <vector>

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
#include <QTextDocument>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <shellapi.h>
#elif defined(Q_OS_MACOS)
#include <QProcess>
#endif

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
    m_emailService = std::make_unique<PN::Comm::EmailService>();
    m_office365Service = m_fileFinder->office365Service();
    m_office365Transport = new PN::Comm::QtNetworkHttpTransport(this);
    m_office365Service->setHttpTransport(m_office365Transport);
    m_graphEmailBackend = std::make_unique<PN::Comm::GraphEmailBackend>(m_office365Service);
    m_emailService->registerBackend(PN::Comm::BackendId::Graph, m_graphEmailBackend.get());
    m_emlFileEmailBackend = std::make_unique<PN::Comm::EmlFileEmailBackend>(
        PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile));
    m_emailService->registerBackend(PN::Comm::BackendId::DefaultApp, m_emlFileEmailBackend.get());
    // Meetings use the same Email and Scheduling preference as email.  The
    // default-application choice opens an .ics file in the default calendar
    // app, as the email side opens an .eml file in the default mail app.
    m_meetingService = std::make_unique<PN::Comm::MeetingService>();
    m_icsMeetingBackend = std::make_unique<PN::Comm::IcsFileMeetingBackend>(
        PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile));
    m_meetingService->registerBackend(PN::Comm::BackendId::DefaultApp, m_icsMeetingBackend.get());
#if defined(Q_OS_DARWIN)
    // "MacOS Email and Calendar" drives Mail.app and Calendar.app directly via
    // AppKit/Apple Events (see MacMailBackend/MacCalendarBackend) rather than
    // opening a staged .eml/.ics file.
    m_macMailBackend = std::make_unique<PN::Comm::MacMailBackend>();
    m_emailService->registerBackend(PN::Comm::BackendId::MacMail, m_macMailBackend.get());
    m_macCalendarBackend = std::make_unique<PN::Comm::MacCalendarBackend>();
    m_meetingService->registerBackend(PN::Comm::BackendId::MacMail, m_macCalendarBackend.get());
#else
    m_emailService->registerBackend(PN::Comm::BackendId::MacMail, m_emlFileEmailBackend.get());
    m_meetingService->registerBackend(PN::Comm::BackendId::MacMail, m_icsMeetingBackend.get());
#endif
#if defined(Q_OS_WIN)
    // The new Outlook takes the staged .eml/.ics file like any default app, but
    // imports an .ics without its attendees; the Schedule meeting window offers
    // Copy Attendees for it.  Outlook (classic) opens the .eml the way its own
    // file association does, and builds meetings through its object model so
    // the attendees arrive filled in.
    const QString cacheDirectory = PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile);
    const auto openInNewOutlook = [](const QUrl &url) {
        return PN::Comm::OutlookLauncher::openWithNewOutlook(url.toLocalFile());
    };
    m_outlookEmailBackend = std::make_unique<PN::Comm::EmlFileEmailBackend>(
        cacheDirectory, openInNewOutlook, QStringLiteral("outlook-open-failed"));
    m_emailService->registerBackend(PN::Comm::BackendId::Outlook, m_outlookEmailBackend.get());
    m_outlookMeetingBackend = std::make_unique<PN::Comm::IcsFileMeetingBackend>(
        cacheDirectory, openInNewOutlook, QStringLiteral("outlook-open-failed"));
    m_meetingService->registerBackend(PN::Comm::BackendId::Outlook, m_outlookMeetingBackend.get());
    m_outlookClassicEmailBackend = std::make_unique<PN::Comm::EmlFileEmailBackend>(
        cacheDirectory,
        [](const QUrl &url) {
            return PN::Comm::OutlookLauncher::openWithClassicOutlook(url.toLocalFile(), QStringLiteral("/eml"));
        },
        QStringLiteral("outlook-classic-open-failed"));
    m_emailService->registerBackend(PN::Comm::BackendId::OutlookClassic, m_outlookClassicEmailBackend.get());
    m_outlookClassicMeetingBackend = std::make_unique<PN::Comm::OutlookClassicMeetingBackend>();
    m_meetingService->registerBackend(PN::Comm::BackendId::OutlookClassic, m_outlookClassicMeetingBackend.get());
#endif
    m_graphMeetingBackend = std::make_unique<PN::Comm::GraphMeetingBackend>(m_office365Service);
    m_meetingService->registerBackend(PN::Comm::BackendId::Graph, m_graphMeetingBackend.get());
    QSettings emailSettings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    registerThunderbirdBackends(PN::Comm::EmailSettingsStore(emailSettings).thunderbirdPath());
    m_communicationsController = std::make_unique<PN::Comm::CommunicationsController>(m_emailService.get());
    connect(m_communicationsController.get(), &PN::Comm::CommunicationsController::sourceRevalidationCancelled,
            this, [this] {
        if (m_communicationRepository && !m_pendingHandoffRevalidation.isNull())
            m_communicationRepository->cancel(m_pendingHandoffRevalidation);
        m_pendingHandoffRevalidation = {};
    });
    connect(m_communicationsController.get(), &PN::Comm::CommunicationsController::handoffFinished,
            this, [this](const PN::Comm::EmailHandoffResult &result) {
        // A Graph draft has a provider-supplied Outlook web link.  Open it as
        // soon as the complete draft (including its attachments) is ready.
        if (result.error.code.isEmpty() && result.presentationUrl.isValid()
            && !QDesktopServices::openUrl(result.presentationUrl))
            emit errorOccurred(tr("Cannot Open Email Draft"),
                               tr("The draft was created, but the browser could not open it. Open Outlook Drafts to review the email."));
        else if (result.error.code.isEmpty()
                 && m_communicationsController->preparation().backend == PN::Comm::BackendId::ICloud)
            emit infoOccurred(tr("iCloud Draft Created"),
                              tr("The email was saved in iCloud Mail Drafts. iCloud does not provide a link to the new draft, so open iCloud Mail to review and send it."));
    });
    m_recipientSelectionModel = std::make_unique<PN::Comm::RecipientSelectionModel>();
    m_templateEditorModel = std::make_unique<PN::Comm::TemplateEditorModel>(
        QStringLiteral("ProjectNotes") + s_developerProfile);
    m_meetingTemplateEditorModel = std::make_unique<PN::Comm::MeetingTemplateEditorModel>();
    m_office365SettingsModel = std::make_unique<Office365SettingsModel>(
        m_fileFinder, m_office365Service, this);
    m_contactSyncSettingsModel = std::make_unique<PN::Contacts::ContactSyncSettingsModel>(
        &global_DBObjects, m_office365Service, QStringLiteral("ProjectNotes") + s_developerProfile,
        dataLocation(), this);
    const auto iCloudCredentials = [this] {
        return m_contactSyncSettingsModel ? m_contactSyncSettingsModel->iCloudCredentials()
                                          : PN::Comm::ICloudCredentials{};
    };
    m_iCloudEmailBackend = std::make_unique<PN::Comm::ICloudEmailBackend>(iCloudCredentials);
    m_emailService->registerBackend(PN::Comm::BackendId::ICloud, m_iCloudEmailBackend.get());
    m_iCloudMeetingBackend = std::make_unique<PN::Comm::ICloudMeetingBackend>(iCloudCredentials);
    m_meetingService->registerBackend(PN::Comm::BackendId::ICloud, m_iCloudMeetingBackend.get());
    m_office365SettingsModel->setContactSyncModel(m_contactSyncSettingsModel.get());
    m_contactSyncSettingsModel->setIntegrationBackend(preferredEmailBackend());
    m_reportRenderer = new PN::Comm::TextDocumentReportRenderer(this);
    // A previous session that crashed never reached its quit-time cleanup.
    // Only one instance runs at a time (RunGuard), so nothing in the cache is
    // in use yet.
    cleanupTransientCache();
    // This controller is a QML singleton and may outlive the QML engine, so
    // normal process shutdown—not just destruction—is the cleanup boundary.
    connect(qApp, &QCoreApplication::aboutToQuit, this,
            [this] { cleanupTransientCache(); });
#ifdef QT_DEBUG
    connect(m_fileFinder, &FileFinderService::diagnostic, this,
            [](const QString &message) { QLog_Debug(DEBUGLOG, message); });
#endif
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
    m_contactSyncSettingsModel.reset();
    cleanupTransientCache();

    // Join its read-only worker before closing the shared database connection.
    m_communicationRepository.reset();

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

void DesktopAppController::cleanupTransientCache()
{
    cancelProjectEmailReview();
    // Rendered reports, staged attachments, meeting .ics files and plug-in
    // report output are transient cache data.  A cleanup call is idempotent,
    // which also makes the destructor safe after the aboutToQuit handler has
    // already run.
    if (m_reportRenderer && !m_pendingPdfRender.isNull())
        m_reportRenderer->cancel(m_pendingPdfRender);
    const QStringList failed =
        PN::Comm::TransientCacheCleaner(PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile)).clean();
    for (const QString &path : failed)
        QLog_Warning(CONSOLELOG, QStringLiteral("Could not remove temporary file %1").arg(path));
}

void DesktopAppController::registerThunderbirdBackends(const QString &command)
{
    // A command is parsed by ThunderbirdLauncher and launched without a shell.
    m_emailService->registerBackend(PN::Comm::BackendId::Thunderbird, nullptr);
    m_meetingService->registerBackend(PN::Comm::BackendId::Thunderbird, nullptr);
    m_thunderbirdEmailBackend.reset();
    m_thunderbirdMeetingBackend.reset();
#ifdef PN_APP_STORE
    // Starting Thunderbird is not possible from the App Sandbox.
    Q_UNUSED(command);
#else
    if (command.trimmed().isEmpty())
        return;
    m_thunderbirdEmailBackend = std::make_unique<PN::Comm::ThunderbirdEmailBackend>(command.trimmed());
    m_emailService->registerBackend(PN::Comm::BackendId::Thunderbird, m_thunderbirdEmailBackend.get());
    m_thunderbirdMeetingBackend = std::make_unique<PN::Comm::ThunderbirdMeetingBackend>(
        PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile), command.trimmed());
    m_meetingService->registerBackend(PN::Comm::BackendId::Thunderbird, m_thunderbirdMeetingBackend.get());
#endif
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

using PN::ModelViewState::sourceModelOf;
using PN::ModelViewState::restoreColumnFilters;
using PN::ModelViewState::restoreSort;

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
    QSettings localSettings(QStringLiteral("ProjectNotes") + s_developerProfile,
                            QStringLiteral("AppSettings"));
    PN::LegacyWidgetSettings::cleanupLocal(localSettings);

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
    global_DBObjects.companiesmodel()->refresh();
    global_DBObjects.contactsmodel()->refresh();

    // Restore any column filters left active in the Filter Editor before the
    // last restart (persisted to application_settings, keyed per table). No-op
    // (and no extra refresh) for models with nothing persisted; allitemsmodel
    // is otherwise loaded lazily, so this only forces it early when needed.
    restoreColumnFilters(global_DBObjects.projectinformationmodel());
    restoreColumnFilters(global_DBObjects.companiesmodel());
    restoreColumnFilters(global_DBObjects.contactsmodel());
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
                                     QStringLiteral("contacts"), QStringLiteral("companies"),
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

    // Snapshot workers always get a fresh read-only connection. A newly opened
    // database receives a new generation so a later action cannot use a stale
    // context from a prior database session.
    m_communicationRepository = std::make_unique<PN::Comm::AsyncSqliteCommunicationRepository>(
        dbPath, &db_rwlock, ++m_communicationDatabaseGeneration);
    m_templateEditorModel->setDatabaseKey(dbPath);
    m_contactSyncSettingsModel->setDatabase(dbPath);
    const auto meetingDefaults = PN::Comm::MeetingTemplateStore().initializeDefaults();
    if (!meetingDefaults.ok())
        emit errorOccurred(tr("Meeting Templates"),
                           tr("Could not initialize standard meeting types. Check Settings > Meeting Templates."));
    m_meetingTemplateEditorModel->setDatabaseKey(dbPath);

    m_databaseOpen = true;
    emit databaseReady();
    emit projectManagerChanged();   // picks up any PM configured in a prior session

    // Start the plugins now that the database is open (some query it as they
    // register their menus).
    ensureJsPlugins();

    // Auto-start cloud sync if the user has it enabled (same as the Widgets app).
    // Deferred so the UI is up first; the heavy bootstrap runs on the sync thread.
    if (syncEnabled() && !syncEmail().isEmpty() && !syncPassword().isEmpty())
        QTimer::singleShot(0, this, &DesktopAppController::syncNow);

    return true;
}

// ── Models ───────────────────────────────────────────────────────────────────

QObject* DesktopAppController::fileFinder() const
{ return m_fileFinder; }

QObject* DesktopAppController::communicationsController() const
{ return m_communicationsController.get(); }

QObject* DesktopAppController::recipientSelectionModel() const
{ return m_recipientSelectionModel.get(); }

QObject* DesktopAppController::templateEditorModel() const
{ return m_templateEditorModel.get(); }

QObject* DesktopAppController::meetingTemplateEditorModel() const
{ return m_meetingTemplateEditorModel.get(); }

QObject* DesktopAppController::office365SettingsModel() const
{ return m_office365SettingsModel.get(); }

QObject* DesktopAppController::contactSyncSettingsModel() const
{ return m_contactSyncSettingsModel.get(); }

QString DesktopAppController::preferredEmailBackend() const
{
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    return PN::Comm::toStableString(PN::Comm::EmailSettingsStore(settings).preferredBackend());
}

void DesktopAppController::setPreferredEmailBackend(const QString &backend)
{
    const auto parsed = PN::Comm::backendIdFromStableString(backend);
    if (!parsed || !PN::Comm::isBackendAvailableOnThisPlatform(*parsed)) return;
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    PN::Comm::EmailSettingsStore(settings).setPreferredBackend(*parsed);
    m_contactSyncSettingsModel->setIntegrationBackend(PN::Comm::toStableString(*parsed));
    emit emailSettingsChanged();
}

QString DesktopAppController::thunderbirdExecutable() const
{
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    return PN::Comm::EmailSettingsStore(settings).thunderbirdPath();
}

void DesktopAppController::setThunderbirdExecutable(const QString &path)
{
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    PN::Comm::EmailSettingsStore(settings).setThunderbirdPath(path.trimmed());
    // Recreate the adapters so subsequent reviews use the newly saved command.
    registerThunderbirdBackends(path);
    emit emailSettingsChanged();
}

QString DesktopAppController::emailSenderAddress() const
{
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    return PN::Comm::EmailSettingsStore(settings).senderAddress();
}

void DesktopAppController::setEmailSenderAddress(const QString &address)
{
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    PN::Comm::EmailSettingsStore(settings).setSenderAddress(address.trimmed());
    emit emailSettingsChanged();
}

bool DesktopAppController::stageRequestedGeneratedAttachment(PN::Comm::EmailPreparation *preparation,
                                                              PN::Comm::ValidationResult *validation)
{
    if (!preparation)
        return true;
    PN::Comm::ArtifactStore store(PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile));
    PN::Comm::OperationManifest manifest;
    manifest.operationId = preparation->operationId;
    manifest.workflow = preparation->source.workflow;
    manifest.databaseKey = preparation->source.databaseKey;
    manifest.projectId = preparation->source.projectId;
    manifest.backend = preparation->backend;
    PN::Comm::ServiceError error;
    if (!store.createOperation(manifest, &error)) {
        validation->addError(error.code.isEmpty() ? QStringLiteral("artifact-staging-failed") : error.code,
                             QStringLiteral("attachments"));
        return false;
    }
    // Thunderbird consumes an operation-owned HTML file for its compose body,
    // even when the user did not choose to retain an HTML export.
    if (preparation->backend == PN::Comm::BackendId::Thunderbird) {
        const auto body = store.stageGeneratedContent(preparation->operationId,
                                                      QStringLiteral("message.html"),
                                                      preparation->document.htmlDocument.toUtf8(),
                                                      QStringLiteral("text/html"));
        if (!body.ok() || !m_thunderbirdEmailBackend) {
            validation->addError(body.error.code.isEmpty() ? QStringLiteral("thunderbird-body-staging-failed")
                                                           : body.error.code,
                                 QStringLiteral("body"));
            return false;
        }
        const QString operationDirectory = QDir(store.stagingRoot()).filePath(
            preparation->operationId.toString(QUuid::WithoutBraces));
        m_thunderbirdEmailBackend->setBodyFile(preparation->operationId, body.artifact.absolutePath,
                                               operationDirectory);
    }
    if (!preparation->retainHtml && preparation->mode != PN::Comm::EmailMode::HtmlAttachment
        && !preparation->createHtmlExport)
        return true;

    QString name = preparation->document.fileStem.trimmed();
    if (name.isEmpty())
        name = QStringLiteral("ProjectNotesReport");
    for (qsizetype index = 0; index < name.size(); ++index) {
        const QChar character = name.at(index);
        if (character == u'/' || character == u'\\' || character == u':' || character == u'*'
            || character == u'?' || character == u'"' || character == u'<' || character == u'>'
            || character == u'|')
            name[index] = u'_';
    }
    name += QStringLiteral(".html");
    const auto staged = store.stageGeneratedContent(preparation->operationId, name,
                                                    preparation->document.htmlDocument.toUtf8(),
                                                    QStringLiteral("text/html"));
    if (!staged.ok()) {
        validation->addError(staged.error.code.isEmpty() ? QStringLiteral("artifact-staging-failed")
                                                         : staged.error.code,
                             QStringLiteral("attachments"));
        return false;
    }
    if (preparation->mode == PN::Comm::EmailMode::HtmlAttachment)
        preparation->attachments.append(staged.artifact);
    else
        preparation->exports.append(staged.artifact);
    return true;
}

void DesktopAppController::stageRequestedGeneratedAttachmentAsync(
    PN::Comm::EmailPreparation preparation,
    std::function<void(std::optional<PN::Comm::EmailPreparation>, PN::Comm::ValidationResult)> completion)
{
    if (preparation.backend == PN::Comm::BackendId::Graph && m_office365Service) {
        const PN::Comm::Office365Account account = m_office365Service->account();
        preparation.accountKey = account.label;
        preparation.accountGeneration = account.generation;
    }
    PN::Comm::ValidationResult validation;
    if (!stageRequestedGeneratedAttachment(&preparation, &validation)) {
        completion(std::nullopt, std::move(validation));
        return;
    }
    if (preparation.mode != PN::Comm::EmailMode::PdfAttachment && !preparation.displayPdf
        && !preparation.createPdfExport) {
        completion(std::move(preparation), std::move(validation));
        return;
    }
    if (!m_pendingPdfRender.isNull())
        m_reportRenderer->cancel(m_pendingPdfRender);
    m_pendingPdfRender = preparation.operationId;
    QString stem = preparation.document.fileStem.trimmed();
    if (stem.isEmpty()) stem = QStringLiteral("ProjectNotesReport");
    for (QChar &character : stem)
        if (QStringLiteral("/\\:*?\"<>|").contains(character)) character = u'_';
    const QString displayName = stem + QStringLiteral(".pdf");
    PN::Comm::ArtifactStore store(PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile)); PN::Comm::ServiceError error;
    const QString path = store.reserveGeneratedPath(preparation.operationId, displayName, &error);
    if (path.isEmpty()) {
        m_pendingPdfRender = {};
        validation.addError(error.code.isEmpty() ? QStringLiteral("artifact-staging-failed") : error.code,
                            QStringLiteral("attachments"));
        completion(std::nullopt, std::move(validation));
        return;
    }
    PN::Comm::RenderRequest request;
    request.operationId = preparation.operationId;
    request.previewRevision = preparation.previewRevision;
    request.html = preparation.document.htmlDocument;
    request.outputStem = stem;
    request.pageLayout = preparation.document.pdfLayout;
    request.outputPdfPath = path;
    QPointer<DesktopAppController> self(this);
    m_reportRenderer->render(std::move(request), [self, operationId = preparation.operationId,
                                                      preparation = std::move(preparation), displayName,
                                                      validation = std::move(validation),
                                                      completion = std::move(completion)](PN::Comm::RenderResult result) mutable {
        if (!self) return;
        if (self->m_pendingPdfRender != operationId)
            return;
        self->m_pendingPdfRender = {};
        if (!result.error.code.isEmpty()) {
            validation.addError(result.error.code, QStringLiteral("attachments"));
            completion(std::nullopt, std::move(validation));
            return;
        }
        PN::Comm::ArtifactStore store(PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile));
        const auto registered = store.registerGeneratedFile(preparation.operationId, result.pdf.absolutePath,
                                                            displayName, QStringLiteral("application/pdf"));
        if (!registered.ok()) {
            validation.addError(registered.error.code.isEmpty() ? QStringLiteral("artifact-staging-failed") : registered.error.code,
                                QStringLiteral("attachments"));
            completion(std::nullopt, std::move(validation));
            return;
        }
        if (preparation.mode == PN::Comm::EmailMode::PdfAttachment)
            preparation.attachments.append(registered.artifact);
        else
            preparation.exports.append(registered.artifact);
        if (preparation.displayPdf
            && !QDesktopServices::openUrl(QUrl::fromLocalFile(registered.artifact.absolutePath))) {
            validation.addWarning(QStringLiteral("report-display-failed"), QStringLiteral("report.display"),
                                  self->tr("The generated PDF was staged but could not be opened by the desktop."));
        }
        completion(std::move(preparation), std::move(validation));
    });
}

bool DesktopAppController::prepareProjectEmailReview(const QString &projectId, const QString &contactId)
{
    if (projectId.trimmed().isEmpty()) return false;
    return prepareProjectEmail(projectId, contactId, {}, false);
}

bool DesktopAppController::prepareContactEmailReview(const QString &contactId)
{
    if (contactId.trimmed().isEmpty()) return false;
    return prepareProjectEmail({}, contactId, {}, false);
}

bool DesktopAppController::prepareLocationEmailReview(const QString &projectId, const QString &locationId, bool attachment)
{
    if (projectId.trimmed().isEmpty() || locationId.trimmed().isEmpty()) return false;
    return prepareProjectEmail(projectId, {}, locationId, attachment);
}

void DesktopAppController::cancelProjectEmailReview()
{
    const bool pending = !m_pendingProjectEmailReview.isNull();
    const QUuid operationId = m_pendingProjectEmailReview;
    m_pendingProjectEmailReview = {};
    if (pending && m_communicationRepository) m_communicationRepository->cancel(operationId);
    if (m_locationAttachmentDownloader) m_locationAttachmentDownloader->cancel();
}

bool DesktopAppController::prepareProjectEmail(const QString &projectId, const QString &contactId,
                                             const QString &locationId, bool attachment)
{
    if (!m_databaseOpen || !m_communicationRepository
        || !m_communicationsController->beginReviewPreparation()) return false;
    cancelProjectEmailReview();
    // Invalidate callbacks belonging to another review sharing this controller.
    if (!m_pendingPdfRender.isNull()) {
        const QUuid operationId = m_pendingPdfRender;
        m_pendingPdfRender = {};
        m_reportRenderer->cancel(operationId);
    }
    m_pendingMeetingNotesReview = {};
    m_pendingMeetingNotesReportReview = {};
    m_pendingProjectReportReview = {};
    m_pendingMeetingReview = {};
    m_meetingReview.reset();
    m_projectEmailReview.reset();
    m_reviewAudienceSnapshot.reset();
    m_reviewAudienceRule.reset();
    m_projectReportOptions.reset();
    m_recipientSelectionModel->setAudience({});
    setAppliedReviewAudiencePreset({});
    PN::Comm::SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source = {global_DBObjects.getDatabaseFile(), m_communicationDatabaseGeneration, projectId, {},
                      projectId.isEmpty() ? PN::Comm::Workflow::ContactEmail
                          : locationId.isEmpty() ? PN::Comm::Workflow::ProjectEmail : PN::Comm::Workflow::ShareLocation,
                      contactId, locationId};
    request.managingCompanyId = managingCompanyId();
    request.projectManagerId = projectManagerId();
    request.capturedAt = QDateTime::currentDateTimeUtc();
    m_pendingProjectEmailReview = request.operationId;
    QPointer<DesktopAppController> self(this);
    m_communicationRepository->loadSnapshot(request, [self, request, attachment](PN::Comm::SnapshotResult result) {
        if (!self || self->m_pendingProjectEmailReview != result.operationId) return;
        const auto fail = [self](QString code, QString message) {
            self->m_pendingProjectEmailReview = {};
            PN::Comm::ValidationResult validation;
            validation.addError(std::move(code), QStringLiteral("email"), std::move(message));
            self->m_communicationsController->setReviewValidation(std::move(validation));
        };
        if (!result.error.code.isEmpty()) { fail(result.error.code, result.error.displayText); return; }
        const auto contentTemplate = request.source.workflow == PN::Comm::Workflow::ContactEmail
            ? std::optional<PN::Comm::CommunicationTemplate>(PN::Comm::CommunicationTemplate{})
            : self->resolveWorkflowTemplate(request.source.workflow);
        if (!contentTemplate) { fail(QStringLiteral("template-required"), self->tr("Configure this email type in Settings.")); return; }
        QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
        PN::Comm::EmailSettingsStore emailSettings(settings);
        auto review = PN::Comm::ProjectEmailPreparationFactory::create(result.snapshot, request.source,
            emailSettings.preferredBackend(), *contentTemplate, attachment);
        if (!review.validation.ok()) {
            self->m_pendingProjectEmailReview = {};
            self->m_communicationsController->setReviewValidation(review.validation);
            return;
        }
        review.preparation.senderAddress = emailSettings.senderAddress();
        const QUrl attachmentUrl = review.attachmentUrl;
        self->stageRequestedGeneratedAttachmentAsync(review.preparation,
            [self, request, review, snapshot = result.snapshot, attachmentUrl]
            (std::optional<PN::Comm::EmailPreparation> staged, PN::Comm::ValidationResult validation) mutable {
                if (!self || self->m_pendingProjectEmailReview != request.operationId) return;
                if (!staged) {
                    self->m_pendingProjectEmailReview = {};
                    self->m_communicationsController->setReviewValidation(std::move(validation));
                    return;
                }
                auto finish = [self, request, review, snapshot, prepared = *staged]
                    (PN::Comm::ArtifactStoreResult attachmentResult) mutable {
                    if (!self || self->m_pendingProjectEmailReview != request.operationId) return;
                    self->m_pendingProjectEmailReview = {};
                    if (!attachmentResult.ok()) {
                        PN::Comm::ValidationResult validation;
                        validation.addError(attachmentResult.error.code, QStringLiteral("attachments"), attachmentResult.error.displayText);
                        self->m_communicationsController->setReviewValidation(std::move(validation));
                        return;
                    }
                    if (!attachmentResult.artifact.absolutePath.isEmpty()) prepared.attachments.append(attachmentResult.artifact);
                    self->m_projectEmailReview = prepared;
                    self->m_reviewAudienceSnapshot = snapshot;
                    self->m_reviewAudienceRule = review.rule;
                    self->m_recipientSelectionModel->setAudience(review.audience);
                    self->m_communicationsController->setPreparation(std::move(prepared));
                    self->m_recipientSelectionModel->setInternalReportContext(false, {});
                    // A clicked contact is an explicit recipient choice, so
                    // never replace it automatically with the project default.
                    if (request.source.contactId.isEmpty())
                        self->applyDefaultReviewAudiencePreset();
                };
                const QString cache = PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile);
                if (attachmentUrl.isLocalFile()) {
                    finish(PN::Comm::ArtifactStore(cache).stageUserAttachment(staged->operationId, attachmentUrl.toLocalFile()));
                } else if (!attachmentUrl.isEmpty()) {
                    if (!self->m_locationAttachmentDownloader)
                        self->m_locationAttachmentDownloader = std::make_unique<PN::Comm::LocationAttachmentDownloader>();
                    self->m_locationAttachmentDownloader->download(attachmentUrl, cache, staged->operationId, std::move(finish));
                } else finish({});
            });
    });
    return true;
}

bool DesktopAppController::prepareMeetingNotesReview(const QString& projectId, const QString& noteId)
{
    if (!m_databaseOpen || !m_communicationRepository || projectId.trimmed().isEmpty() || noteId.trimmed().isEmpty())
        return false;
    if (!m_communicationsController->beginReviewPreparation())
        return false;

    cancelProjectEmailReview();
    PN::Comm::SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source = {global_DBObjects.getDatabaseFile(), m_communicationDatabaseGeneration,
                      projectId, {noteId}, PN::Comm::Workflow::SendMeetingNotes};
    request.managingCompanyId = managingCompanyId();
    request.projectManagerId = projectManagerId();
    request.capturedAt = QDateTime::currentDateTimeUtc();
    m_pendingMeetingNotesReview = request.operationId;
    m_meetingNotesReview.reset();
    m_meetingNotesSnapshot.reset();
    m_meetingNotesSource.reset();
    m_reviewAudienceSnapshot.reset();
    m_reviewAudienceRule.reset();
    m_meetingReview.reset();
    setAppliedReviewAudiencePreset({});

    QPointer<DesktopAppController> self(this);
    m_communicationRepository->loadSnapshot(request, [self, request](PN::Comm::SnapshotResult result) {
        if (!self || self->m_pendingMeetingNotesReview != result.operationId)
            return;
        self->m_pendingMeetingNotesReview = {};
        if (!result.error.code.isEmpty()) {
            emit self->errorOccurred(self->tr("Cannot Prepare Meeting Notes"),
                                     result.error.displayText.isEmpty() ? result.error.code : result.error.displayText);
            return;
        }
        // Persisted backend selection belongs in the immutable preparation so a
        // later, capability-checked handoff cannot silently switch clients.
        // This review path still registers no backend and therefore cannot send.
        QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
        const PN::Comm::BackendId backend = PN::Comm::EmailSettingsStore(settings).preferredBackend();
        // Resolve the configured template here rather than asking at delivery
        // time so its subject and body are always applied to the prepared note.
        const auto contentTemplate =
            self->resolveWorkflowTemplate(PN::Comm::Workflow::SendMeetingNotes);
        auto review = PN::Comm::MeetingNotesPreparationFactory::create(
            result.snapshot, request.source, backend, PN::Comm::EmailMode::InlineHtml,
            contentTemplate ? &*contentTemplate : nullptr);
        if (!review) {
            emit self->errorOccurred(self->tr("Cannot Prepare Meeting Notes"),
                                     self->tr("The selected note could not be prepared for review."));
            return;
        }
        review->preparation.senderAddress = PN::Comm::EmailSettingsStore(settings).senderAddress();
        self->stageRequestedGeneratedAttachmentAsync(review->preparation,
            [self, audience = review->audience, snapshot = result.snapshot, source = request.source]
            (std::optional<PN::Comm::EmailPreparation> prepared, PN::Comm::ValidationResult validation) mutable {
                if (!self) return;
                if (!prepared) { self->m_communicationsController->setReviewValidation(std::move(validation)); return; }
                self->m_recipientSelectionModel->setAudience(audience);
                self->m_meetingNotesReview = *prepared;
                self->m_meetingNotesSnapshot = snapshot;
                self->m_meetingNotesSource = source;
                self->m_reviewAudienceSnapshot = snapshot;
                self->m_communicationsController->setPreparation(std::move(*prepared));
                self->updateRecipientInternalReportContext();
                if (!validation.issues.isEmpty())
                    self->m_communicationsController->setReviewValidation(std::move(validation));
            });
    });
    return true;
}

bool DesktopAppController::prepareMeetingNotesReportReviewWithOptions(const QString& projectId,
                                                                       const QString& reportingDate,
                                                                       bool internalReport,
                                                                       const QString& emailMode,
                                                                       bool retainHtml,
                                                                       bool displayPdf)
{
    const auto mode = PN::Comm::emailModeFromStableString(emailMode);
    if (!mode)
        return false;
    if (!m_databaseOpen || !m_communicationRepository || projectId.trimmed().isEmpty()
        || m_communicationsController->busy())
        return false;
    if (!m_communicationsController->beginReviewPreparation())
        return false;

    cancelProjectEmailReview();
    const QDate date = QDate::fromString(reportingDate.trimmed(), QStringLiteral("MM/dd/yyyy"));
    if (!date.isValid()) {
        PN::Comm::ValidationResult validation;
        validation.addError(QStringLiteral("reporting-date-required"), QStringLiteral("reportingDate"),
                            tr("Choose a valid reporting date."));
        m_communicationsController->setReviewValidation(std::move(validation));
        return false;
    }

    PN::Comm::SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source = {global_DBObjects.getDatabaseFile(), m_communicationDatabaseGeneration,
                      projectId, {}, PN::Comm::Workflow::MeetingNotesReport};
    request.managingCompanyId = managingCompanyId();
    request.projectManagerId = projectManagerId();
    request.capturedAt = QDateTime::currentDateTimeUtc();
    m_pendingMeetingNotesReportReview = request.operationId;
    m_meetingNotesReportReview.reset();
    m_meetingNotesReportSnapshot.reset();
    m_meetingNotesReportSource.reset();
    m_reviewAudienceSnapshot.reset();
    m_reviewAudienceRule.reset();
    m_meetingReview.reset();
    setAppliedReviewAudiencePreset({});

    QPointer<DesktopAppController> self(this);
    m_communicationRepository->loadSnapshot(request, [self, request, date, internalReport, mode, retainHtml, displayPdf](PN::Comm::SnapshotResult result) {
        if (!self || self->m_pendingMeetingNotesReportReview != result.operationId)
            return;
        self->m_pendingMeetingNotesReportReview = {};
        if (!result.error.code.isEmpty()) {
            emit self->errorOccurred(self->tr("Cannot Prepare Meeting Notes Report"),
                                     result.error.displayText.isEmpty() ? result.error.code : result.error.displayText);
            return;
        }

        QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
        const PN::Comm::BackendId backend = PN::Comm::EmailSettingsStore(settings).preferredBackend();
        PN::Comm::ValidationResult validation;
        const auto contentTemplate =
            self->resolveWorkflowTemplate(PN::Comm::Workflow::MeetingNotesReport);
        const auto preparation = PN::Comm::MeetingNotesReportPreparationFactory::create(
            result.snapshot, request.source, date, internalReport, backend,
            *mode, retainHtml, &validation, contentTemplate ? &*contentTemplate : nullptr);
        if (!preparation) {
            self->m_communicationsController->setReviewValidation(std::move(validation));
            return;
        }

        // The native report begins with the same project-team review audience as
        // Send Meeting Notes.  RecipientSelectionModel is the only place where
        // later selection and To/Cc/Bcc edits are resolved.
        const PN::Comm::AudienceRule audienceRule = projectTeamAudienceRule();
        self->m_recipientSelectionModel->setAudience(PN::Comm::resolveAudience(result.snapshot, audienceRule));
        PN::Comm::EmailPreparation prepared = *preparation;
        prepared.senderAddress = PN::Comm::EmailSettingsStore(settings).senderAddress();
        prepared.displayPdf = displayPdf;
        // The report dialog saves a file by preparing in that format's
        // attachment mode, so no PDF is rendered unless the mode needs one.
        prepared.createHtmlExport = true;
        const PN::Comm::RecipientResolution recipients = self->m_recipientSelectionModel->resolve();
        prepared.recipients = recipients.recipients;
        prepared.addressLater = recipients.addressLaterExplicitlyChosen;
        self->stageRequestedGeneratedAttachmentAsync(std::move(prepared),
            [self, snapshot = result.snapshot, source = request.source, internalReport,
             audienceRule, recipientValidation = recipients.validation]
            (std::optional<PN::Comm::EmailPreparation> staged, PN::Comm::ValidationResult validation) mutable {
                if (!self) return;
                if (!staged) { self->m_communicationsController->setReviewValidation(std::move(validation)); return; }
                self->m_meetingNotesReportReview = *staged;
                self->m_meetingNotesReportSnapshot = snapshot;
                self->m_meetingNotesReportSource = source;
                self->m_reviewAudienceSnapshot = snapshot;
                self->m_reviewAudienceRule = audienceRule;
                self->m_meetingNotesReportInternal = internalReport;
                self->m_communicationsController->setPreparation(std::move(*staged));
                self->updateRecipientInternalReportContext();
                self->applyDefaultReviewAudiencePreset();
                validation.issues += recipientValidation.issues;
                if (!validation.issues.isEmpty())
                    self->m_communicationsController->setReviewValidation(std::move(validation));
            });
    });
    return true;
}

bool DesktopAppController::prepareStatusReportReviewWithOptions(const QString& projectId,
                                                                 const QString& reportingDate,
                                                                 bool internalReport,
                                                                 const QString& emailMode,
                                                                 bool retainHtml,
                                                                 bool displayPdf)
{
    const auto mode = PN::Comm::emailModeFromStableString(emailMode);
    return mode && prepareProjectReportReview(projectId, reportingDate, internalReport,
                                              PN::Comm::Workflow::StatusReport, *mode, retainHtml, displayPdf);
}

bool DesktopAppController::prepareTrackerReportReviewWithOptions(const QString& projectId,
                                                                  const QString& reportingDate,
                                                                  bool internalReport,
                                                                  const QString& emailMode,
                                                                  const QStringList& itemTypes,
                                                                  const QStringList& statuses,
                                                                  bool retainHtml,
                                                                  bool displayPdf)
{
    const auto mode = PN::Comm::emailModeFromStableString(emailMode);
    if (!mode || itemTypes.isEmpty() || statuses.isEmpty()) {
        if (m_communicationsController->beginReviewPreparation()) {
            PN::Comm::ValidationResult validation;
            if (!mode)
                validation.addError(QStringLiteral("email-mode-invalid"), QStringLiteral("emailMode"),
                                    tr("Choose a supported email mode."));
            if (itemTypes.isEmpty())
                validation.addError(QStringLiteral("tracker-item-types-required"), QStringLiteral("tracker.itemTypes"),
                                    tr("Choose at least one item type."));
            if (statuses.isEmpty())
                validation.addError(QStringLiteral("tracker-statuses-required"), QStringLiteral("tracker.statuses"),
                                    tr("Choose at least one item status."));
            m_communicationsController->setReviewValidation(std::move(validation));
        }
        return false;
    }
    PN::Comm::TrackerFilters filters;
    filters.itemTypes = QSet<QString>(itemTypes.cbegin(), itemTypes.cend());
    filters.statuses = QSet<QString>(statuses.cbegin(), statuses.cend());
    return prepareProjectReportReview(projectId, reportingDate, internalReport,
                                      PN::Comm::Workflow::TrackerItemsReport, *mode, retainHtml, displayPdf, std::move(filters));
}

bool DesktopAppController::prepareProjectReportReview(const QString &projectId, const QString &reportingDate,
                                                       bool internalReport, PN::Comm::Workflow workflow,
                                                       PN::Comm::EmailMode emailMode,
                                                       bool retainHtml,
                                                       bool displayPdf,
                                                       std::optional<PN::Comm::TrackerFilters> trackerFilters)
{
    if (!m_databaseOpen || !m_communicationRepository || projectId.trimmed().isEmpty()
        || m_communicationsController->busy())
        return false;
    if (!m_communicationsController->beginReviewPreparation())
        return false;
    cancelProjectEmailReview();
    const QDate date = QDate::fromString(reportingDate.trimmed(), QStringLiteral("MM/dd/yyyy"));
    if (!date.isValid()) {
        PN::Comm::ValidationResult validation;
        validation.addError(QStringLiteral("reporting-date-required"), QStringLiteral("reportingDate"),
                            tr("Choose a valid reporting date."));
        m_communicationsController->setReviewValidation(std::move(validation));
        return false;
    }
    PN::Comm::SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source = {global_DBObjects.getDatabaseFile(), m_communicationDatabaseGeneration, projectId, {}, workflow};
    request.managingCompanyId = managingCompanyId(); request.projectManagerId = projectManagerId();
    request.capturedAt = QDateTime::currentDateTimeUtc();
    m_pendingProjectReportReview = request.operationId;
    m_projectReportReview.reset();
    m_projectReportSnapshot.reset();
    m_projectReportSource.reset();
    m_projectReportOptions.reset();
    m_reviewAudienceSnapshot.reset();
    m_reviewAudienceRule.reset();
    m_meetingReview.reset();
    setAppliedReviewAudiencePreset({});
    QPointer<DesktopAppController> self(this);
    m_communicationRepository->loadSnapshot(request, [self, request, date, internalReport, workflow, emailMode, retainHtml, displayPdf,
                                                       trackerFilters = std::move(trackerFilters)](PN::Comm::SnapshotResult result) {
        if (!self || self->m_pendingProjectReportReview != result.operationId) return;
        self->m_pendingProjectReportReview = {};
        if (!result.error.code.isEmpty()) {
            emit self->errorOccurred(self->tr("Cannot Prepare Report"),
                                     result.error.displayText.isEmpty() ? result.error.code : result.error.displayText);
            return;
        }
        PN::Comm::ReportOptions options = PN::Comm::defaultReportOptions(workflow, date);
        options.internalReport = internalReport;
        options.emailMode = emailMode;
        options.retainHtml = retainHtml;
        options.displayPdf = displayPdf;
        if (workflow == PN::Comm::Workflow::TrackerItemsReport && trackerFilters)
            options.tracker = *trackerFilters;
        QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
        const auto backend = PN::Comm::EmailSettingsStore(settings).preferredBackend();
        const auto contentTemplate = self->resolveWorkflowTemplate(workflow);
        const PN::Comm::CommunicationTemplate *templatePointer =
            contentTemplate ? &*contentTemplate : nullptr;
        const auto review = workflow == PN::Comm::Workflow::StatusReport
            ? PN::Comm::ProjectReportPreparationFactory::createStatus(result.snapshot, request.source, options, backend,
                                                                      templatePointer)
            : PN::Comm::ProjectReportPreparationFactory::createTracker(result.snapshot, request.source, options, backend,
                                                                       templatePointer);
        if (!review) {
            PN::Comm::ValidationResult validation;
            validation.addError(QStringLiteral("report-preparation-failed"), QStringLiteral("report"));
            self->m_communicationsController->setReviewValidation(std::move(validation));
            return;
        }
        PN::Comm::EmailPreparation prepared = review->preparation;
        prepared.senderAddress = PN::Comm::EmailSettingsStore(settings).senderAddress();
        // See prepareMeetingNotesReportReviewWithOptions(): no PDF unless the
        // chosen mode needs one.
        prepared.createHtmlExport = true;
        self->stageRequestedGeneratedAttachmentAsync(std::move(prepared),
            [self, audience = review->audience, snapshot = result.snapshot, source = request.source, options,
             reviewValidation = review->validation]
            (std::optional<PN::Comm::EmailPreparation> staged, PN::Comm::ValidationResult validation) mutable {
                if (!self) return;
                if (!staged) { self->m_communicationsController->setReviewValidation(std::move(validation)); return; }
                self->m_recipientSelectionModel->setAudience(audience);
                self->m_projectReportReview = *staged;
                self->m_projectReportSnapshot = snapshot;
                self->m_projectReportSource = source;
                self->m_projectReportOptions = options;
                self->m_reviewAudienceSnapshot = snapshot;
                self->m_reviewAudienceRule = projectTeamAudienceRule();
                self->m_communicationsController->setPreparation(std::move(*staged));
                self->updateRecipientInternalReportContext();
                // Without a saved default the receives-status preselection
                // from the preparation factory stays in place.
                self->applyDefaultReviewAudiencePreset();
                validation.issues += reviewValidation.issues;
                if (!validation.issues.isEmpty())
                    self->m_communicationsController->setReviewValidation(std::move(validation));
            });
    });
    return true;
}

bool DesktopAppController::restoreProjectReportDefaultAudience()
{
    if (!m_projectReportSnapshot || !m_projectReportSource || !m_recipientSelectionModel || !m_communicationsController
        || m_communicationsController->busy())
        return false;
    if (m_projectReportSource->workflow != PN::Comm::Workflow::StatusReport
        && m_projectReportSource->workflow != PN::Comm::Workflow::TrackerItemsReport)
        return false;
    const PN::Comm::AudienceRule rule = projectTeamAudienceRule();
    PN::Comm::AudienceResolution audience = PN::Comm::resolveAudience(*m_projectReportSnapshot, rule);
    for (const PN::Comm::SnapshotContact &contact : audience.contacts)
        if (!contact.receivesStatus)
            audience.initialOverrides.append({contact.id, false, PN::Comm::RecipientRole::To});
    m_recipientSelectionModel->setAudience(std::move(audience));
    updateRecipientInternalReportContext();
    m_reviewAudienceRule = rule;
    setAppliedReviewAudiencePreset({});
    return true;
}

QVariantList DesktopAppController::reviewAudiencePresets() const
{
    QVariantList result;
    if (!m_reviewAudienceSnapshot || !m_communicationsController)
        return result;
    const PN::Comm::SourceContext source = activeReviewSource();
    const PN::Comm::AudiencePresetStore store;
    // Saved audiences belong to the project, so every report lists them all;
    // only the default is specific to the report being reviewed.
    for (const PN::Comm::AudiencePreset &preset : store.presets(source.projectId)) {
        result.append(QVariantMap{{QStringLiteral("id"), preset.id},
                                  {QStringLiteral("name"), preset.name},
                                  {QStringLiteral("reportDefault"), preset.defaultFor.contains(source.workflow)},
                                  {QStringLiteral("applied"), preset.id == m_reviewAudiencePresetId}});
    }
    return result;
}

bool DesktopAppController::saveReviewAudiencePreset(const QString &name)
{
    if (!m_reviewAudienceSnapshot || !m_reviewAudienceRule || !m_communicationsController)
        return false;
    const PN::Comm::SourceContext source = activeReviewSource();
    if (source.projectId.isEmpty() || name.trimmed().isEmpty())
        return false;
    PN::Comm::AudiencePreset preset;
    preset.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    preset.name = name.trimmed();
    preset.projectId = source.projectId;
    preset.rule = *m_reviewAudienceRule;
    preset.overrides = m_recipientSelectionModel->overrides();
    PN::Comm::AudiencePresetStore store;
    if (!store.save(preset).ok())
        return false;
    // The review now shows exactly the saved audience, so it becomes the applied one.
    for (const PN::Comm::AudiencePreset &saved : store.presets(source.projectId))
        if (saved.name.compare(preset.name, Qt::CaseInsensitive) == 0)
            setAppliedReviewAudiencePreset(saved.id);
    return true;
}

bool DesktopAppController::applyReviewAudiencePreset(const QString &presetId)
{
    if (!m_reviewAudienceSnapshot || !m_communicationsController || presetId.trimmed().isEmpty())
        return false;
    const PN::Comm::SourceContext source = activeReviewSource();
    const PN::Comm::AudiencePresetStore store;
    for (const PN::Comm::AudiencePreset &preset : store.presets(source.projectId)) {
        if (preset.id != presetId)
            continue;
        const auto rule = reviewPresetRule(source.workflow, *m_reviewAudienceSnapshot, preset);
        m_recipientSelectionModel->setAudience(PN::Comm::resolveAudience(*m_reviewAudienceSnapshot, rule));
        updateRecipientInternalReportContext();
        m_recipientSelectionModel->applyOverrides(preset.overrides);
        m_reviewAudienceRule = rule;
        setAppliedReviewAudiencePreset(preset.id);
        return true;
    }
    return false;
}

bool DesktopAppController::setReviewAudiencePresetDefault(const QString &presetId)
{
    if (!m_reviewAudienceSnapshot || !m_communicationsController || presetId.trimmed().isEmpty())
        return false;
    const PN::Comm::SourceContext source = activeReviewSource();
    return PN::Comm::AudiencePresetStore().setDefault(presetId, source.workflow, source.projectId).ok();
}

std::optional<PN::Comm::CommunicationTemplate>
DesktopAppController::resolveWorkflowTemplate(PN::Comm::Workflow workflow)
{
    if (!m_templateEditorModel)
        return {};
    // Each workflow has exactly one active template. Pointing the editor model
    // at the workflow makes its own selection rules (bundled default, or the
    // user's saved copy) pick the template, so there is nothing to choose here.
    m_templateEditorModel->setWorkflow(PN::Comm::toStableString(workflow));
    return m_templateEditorModel->templateById(m_templateEditorModel->selectedTemplateId());
}

bool DesktopAppController::applyDefaultReviewAudiencePreset()
{
    if (!m_reviewAudienceSnapshot || !m_communicationsController)
        return false;
    const PN::Comm::SourceContext source = activeReviewSource();
    const auto preset = PN::Comm::AudiencePresetStore()
        .defaultFor(source.workflow, source.projectId);
    if (!preset)
        return false;
    const auto rule = reviewPresetRule(source.workflow, *m_reviewAudienceSnapshot, *preset);
    m_recipientSelectionModel->setAudience(PN::Comm::resolveAudience(*m_reviewAudienceSnapshot, rule));
    updateRecipientInternalReportContext();
    m_recipientSelectionModel->applyOverrides(preset->overrides);
    m_reviewAudienceRule = rule;
    setAppliedReviewAudiencePreset(preset->id);
    return true;
}

void DesktopAppController::setAppliedReviewAudiencePreset(const QString &presetId)
{
    // Always signal: a new review may load the same default as the last one,
    // and the picker is cleared when the review is re-prepared.
    m_reviewAudiencePresetId = presetId;
    emit reviewAudiencePresetChanged();
}

void DesktopAppController::updateRecipientInternalReportContext()
{
    if (!m_recipientSelectionModel)
        return;
    bool internalReport = false;
    if (m_communicationsController && !m_meetingReview) {
        switch (m_communicationsController->preparation().source.workflow) {
        case PN::Comm::Workflow::MeetingNotesReport:
            internalReport = m_meetingNotesReportInternal;
            break;
        case PN::Comm::Workflow::StatusReport:
        case PN::Comm::Workflow::TrackerItemsReport:
            internalReport = m_projectReportOptions && m_projectReportOptions->internalReport;
            break;
        case PN::Comm::Workflow::SendMeetingNotes:
        case PN::Comm::Workflow::ScheduleMeeting:
        case PN::Comm::Workflow::ProjectEmail:
        case PN::Comm::Workflow::ShareLocation:
        case PN::Comm::Workflow::ContactEmail:
            break;
        }
    }
    m_recipientSelectionModel->setInternalReportContext(
        internalReport, m_reviewAudienceSnapshot ? m_reviewAudienceSnapshot->managingCompanyId : QString());
}

bool DesktopAppController::handoffPreparedReview()
{
    if (m_communicationsController->busy() || !m_databaseOpen || !m_communicationRepository
        || !m_reviewAudienceSnapshot || !m_pendingHandoffRevalidation.isNull())
        return false;
    const PN::Comm::EmailPreparation current = m_communicationsController->preparation();
    std::optional<PN::Comm::EmailPreparation> *review = nullptr;
    switch (current.source.workflow) {
    case PN::Comm::Workflow::SendMeetingNotes: review = &m_meetingNotesReview; break;
    case PN::Comm::Workflow::MeetingNotesReport: review = &m_meetingNotesReportReview; break;
    case PN::Comm::Workflow::StatusReport:
    case PN::Comm::Workflow::TrackerItemsReport: review = &m_projectReportReview; break;
    case PN::Comm::Workflow::ProjectEmail:
    case PN::Comm::Workflow::ContactEmail:
    case PN::Comm::Workflow::ShareLocation: review = &m_projectEmailReview; break;
    default: break;
    }
    if (!review || !review->has_value()) {
        PN::Comm::ValidationResult validation;
        validation.addError(QStringLiteral("report-review-required"), QStringLiteral("source"),
                            tr("Prepare the report again before creating the email draft."));
        m_communicationsController->setReviewValidation(std::move(validation));
        return false;
    }
    if (current.source.databaseGeneration != m_communicationDatabaseGeneration) {
        PN::Comm::ValidationResult validation;
        validation.addError(QStringLiteral("database-generation-changed"), QStringLiteral("source"),
                            tr("The selected database changed. Regenerate the review."));
        m_communicationsController->setReviewValidation(std::move(validation));
        return false;
    }
    if (!m_communicationsController->beginSourceRevalidation())
        return false;
    PN::Comm::SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.previewRevision = current.previewRevision;
    request.source = current.source;
    request.managingCompanyId = managingCompanyId();
    request.projectManagerId = projectManagerId();
    request.capturedAt = QDateTime::currentDateTimeUtc();
    const QByteArray expectedFingerprint = m_reviewAudienceSnapshot->fingerprint;
    m_pendingHandoffRevalidation = request.operationId;
    QPointer<DesktopAppController> self(this);
    m_communicationRepository->loadSnapshot(request, [self, request, review, expectedFingerprint](PN::Comm::SnapshotResult result) {
        if (!self || self->m_pendingHandoffRevalidation != result.operationId)
            return;
        self->m_pendingHandoffRevalidation = {};
        if (self->m_communicationsController->stage() != PN::Comm::PreparationStage::RevalidatingSource)
            return;
        if (!result.error.code.isEmpty()) {
            PN::Comm::ValidationResult validation;
            validation.addError(result.error.code, QStringLiteral("source"),
                                result.error.displayText.isEmpty() ? tr("The source could not be revalidated.")
                                                                   : result.error.displayText);
            self->m_communicationsController->failSourceRevalidation(std::move(validation));
            return;
        }
        if (result.snapshot.fingerprint != expectedFingerprint) {
            PN::Comm::ValidationResult validation;
            validation.addError(QStringLiteral("review-source-changed"), QStringLiteral("source"),
                                tr("The source data changed. Regenerate the review before handing it off."));
            self->m_communicationsController->failSourceRevalidation(std::move(validation));
            return;
        }
        PN::Comm::RecipientResolution recipients = self->m_recipientSelectionModel->resolve();
        // Plain contact emails are fixed to the contact clicked; project emails
        // use the review's editable audience, even when opened from a contact.
        if (request.source.workflow == PN::Comm::Workflow::ContactEmail) {
            recipients = {};
            recipients.recipients = (*review)->recipients;
        }
        if (!recipients.validation.ok()) {
            self->m_communicationsController->failSourceRevalidation(recipients.validation);
            return;
        }
        // The controller owns all review-local mutations (subject/body and
        // staged attachments); keep that value while freezing final recipients.
        PN::Comm::EmailPreparation prepared = self->m_communicationsController->preparation();
        prepared.recipients = recipients.recipients;
        prepared.addressLater = recipients.addressLaterExplicitlyChosen;
        // Thunderbird reads its compose body from a file. Freeze review edits
        // into that file as well as the HTML passed to the other backends.
        if (prepared.backend == PN::Comm::BackendId::Thunderbird && self->m_thunderbirdEmailBackend
            && prepared.mode == PN::Comm::EmailMode::InlineHtml) {
            PN::Comm::ArtifactStore store(PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile));
            const auto body = store.stageGeneratedContent(prepared.operationId, QStringLiteral("message.html"),
                prepared.document.emailFragment.toUtf8(), QStringLiteral("text/html"));
            if (!body.ok()) {
                PN::Comm::ValidationResult validation;
                validation.addError(body.error.code, QStringLiteral("body"), tr("The email body could not be prepared."));
                self->m_communicationsController->failSourceRevalidation(std::move(validation));
                return;
            }
            self->m_thunderbirdEmailBackend->setBodyFile(prepared.operationId, body.artifact.absolutePath,
                QDir(store.stagingRoot()).filePath(prepared.operationId.toString(QUuid::WithoutBraces)));
        }
        *review = prepared;
        self->m_communicationsController->handoffAfterSourceRevalidation(
            recipients.recipients, recipients.addressLaterExplicitlyChosen);
    });
    return true;
}

PN::Comm::SourceContext DesktopAppController::activeReviewSource() const
{
    if (m_meetingReview)
        return m_meetingReview->source;
    return m_communicationsController ? m_communicationsController->preparation().source
                                      : PN::Comm::SourceContext{};
}

QVariantList DesktopAppController::meetingTemplateChoices() const
{
    QVariantList result;
    if (!m_databaseOpen)
        return result;
    for (const PN::Comm::MeetingTemplate &meetingType : PN::Comm::MeetingTemplateStore().templates())
        result.append(QVariantMap{{QStringLiteral("id"), meetingType.id},
                                  {QStringLiteral("name"), meetingType.name},
                                  {QStringLiteral("durationMinutes"), meetingType.durationMinutes}});
    return result;
}

bool DesktopAppController::prepareMeetingReview(const QString &projectId, const QString &templateId)
{
    if (!m_databaseOpen || !m_communicationRepository || projectId.trimmed().isEmpty() || m_meetingHandoffBusy)
        return false;
    std::optional<PN::Comm::MeetingTemplate> meetingType;
    for (const PN::Comm::MeetingTemplate &candidate : PN::Comm::MeetingTemplateStore().templates())
        if (candidate.id == templateId)
            meetingType = candidate;
    if (!meetingType) {
        emit errorOccurred(tr("Cannot Schedule Meeting"),
                           tr("The meeting type no longer exists. Choose another in Settings > Meeting Templates."));
        return false;
    }

    cancelProjectEmailReview();
    PN::Comm::SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source = {global_DBObjects.getDatabaseFile(), m_communicationDatabaseGeneration,
                      projectId, {}, PN::Comm::Workflow::ScheduleMeeting};
    request.managingCompanyId = managingCompanyId();
    request.projectManagerId = projectManagerId();
    request.capturedAt = QDateTime::currentDateTimeUtc();
    m_pendingMeetingReview = request.operationId;
    m_meetingReview.reset();
    m_reviewAudienceSnapshot.reset();
    m_reviewAudienceRule.reset();
    setAppliedReviewAudiencePreset({});
    emit meetingReviewChanged();

    QPointer<DesktopAppController> self(this);
    m_communicationRepository->loadSnapshot(request, [self, request, meetingType = *meetingType](PN::Comm::SnapshotResult result) {
        if (!self || self->m_pendingMeetingReview != result.operationId)
            return;
        self->m_pendingMeetingReview = {};
        if (!result.error.code.isEmpty()) {
            emit self->errorOccurred(self->tr("Cannot Schedule Meeting"),
                                     result.error.displayText.isEmpty() ? result.error.code : result.error.displayText);
            return;
        }
        // Meetings follow the Email, Contacts, and Calendar integration preference.
        QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
        const PN::Comm::BackendId backend = PN::Comm::EmailSettingsStore(settings).preferredBackend();
        PN::Comm::ValidationResult validation;
        auto prepared = PN::Comm::MeetingPreparationFactory::create(
            result.snapshot, request.source, meetingType, backend, QDateTime::currentDateTime(), &validation);
        if (!prepared) {
            emit self->errorOccurred(self->tr("Cannot Schedule Meeting"),
                                     self->tr("The meeting type \"%1\" could not be applied to this project. "
                                              "Check its subject and agenda fields in Settings > Meeting Templates.")
                                         .arg(meetingType.name));
            return;
        }
        // Falls back to the configured sender address only when the project has
        // no Project Manager email to use as organizer (see
        // MeetingPreparationFactory::preparation, which already prefers that).
        if (prepared->organizerAddress.trimmed().isEmpty())
            prepared->organizerAddress = PN::Comm::EmailSettingsStore(settings).senderAddress();
        // A meeting starts from the same project-team audience as the reports,
        // then loads the audience saved as the meeting default, if any.
        const PN::Comm::AudienceRule rule = projectTeamAudienceRule();
        self->m_meetingReview = *prepared;
        self->m_reviewAudienceSnapshot = result.snapshot;
        self->m_reviewAudienceRule = rule;
        self->m_recipientSelectionModel->setAudience(PN::Comm::resolveAudience(result.snapshot, rule));
        self->updateRecipientInternalReportContext();
        // The attendee pane lists saved audiences when this signal arrives, so
        // send it even when there is no meeting default to load.
        if (!self->applyDefaultReviewAudiencePreset())
            self->setAppliedReviewAudiencePreset({});
        emit self->meetingReviewChanged();
    });
    return true;
}

QVariantMap DesktopAppController::meetingReview() const
{
    if (!m_meetingReview)
        return {{QStringLiteral("ready"), false}, {QStringLiteral("loading"), !m_pendingMeetingReview.isNull()}};
    const PN::Comm::MeetingPreparation &review = *m_meetingReview;
    const QDateTime start = review.start.toLocalTime();
    QString backendLabel;
    switch (review.backend) {
    case PN::Comm::BackendId::Graph: backendLabel = tr("Microsoft 365"); break;
    case PN::Comm::BackendId::Thunderbird: backendLabel = tr("Thunderbird"); break;
    case PN::Comm::BackendId::DefaultApp: backendLabel = tr("Default application"); break;
    case PN::Comm::BackendId::Outlook: backendLabel = tr("Outlook"); break;
    case PN::Comm::BackendId::OutlookClassic: backendLabel = tr("Outlook (Classic)"); break;
    case PN::Comm::BackendId::ICloud: backendLabel = tr("iCloud"); break;
    default: backendLabel = PN::Comm::toStableString(review.backend); break;
    }
    const QString agenda = !review.htmlAgenda.isEmpty() ? review.htmlAgenda
                                                         : review.plainAgenda.toHtmlEscaped().replace(u'\n', QStringLiteral("<br>"));
    return {{QStringLiteral("ready"), true},
            {QStringLiteral("loading"), false},
            {QStringLiteral("templateName"), review.templateName},
            {QStringLiteral("subject"), review.subject},
            {QStringLiteral("agendaHtml"), agenda},
            {QStringLiteral("location"), review.location},
            {QStringLiteral("date"), start.date().toString(QStringLiteral("MM/dd/yyyy"))},
            {QStringLiteral("time"), start.time().toString(QStringLiteral("h:mm AP"))},
            {QStringLiteral("durationMinutes"), review.durationMinutes},
            {QStringLiteral("timeZone"), start.timeZoneAbbreviation()},
            {QStringLiteral("backend"), PN::Comm::toStableString(review.backend)},
            {QStringLiteral("backendLabel"), backendLabel},
            {QStringLiteral("organizer"), review.organizerAddress.isEmpty()
                                              ? review.organizerName
                                              : QStringLiteral("%1 <%2>").arg(review.organizerName, review.organizerAddress)}};
}

bool DesktopAppController::handoffPreparedMeeting(const QString &subject, const QString &agendaHtml,
                                                  const QString &location, const QString &date,
                                                  const QString &time, int durationMinutes)
{
    if (m_meetingHandoffBusy || !m_meetingReview || !m_reviewAudienceSnapshot
        || !m_communicationRepository || !m_databaseOpen)
        return false;
    const auto reject = [this](const QString &message) {
        emit meetingHandoffFinished(false, message);
        return false;
    };
    const QDate day = QDate::fromString(date.trimmed(), QStringLiteral("MM/dd/yyyy"));
    // The review shows 12-hour time; a typed 24-hour time is accepted too.
    QTime startTime;
    const QString typedTime = time.trimmed().toUpper();
    for (const QString &format : {QStringLiteral("h:mm AP"), QStringLiteral("h:mmAP"), QStringLiteral("h AP"),
                                  QStringLiteral("hAP"), QStringLiteral("H:mm")}) {
        startTime = QTime::fromString(typedTime, format);
        if (startTime.isValid())
            break;
    }
    if (!day.isValid() || !startTime.isValid())
        return reject(tr("Choose a valid date and start time, such as 9:00 AM."));
    if (durationMinutes <= 0 || durationMinutes > 24 * 60)
        return reject(tr("Choose a duration between 1 minute and 24 hours."));
    if (subject.trimmed().isEmpty())
        return reject(meetingErrorText(QStringLiteral("meeting-subject-required")));
    if (m_meetingReview->source.databaseGeneration != m_communicationDatabaseGeneration)
        return reject(tr("The selected database changed. Close this review and schedule the meeting again."));

    PN::Comm::MeetingPreparation &review = *m_meetingReview;
    review.subject = subject.simplified();
    review.location = location.simplified();
    review.start = QDateTime(day, startTime);
    review.durationMinutes = durationMinutes;
    QTextDocument agenda;
    agenda.setHtml(agendaHtml);
    review.plainAgenda = agenda.toPlainText().trimmed();
    review.htmlAgenda = review.plainAgenda.isEmpty() ? QString() : agendaHtml;
    // Every attempt is a new operation: a retried handoff must not collide
    // with the staged files, or the provider event, of an earlier attempt.
    review.operationId = QUuid::createUuid();
    if (review.backend == PN::Comm::BackendId::Graph) {
        const PN::Comm::Office365Account account = m_office365Service ? m_office365Service->account()
                                                                      : PN::Comm::Office365Account{};
        if (!account.authenticated)
            return reject(meetingErrorText(QStringLiteral("office365-not-authenticated")));
        review.accountKey = account.label;
        review.accountGeneration = account.generation;
    }

    PN::Comm::SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source = review.source;
    request.managingCompanyId = managingCompanyId();
    request.projectManagerId = projectManagerId();
    request.capturedAt = QDateTime::currentDateTimeUtc();
    const QByteArray expectedFingerprint = m_reviewAudienceSnapshot->fingerprint;
    m_pendingMeetingRevalidation = request.operationId;
    m_meetingHandoffBusy = true;
    emit meetingReviewChanged();

    QPointer<DesktopAppController> self(this);
    m_communicationRepository->loadSnapshot(request, [self, expectedFingerprint](PN::Comm::SnapshotResult result) {
        if (!self || self->m_pendingMeetingRevalidation != result.operationId)
            return;
        self->m_pendingMeetingRevalidation = {};
        PN::Comm::MeetingHandoffResult failure;
        if (!self->m_meetingReview) {
            failure.error.code = QStringLiteral("operation-cancelled");
            self->finishMeetingHandoff(failure);
            return;
        }
        failure.operationId = self->m_meetingReview->operationId;
        if (!result.error.code.isEmpty()) {
            failure.error = {result.error.code, result.error.displayText};
            self->finishMeetingHandoff(failure);
            return;
        }
        if (result.snapshot.fingerprint != expectedFingerprint) {
            failure.error.code = QStringLiteral("review-source-changed");
            self->finishMeetingHandoff(failure);
            return;
        }
        const PN::Comm::RecipientResolution recipients = self->m_recipientSelectionModel->resolve();
        if (!recipients.validation.ok()) {
            for (const PN::Comm::ValidationIssue &issue : recipients.validation.issues)
                if (issue.severity == PN::Comm::IssueSeverity::Error) {
                    failure.error.code = issue.code;
                    break;
                }
            self->finishMeetingHandoff(failure);
            return;
        }
        const PN::Comm::MeetingRequest meeting =
            PN::Comm::MeetingPreparationFactory::request(*self->m_meetingReview, recipients.recipients);
        const PN::Comm::BackendId backend = meeting.backend;
        self->m_meetingService->handoff(meeting, [self, backend](PN::Comm::MeetingHandoffResult outcome) {
            if (!self)
                return;
            self->finishMeetingHandoff(outcome);
#if defined(Q_OS_DARWIN)
            // MacCalendarBackend creates the event directly in Calendar rather
            // than opening a file, so nothing else confirms it happened; the
            // calendar is also chosen automatically and may not be the one the
            // user was last looking at, so name it explicitly rather than
            // leaving the dialog to just close silently.
            if (backend == PN::Comm::BackendId::MacMail && outcome.error.code.isEmpty()
                && !outcome.eventIdentity.isEmpty()) {
                emit self->infoOccurred(self->tr("Schedule Meeting"),
                    self->tr("The event was created on your \"%1\" calendar. Paste the copied attendees into "
                             "the event's invitees, then click Send in Calendar.").arg(outcome.eventIdentity));
            }
#endif
            // The iCloud review stays open after success because CalDAV does
            // not provide a link to the newly created event.  Keeping the
            // review visible preserves Copy Attendees without adding a second,
            // redundant confirmation dialog here.
        });
    });
    return true;
}

void DesktopAppController::finishMeetingHandoff(const PN::Comm::MeetingHandoffResult &result)
{
    m_meetingHandoffBusy = false;
    emit meetingReviewChanged();
    if (!result.error.code.isEmpty()) {
        const bool unknownOutcome = result.certainty == PN::Comm::OutcomeCertainty::Uncertain
            || result.error.certainty == PN::Comm::OutcomeCertainty::Uncertain;
        const bool iCloud = m_meetingReview && m_meetingReview->backend == PN::Comm::BackendId::ICloud;
        emit meetingHandoffFinished(false, unknownOutcome
            ? (iCloud ? tr("iCloud may have created the event before the connection failed. "
                           "Check iCloud Calendar before trying again.")
                      : tr("Microsoft 365 may have created the meeting before the connection failed. "
                           "Check your Outlook calendar before trying again."))
            : meetingErrorText(result.error.code, result.error.displayText));
        return;
    }
#ifdef QT_DEBUG
    if (!result.presentationUrl.isEmpty() || !result.providerWebLink.isEmpty())
        QLog_Debug(DEBUGLOG, QStringLiteral("Schedule Meeting: event id %1; provider webLink %2; opening %3")
                                 .arg(result.eventIdentity,
                                      result.providerWebLink.toString(QUrl::FullyEncoded),
                                      result.presentationUrl.toString(QUrl::FullyEncoded)));
#endif
    // The invite opens in the calendar app without a confirmation box; only a
    // draft the browser could not open needs telling.
    if (result.presentationUrl.isValid() && !QDesktopServices::openUrl(result.presentationUrl)) {
        const QString message = tr("The draft meeting was created in your Microsoft 365 calendar, but the browser "
                                   "could not open it. Open your Outlook calendar to review it and send the invitations.");
        emit meetingHandoffFinished(true, message);
        emit infoOccurred(tr("Schedule Meeting"), message);
        return;
    }
    emit meetingHandoffFinished(true, QString());
}

int DesktopAppController::copyMeetingAttendees() const
{
    if (!m_meetingReview || !m_recipientSelectionModel)
        return 0;
    const PN::Comm::MeetingRequest meeting =
        PN::Comm::MeetingPreparationFactory::request(*m_meetingReview, m_recipientSelectionModel->resolve().recipients);
    QStringList entries;
    for (const PN::Comm::MeetingAttendee &attendee : meeting.attendees)
        entries << (attendee.displayName.isEmpty() ? attendee.address
                                                   : QStringLiteral("%1 <%2>").arg(attendee.displayName, attendee.address));
    if (!entries.isEmpty())
        copyTextToClipboard(entries.join(QStringLiteral(", ")));
    return int(entries.size());
}

void DesktopAppController::closeMeetingReview()
{
    if (m_communicationRepository && !m_pendingMeetingReview.isNull())
        m_communicationRepository->cancel(m_pendingMeetingReview);
    m_pendingMeetingReview = {};
    if (!m_pendingMeetingRevalidation.isNull()) {
        if (m_communicationRepository)
            m_communicationRepository->cancel(m_pendingMeetingRevalidation);
        m_pendingMeetingRevalidation = {};
        m_meetingHandoffBusy = false;
    }
    // A handoff already with its backend finishes on its own and reports back.
    m_meetingReview.reset();
    emit meetingReviewChanged();
}

QString DesktopAppController::meetingErrorText(const QString &code, const QString &fallback) const
{
    static const QHash<QString, const char *> messages = {
        {QStringLiteral("meeting-subject-required"), QT_TR_NOOP("Enter a meeting subject.")},
        {QStringLiteral("meeting-subject-invalid"), QT_TR_NOOP("The subject cannot contain line breaks.")},
        {QStringLiteral("meeting-location-invalid"), QT_TR_NOOP("The location cannot contain line breaks.")},
        {QStringLiteral("meeting-time-required"), QT_TR_NOOP("Choose a valid start time and duration.")},
        {QStringLiteral("meeting-end-before-start"), QT_TR_NOOP("Choose a valid start time and duration.")},
        {QStringLiteral("recipient-address-invalid"), QT_TR_NOOP("One of the selected attendees has an invalid email address. Correct it on the Contacts page or clear that attendee.")},
        {QStringLiteral("recipient-role-conflict"), QT_TR_NOOP("The same email address is selected more than once with different roles.")},
        {QStringLiteral("organizer-address-invalid"), QT_TR_NOOP("The project manager's email address is not valid. Correct it on the Contacts page.")},
        {QStringLiteral("review-source-changed"), QT_TR_NOOP("The project data changed. Close this review and schedule the meeting again.")},
        {QStringLiteral("meeting-operation-busy"), QT_TR_NOOP("Another meeting is still being scheduled.")},
        {QStringLiteral("meeting-backend-unavailable"), QT_TR_NOOP("The selected integration cannot schedule meetings. Choose another in Settings > Email, Contacts, and Calendar.")},
        {QStringLiteral("thunderbird-unavailable"), QT_TR_NOOP("Thunderbird was not found. Check its path in Settings > Email, Contacts, and Calendar.")},
        {QStringLiteral("thunderbird-launch-failed"), QT_TR_NOOP("Thunderbird could not be started.")},
        {QStringLiteral("outlook-open-failed"), QT_TR_NOOP("The meeting file was created, but the new Outlook did not open it. Check that the new Outlook is installed, or choose another integration in Settings > Email, Contacts, and Calendar.")},
        {QStringLiteral("outlook-classic-unavailable"), QT_TR_NOOP("Outlook (classic) was not found. Check that it is installed, or choose another integration in Settings > Email, Contacts, and Calendar.")},
        {QStringLiteral("outlook-classic-failed"), QT_TR_NOOP("Outlook (classic) could not create the meeting. Close any Outlook dialog that is waiting for an answer, then try again.")},
        {QStringLiteral("calendar-open-failed"), QT_TR_NOOP("The meeting file was created, but no calendar app opened it. Set a default app for .ics files, or choose another integration in Settings > Email, Contacts, and Calendar.")},
        {QStringLiteral("office365-not-authenticated"), QT_TR_NOOP("Sign in to Microsoft 365 in Settings > Office 365 Integration, then try again.")},
        {QStringLiteral("account-generation-changed"), QT_TR_NOOP("The Microsoft 365 account changed. Try again.")},
        {QStringLiteral("graph-calendar-consent-required"), QT_TR_NOOP("Microsoft 365 did not allow calendar access. Sign out and sign in again in Settings > Office 365 Integration to grant calendar permission.")},
        {QStringLiteral("icloud-credentials-required"), QT_TR_NOOP("Enter your Apple Account and save an app-specific password in Settings > Email, Contacts, and Calendar, then try again.")},
        {QStringLiteral("icloud-calendar-authentication-failed"), QT_TR_NOOP("iCloud rejected the Apple Account or app-specific password. Check both in Settings > Email, Contacts, and Calendar.")},
        {QStringLiteral("icloud-calendar-not-found"), QT_TR_NOOP("No writable iCloud calendar was found for this account.")},
        {QStringLiteral("icloud-calendar-discovery-failed"), QT_TR_NOOP("Project Notes could not find your iCloud calendars. Check your connection and try again.")},
        {QStringLiteral("icloud-calendar-event-create-failed"), QT_TR_NOOP("iCloud could not create the event. Check your connection and try again.")},
        {QStringLiteral("operation-cancelled"), QT_TR_NOOP("Scheduling was cancelled.")},
    };
    const auto message = messages.constFind(code);
    if (message != messages.cend())
        return tr(*message);
    if (!fallback.isEmpty())
        return fallback;
    return tr("The meeting could not be scheduled (%1).").arg(code);
}

bool DesktopAppController::saveReviewGeneratedAttachment(const QString &displayName,
                                                         const QString &destination)
{
    if (!m_communicationsController || m_communicationsController->busy()
        || displayName.trimmed().isEmpty())
        return false;
    const QUrl destinationUrl(destination);
    const QString localDestination = destinationUrl.isLocalFile() ? destinationUrl.toLocalFile() : destination;
    const auto &preparation = m_communicationsController->preparation();
    const PN::Comm::Artifact *artifactToSave = nullptr;
    const auto attachment = std::find_if(preparation.attachments.cbegin(), preparation.attachments.cend(), [&displayName](const PN::Comm::Artifact &artifact) {
        return artifact.generatedByApp && artifact.displayName == displayName;
    });
    if (attachment != preparation.attachments.cend()) {
        artifactToSave = &*attachment;
    } else {
        const auto exported = std::find_if(preparation.exports.cbegin(), preparation.exports.cend(), [&displayName](const PN::Comm::Artifact &artifact) {
            return artifact.generatedByApp && artifact.displayName == displayName;
        });
        if (exported != preparation.exports.cend())
            artifactToSave = &*exported;
    }
    if (!artifactToSave)
        return false;
    PN::Comm::ServiceError error;
    if (PN::Comm::ArtifactStore(PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile)).publish(*artifactToSave, localDestination, &error))
        return true;
    PN::Comm::ValidationResult validation;
    validation.addError(error.code.isEmpty() ? QStringLiteral("artifact-publish-failed") : error.code,
                        QStringLiteral("attachments"), tr("The generated attachment could not be saved."));
    m_communicationsController->setReviewValidation(std::move(validation));
    return false;
}

bool DesktopAppController::removeExistingReportSaveFile(const QString &destination)
{
    const QUrl destinationUrl(destination);
    const QString localDestination = destinationUrl.isLocalFile() ? destinationUrl.toLocalFile() : destination;
    const QFileInfo existing(localDestination);
    if (!existing.exists())
        return true;
    if (!existing.isFile() || existing.isSymLink())
        return false;
    if (QFile::remove(existing.absoluteFilePath()))
        return true;

    PN::Comm::ValidationResult validation;
    validation.addError(QStringLiteral("destination-remove-failed"), QStringLiteral("destination"),
                        tr("The existing report file could not be replaced."));
    m_communicationsController->setReviewValidation(std::move(validation));
    return false;
}

QAbstractItemModel* DesktopAppController::projectsListModel() const
{ return global_DBObjects.projectinformationmodelproxy(); }
QAbstractItemModel* DesktopAppController::projectNotesModel() const
{ return global_DBObjects.projectnotesmodelproxy(); }
QAbstractItemModel* DesktopAppController::meetingAttendeesModel() const
{ return global_DBObjects.meetingattendeesmodelproxy(); }
QAbstractItemModel* DesktopAppController::notesActionItemsModel() const
{ return global_DBObjects.notesactionitemsmodelproxy(); }
QAbstractItemModel* DesktopAppController::contactsModel() const
{ return global_DBObjects.contactsmodelproxy(); }
QAbstractItemModel* DesktopAppController::companiesModel() const
{ return global_DBObjects.companiesmodelproxy(); }
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
    global_DBObjects.contactsmodel()->refresh();
    global_DBObjects.companiesmodel()->refresh();
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

bool DesktopAppController::viewReportAfterSave() const
{
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    settings.setFallbacksEnabled(false);
    return settings.value(QStringLiteral("Reports/ViewAfterSave"), true).toBool();
}

void DesktopAppController::setViewReportAfterSave(bool enabled)
{
    if (viewReportAfterSave() == enabled)
        return;
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    settings.setFallbacksEnabled(false);
    settings.setValue(QStringLiteral("Reports/ViewAfterSave"), enabled);
    emit viewReportAfterSaveChanged();
}

QString DesktopAppController::managingCompanyId() const
{ return global_DBObjects.getManagingCompany(); }
void DesktopAppController::setManagingCompanyId(const QString& companyId)
{ global_DBObjects.setManagingCompany(companyId); }
QString DesktopAppController::projectManagerId() const
{ return global_DBObjects.getProjectManager(); }
void DesktopAppController::setProjectManagerId(const QString& contactId)
{
    global_DBObjects.setProjectManager(contactId);
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

    const QString contactId = global_DBObjects.getProjectManager();
    if (contactId.isEmpty())
        return QString();

    QString name;
    auto* proxy = global_DBObjects.contactsmodelproxy();
    if (proxy) {
        for (int row = 0; row < proxy->rowCount(); ++row) {
            if (proxy->data(proxy->index(row, 0)).toString() == contactId) {
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

bool DesktopAppController::showProjectSidebar() const
{ return global_DBObjects.getProjectSidebarVisible(); }
void DesktopAppController::setShowProjectSidebar(bool v)
{
    if (showProjectSidebar() == v)
        return;
    global_DBObjects.setProjectSidebarVisible(v);
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

QVariantList DesktopAppController::filterColumns(QAbstractItemModel* model) const
{
    return PN::ModelViewState::filterColumns(model);
}

QVariantList DesktopAppController::columnDistinctValues(QAbstractItemModel* model,
                                                       const QString& field) const
{
    return PN::ModelViewState::columnDistinctValues(model, field);
}

void DesktopAppController::applyColumnFilters(QAbstractItemModel* model,
                                              const QVariantList& specs)
{
    if (!sourceModelOf(model)) return;
    PN::ModelViewState::applyColumnFilters(model, specs);
    ++m_filterRev;
    emit filterRevChanged();
}

void DesktopAppController::clearColumnFilters(QAbstractItemModel* model)
{
    if (!sourceModelOf(model)) return;
    PN::ModelViewState::clearColumnFilters(model);
    ++m_filterRev;
    emit filterRevChanged();
}

QVariantList DesktopAppController::activeColumnFilters(QAbstractItemModel* model)
{
    return PN::ModelViewState::activeColumnFilters(model);
}

void DesktopAppController::applyQuickFilter(QAbstractItemModel* model, const QString& field,
                                            const QVariantList& values)
{
    if (!sourceModelOf(model)) return;
    PN::ModelViewState::applyQuickFilter(model, field, values);
    ++m_filterRev;
    emit filterRevChanged();
}

bool DesktopAppController::hasActiveColumnFilters(QAbstractItemModel* model) const
{
    return PN::ModelViewState::hasActiveColumnFilters(model);
}

QAbstractItemModel* DesktopAppController::modelForSection(const QString& section) const
{
    if (section == "projects")     return projectsListModel();
    if (section == "items")        return allItemsModel();
    if (section == "contacts")     return contactsModel();
    if (section == "companies")    return companiesModel();
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
    return PN::ModelViewState::sortColumns(model);
}

void DesktopAppController::applySort(QAbstractItemModel* model, const QString& field, bool descending)
{
    auto* src = sourceModelOf(model);
    if (!src || src->getColumnNumber(field) < 0) return;
    PN::ModelViewState::applySort(model, field, descending);
    ++m_sortRev;
    emit sortRevChanged();
}

void DesktopAppController::clearSort(QAbstractItemModel* model)
{
    if (!sourceModelOf(model)) return;
    PN::ModelViewState::clearSort(model);
    ++m_sortRev;
    emit sortRevChanged();
}

QVariantMap DesktopAppController::activeSort(QAbstractItemModel* model) const
{
    return PN::ModelViewState::activeSort(model);
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

// ── Plugin menus ─────────────────────────────────────────────────────────────

QVariantList DesktopAppController::pluginMenusForModel(QAbstractItemModel* model)
{
    SqlQueryModel* src = sourceModelOf(model);
    return pluginMenusForTable(src ? src->tablename() : QString());
}

QVariantList DesktopAppController::pluginMenusForTable(const QString& table)
{
    QVariantList out;
    m_pluginMenuCache.clear();
    if (table.isEmpty())
        return out;
    appendJsRecordMenus(table, &out);
    return out;
}

void DesktopAppController::appendJsRecordMenus(const QString& table, QVariantList* out)
{
    if (!m_jsPlugins)
        return;
    const QString entity = PN::Plugins::PublishedSchema::instance().publishedEntity(table);
    for (const QVariant& m : m_jsPlugins->menusForEntity(entity)) {
        const QVariantMap menu = m.toMap();
        PluginMenuRef ref;
        ref.jsPluginId = menu.value(QStringLiteral("pluginId")).toString();
        ref.jsMenuId = menu.value(QStringLiteral("menuId")).toString();
        m_pluginMenuCache.append(ref);
        out->append(QVariantMap{{QStringLiteral("title"), menu.value(QStringLiteral("title"))},
                                {QStringLiteral("submenu"), menu.value(QStringLiteral("submenu"))},
                                {QStringLiteral("index"), m_pluginMenuCache.size() - 1}});
    }
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
    if (m_jsPlugins)
        m_jsPlugins->runMenu(ref.jsPluginId, ref.jsMenuId,
                             PN::Plugins::PublishedSchema::instance().publishedEntity(table), recordId);
}

QVariantList DesktopAppController::globalPluginMenus()
{
    QVariantList out;
    m_globalPluginMenuCache.clear();
    if (m_jsPlugins) {
        for (const QVariant& m : m_jsPlugins->globalMenus()) {
            const QVariantMap menu = m.toMap();
            PluginMenuRef ref;
            ref.jsPluginId = menu.value(QStringLiteral("pluginId")).toString();
            ref.jsMenuId = menu.value(QStringLiteral("menuId")).toString();
            m_globalPluginMenuCache.append(ref);
            out.append(QVariantMap{{QStringLiteral("title"), menu.value(QStringLiteral("title"))},
                                   {QStringLiteral("submenu"), menu.value(QStringLiteral("submenu"))},
                                   {QStringLiteral("index"), m_globalPluginMenuCache.size() - 1}});
        }
    }
    return out;
}

void DesktopAppController::runGlobalPluginMenu(int index)
{
    if (index < 0 || index >= m_globalPluginMenuCache.size())
        return;
    const PluginMenuRef& ref = m_globalPluginMenuCache.at(index);
    if (m_jsPlugins)
        m_jsPlugins->runMenu(ref.jsPluginId, ref.jsMenuId, {}, {});
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

QString DesktopAppController::companyNameForId(const QString& companyId) const
{
    auto* src = global_DBObjects.companiesmodel();
    if (!src || companyId.isEmpty()) return {};
    QVariant key(companyId);
    return src->findValue(key, 0, 1).toString();  // col 1 = client_name
}

int DesktopAppController::companyRowForId(const QString& companyId) const
{
    auto* proxy = global_DBObjects.companiesmodelproxy();
    auto* src   = global_DBObjects.companiesmodel();
    if (!proxy || !src || companyId.isEmpty()) return -1;
    QVariant key(companyId);
    const QModelIndex srcIdx = src->findIndex(key, 0);
    if (!srcIdx.isValid()) return -1;
    const QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
    return proxyIdx.isValid() ? proxyIdx.row() : -1;
}

QString DesktopAppController::companyIdAtRow(int row) const
{
    auto* proxy = global_DBObjects.companiesmodelproxy();
    if (!proxy || row < 0 || row >= proxy->rowCount()) return {};
    return proxy->data(proxy->index(row, 0)).toString();
}

int DesktopAppController::contactsRowForId(const QString& contactId) const
{
    auto* proxy = global_DBObjects.contactsmodelproxy();
    auto* src   = global_DBObjects.contactsmodel();
    if (!proxy || !src || contactId.isEmpty()) return -1;
    QVariant key(contactId);
    const QModelIndex srcIdx = src->findIndex(key, 0);
    if (!srcIdx.isValid()) return -1;
    const QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
    return proxyIdx.isValid() ? proxyIdx.row() : -1;
}

QString DesktopAppController::contactNameForId(const QString& contactId) const
{
    auto* src = global_DBObjects.contactsmodel();
    if (!src || contactId.isEmpty()) return {};
    QVariant key(contactId);
    return src->findValue(key, 0, 1).toString();  // col 1 = name
}

// ── Picker lists ─────────────────────────────────────────────────────────────

// Sort a picker list (list of {id, name} maps) alphabetically by name. The
// source proxies carry whatever sort the user last applied on the Contacts /
// Companies list pages, so every combo that feeds off these lists sorts its own
// drop-down here rather than inheriting that order.
static void sortByName(QVariantList& list)
{
    std::sort(list.begin(), list.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("name").toString().localeAwareCompare(
                   b.toMap().value("name").toString()) < 0;
    });
}

QVariantList DesktopAppController::companyList() const
{
    QVariantList out;
    auto* proxy = global_DBObjects.companiesmodelproxy();
    if (!proxy) return out;
    for (int row = 0; row < proxy->rowCount(); ++row) {
        QVariantMap m;
        m.insert("id",   proxy->data(proxy->index(row, 0)).toString());
        m.insert("name", proxy->data(proxy->index(row, 1)).toString());
        out.append(m);
    }
    sortByName(out);
    return out;
}

QVariantList DesktopAppController::contactList() const
{
    QVariantList out;
    auto* proxy = global_DBObjects.contactsmodelproxy();
    if (!proxy) return out;
    for (int row = 0; row < proxy->rowCount(); ++row) {
        QVariantMap m;
        m.insert("id",   proxy->data(proxy->index(row, 0)).toString());
        m.insert("name", proxy->data(proxy->index(row, 1)).toString());
        out.append(m);
    }
    sortByName(out);
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

    // The SQL already returns the team alphabetically; re-sort so any appended
    // includeIds (people no longer on the team) land in order too.
    sortByName(out);
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
        // Safe to call while DB_LOCK is held: contactNameForId() reads from the
        // contacts model's in-memory cache, it doesn't run its own query.
        m.insert("by",   contactNameForId(qry.value(1).toString()));
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
// Same shape as addContact()/addCompany(); Main.qml discards the staged row again
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

bool DesktopAppController::isProjectTeamMember(const QString& projectId, const QString& contactId) const
{
    if (projectId.isEmpty() || contactId.isEmpty()) return true;
    DB_LOCK;
    QSqlQuery q(global_DBObjects.getDb());
    q.prepare("SELECT count(*) FROM project_people "
              "WHERE project_id = ? AND people_id = ? AND deleted = 0");
    q.addBindValue(projectId);
    q.addBindValue(contactId);
    q.exec();
    const bool exists = q.next() && q.value(0).toInt() > 0;
    DB_UNLOCK;
    return exists;
}

bool DesktopAppController::addContactToProjectTeam(const QString& projectId, const QString& contactId)
{
    if (projectId.isEmpty() || contactId.isEmpty()) return true;
    if (isProjectTeamMember(projectId, contactId)) return true;

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
    return applyRowFields(src, srcIdx.row(), { {2, contactId}, {4, "0"}, {5, ""} });
}

bool DesktopAppController::saveProject(int row,
        const QString& projectNumber, const QString& projectName,
        const QString& projectStatus, const QString& primaryContactId,
        const QString& companyId, const QString& lastStatusDate,
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
                  {13, companyId},       {14, projectStatus} });

    if (!applyRowFields(proxy, pIdx.row(), {
            { 1, projectNumber},   { 2, projectName},        { 3, lastStatusDate},
            { 4, lastInvoiceDate}, { 5, primaryContactId},   { 6, budget},
            { 7, actual},          { 8, bcwp},               { 9, bcws},
            {10, bac},             {11, invoicingPeriod},    {12, statusReportPeriod},
            {13, companyId},       {14, projectStatus} }))
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

QVariantMap DesktopAppController::noteLocationForId(const QString& noteId,
                                                   const QString& projectId)
{
    QVariantMap out;
    out.insert("row", -1);
    out.insert("projectId", QString());
    if (noteId.isEmpty())
        return out;

    // The Project Notes search row carries the project id; the Meeting Attendees
    // row only carries the note id, so fall back to a direct lookup (ids are
    // UUIDs, but escape the quote defensively — mirrors mainwindow.cpp).
    QString resolvedProjectId = projectId;
    if (resolvedProjectId.isEmpty())
    {
        QString safeId = noteId;
        safeId.replace('\'', QStringLiteral("''"));
        resolvedProjectId = global_DBObjects.execute(
            QStringLiteral("SELECT project_id FROM project_notes WHERE id = '%1'").arg(safeId));
    }
    if (resolvedProjectId.isEmpty())
        return out;

    out.insert("projectId", resolvedProjectId);

    // Load this project's notes so the detail page's getProjectNoteData(row) hits.
    setProjectFilter(resolvedProjectId);

    auto* proxy = global_DBObjects.projectnotesmodelproxy();
    auto* src   = global_DBObjects.projectnotesmodel();
    if (proxy && src)
    {
        QVariant key(noteId);
        const QModelIndex srcIdx = src->findIndex(key, 0);
        if (srcIdx.isValid())
        {
            const QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
            if (proxyIdx.isValid())
                out.insert("row", proxyIdx.row());
        }
    }
    return out;
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

bool DesktopAppController::saveAttendee(int row, const QString& contactId)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.meetingattendeesmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;

    const QString safeNote   = model->data(model->index(row, 1)).toString().replace("'", "''");
    const QString safeId     = model->data(model->index(row, 0)).toString().replace("'", "''");
    const QString safeContact = QString(contactId).replace("'", "''");
    const QString sql = QString(
        "SELECT COUNT(*) FROM meeting_attendees "
        "WHERE note_id = '%1' AND person_id = '%2' AND id != '%3' AND deleted = 0"
    ).arg(safeNote, safeContact, safeId);
    if (global_DBObjects.execute(sql).toInt() > 0) {
        const QString msg = tr("Attendee already exists.");
        global_DBObjects.setLastSaveError(msg);
        emit errorOccurred(tr("Could Not Save"), msg);
        return false;
    }

    const bool ok = model->setData(model->index(row, 2), contactId);
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
        const QString& dateIdentified, const QString& dateDue, const QString& description,
        const QString& lastUpdate, const QString& dateResolved)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.notesactionitemsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    // notesActionItemsModel columns: 2 item_type, 3 item_name, 4 identified_by,
    // 5 date_identified, 6 description, 7 assigned_to, 8 priority, 9 status,
    // 10 date_due, 11 last_update, 12 date_resolved (see notesactionitemsmodel.cpp
    // SELECT order). NotesActionItemsModel::setData has no auto date handling, so
    // last_update / date_resolved are whatever the editor holds.
    return applyRowFields(model, row, {
        {3,  itemName},      {2, itemType},   {8,  priority},
        {9,  status},        {7, assignedTo}, {4,  identifiedBy},
        {5,  dateIdentified}, {10, dateDue},  {6,  description},
        {11, lastUpdate},    {12, dateResolved} });
}

// ── Contacts ─────────────────────────────────────────────────────────────────

int DesktopAppController::addContact()
{
    return proxyRowFromSource(global_DBObjects.contactsmodelproxy(),
                              global_DBObjects.contactsmodel()->newRecord());
}

bool DesktopAppController::deleteContact(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.contactsmodelproxy(),
                       global_DBObjects.contactsmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getContactData(int row) const
{ return proxyRowToMap(global_DBObjects.contactsmodelproxy(), row); }

QString DesktopAppController::contactIdAtRow(int row) const
{
    auto* proxy = global_DBObjects.contactsmodelproxy();
    if (!proxy || row < 0 || row >= proxy->rowCount()) return {};
    return proxy->data(proxy->index(row, 0)).toString();
}

bool DesktopAppController::saveContact(int row, const QString& name, const QString& email,
                                      const QString& officePhone, const QString& cellPhone,
                                      const QString& companyId, const QString& role)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.contactsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    return applyRowFields(model, pIdx.row(), {
        {1, name},      {2, email},    {3, officePhone},
        {4, cellPhone}, {5, companyId}, {6, role} });
}

int DesktopAppController::addContactsFromVCardDrop(const QStringList& fileUrls, const QString& text)
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
        const QString companyId = findOrCreateCompany(&global_DBObjects, contact.company);
        findOrCreateContact(&global_DBObjects, contact, companyId);
    }

    // findOrCreateContact() writes through the unfiltered contacts model so the
    // insert can't be scoped out by whatever filter the visible Contacts list
    // currently has active — refresh the model actually bound to contactsModel()
    // so the new rows show up immediately rather than waiting for the next
    // dirty-triggered reload.
    global_DBObjects.contactsmodel()->refresh();

    emit infoOccurred(tr("Contacts Added"), contacts.size() == 1
        ? tr("1 contact was added.")
        : tr("%1 contacts were added.").arg(contacts.size()));
    return contacts.size();
}

// ── Companies ────────────────────────────────────────────────────────────────

int DesktopAppController::addCompany()
{
    return proxyRowFromSource(global_DBObjects.companiesmodelproxy(),
                              global_DBObjects.companiesmodel()->newRecord());
}

bool DesktopAppController::deleteCompany(int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(global_DBObjects.companiesmodelproxy(),
                       global_DBObjects.companiesmodel(), row))
        return true;
    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
}

QVariantMap DesktopAppController::getCompanyData(int row) const
{ return proxyRowToMap(global_DBObjects.companiesmodelproxy(), row); }

QString DesktopAppController::companyIdAtProxyRow(int row) const
{ return companyIdAtRow(row); }

bool DesktopAppController::saveCompany(int row, const QString& companyName)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.companiesmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    const bool ok = model->setData(model->index(row, 1), companyName);
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
        const QString& dateDue, bool internalItem,
        const QString& lastUpdate, const QString& dateResolved)
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
    if (!applyRowFields(model, pIdx.row(), {
        { 1, itemNumber},    { 2, itemType},   { 3, itemName},
        { 4, identifiedBy},  { 5, dateIdentified}, { 6, description},
        { 8, priority},      { 9, status},     { 7, assignedTo},
        {10, dateDue},       {15, internalItem ? "1" : "0"} }))
        return false;

    // Date Resolved (12) and Date Updated (11) are written last so a value the
    // user typed in those fields wins over TrackerItemsModel::setData's own
    // bookkeeping: every edit above bumps last_update to today, and setting the
    // status to "Resolved" stamps date_resolved. An empty string means "leave as
    // the model set it" — last_update is required so it is never legitimately
    // blank, and date_resolved is normally cleared by moving off "Resolved"
    // rather than by emptying the field.
    std::vector<std::pair<int, QString>> extra;
    if (!dateResolved.isEmpty()) extra.emplace_back(12, dateResolved);
    if (!lastUpdate.isEmpty())   extra.emplace_back(11, lastUpdate);
    for (const auto& f : extra) {
        if (!model->setData(model->index(pIdx.row(), f.first), f.second)) {
            QString err = global_DBObjects.lastSaveError();
            if (err.isEmpty()) err = tr("The record could not be saved.");
            emit errorOccurred(tr("Could Not Save"), err);
            return false;
        }
    }
    return true;
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
    for (const QString& contactId : { identifiedBy, assignedTo }) {
        if (contactId.isEmpty() || isProjectTeamMember(newProjectId, contactId)) continue;
        bool already = false;
        for (const QVariant& m : membersToAdd)
            if (m.toMap().value("id").toString() == contactId) { already = true; break; }
        if (already) continue;
        QVariantMap m;
        m["id"]   = contactId;
        m["name"] = contactNameForId(contactId);
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

        for (const QString& contactId : { identifiedBy, assignedTo })
            if (!contactId.isEmpty())
                addContactToProjectTeam(newProjectId, contactId);
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

bool DesktopAppController::saveTeamMember(int row, const QString& contactId,
                                          const QString& role, bool receiveStatusReport)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.projectteammembersmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;
    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    // Guard against adding the same contact to a project twice. There is a partial
    // unique index on project_people(project_id, people_id) WHERE deleted = 0, so a
    // duplicate would otherwise surface as a raw SQL failure. Column 0 is the row id
    // (empty for the not-yet-saved row we are filling in), column 1 is project_id.
    const QString rowId     = model->index(pIdx.row(), 0).data().toString();
    const QString projectId = model->index(pIdx.row(), 1).data().toString();
    if (!contactId.isEmpty() && !projectId.isEmpty())
    {
        DB_LOCK;
        QSqlQuery dup(global_DBObjects.getDb());
        dup.prepare("SELECT count(*) FROM project_people "
                    "WHERE project_id = ? AND people_id = ? AND deleted = 0 AND id <> ?");
        dup.addBindValue(projectId);
        dup.addBindValue(contactId);
        dup.addBindValue(rowId);
        dup.exec();
        const bool exists = dup.next() && dup.value(0).toInt() > 0;
        DB_UNLOCK;

        if (exists)
        {
            emit errorOccurred(tr("Duplicate Team Member"),
                               tr("That contact is already a team member on this project."));
            return false;
        }
    }

    // people_id is written first: it is NOT NULL in the schema and carries the
    // unique key, so it is the write that can be rejected. applyRowFields stops at
    // the first failure, so a rejected people_id never lets the later writes insert
    // a row with a null people_id (which would raise a raw SQL constraint error).
    return applyRowFields(model, pIdx.row(), {
        {2, contactId}, {4, receiveStatusReport ? "1" : "0"}, {5, role} });
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
        const QString companyId = findOrCreateCompany(&global_DBObjects, contact.company);
        const QString contactId = findOrCreateContact(&global_DBObjects, contact, companyId);
        if (!contactId.isEmpty())
            addContactToProjectTeam(projectId, contactId);
    }

    // Mirrors moveTrackerItem()'s membersToAdd handling: findOrCreateContact()
    // writes through the unfiltered contacts model, and addContactToProjectTeam()
    // writes straight against the source team-members model rather than
    // whatever project the visible proxy is currently filtered to — refresh
    // both so the drop's results show up immediately on screen.
    global_DBObjects.contactsmodel()->refresh();
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
    rememberLocation(fileUrlOrPath);
    return true;
}

void DesktopAppController::rememberLocation(const QString& fileUrlOrPath)
{
    SecurityScopedBookmarks::remember(fileUrlOrPath);
}

bool DesktopAppController::addProjectLocationFromUrl(const QString& projectId,
                                                     const QString& fileUrlOrPath)
{
    if (localPath(fileUrlOrPath).isEmpty()) return false;
    const int row = addProjectLocation(projectId);
    if (row < 0) return false;
    return setProjectLocationPath(row, fileUrlOrPath);
}

namespace {

// ms-word:ofe|u|<url> (and the ms-excel:/ms-powerpoint:/ms-project: equivalents)
// rely on a literal, un-encoded "|" as their command delimiter — see
// officeDeepLinkFor()'s header comment. QUrl has no "opaque URI" mode that
// preserves that: parsing the string into a QUrl and hoisting it back out
// through QDesktopServices::openUrl() percent-encodes each "|" to "%7C",
// which Office's protocol handler doesn't recognize ("This action couldn't
// be performed because Office doesn't recognize the command it was given").
// So these already-fully-formed native URIs go straight to the OS, bypassing
// QUrl's encode/decode round trip entirely.
bool openRawUri(const QString& uri)
{
#if defined(Q_OS_WIN)
    HINSTANCE result = ShellExecuteW(nullptr, L"open",
                                      reinterpret_cast<const wchar_t*>(uri.utf16()),
                                      nullptr, nullptr, SW_SHOWNORMAL);
    // Per the ShellExecute docs, a return value greater than 32 indicates success;
    // values 32 and below (e.g. SE_ERR_NOASSOC) mean no handler was found.
    return reinterpret_cast<INT_PTR>(result) > 32;
#elif defined(Q_OS_MACOS)
    // `open` resolves and hands the raw string to LaunchServices without
    // Qt's percent-encoding; a nonzero exit means no app claims the scheme.
    return QProcess::execute(QStringLiteral("open"), {uri}) == 0;
#else
    // No native Office desktop app / URI handler expected on this platform;
    // let the caller fall back to opening the canonical URL instead.
    Q_UNUSED(uri);
    return false;
#endif
}

} // namespace

void DesktopAppController::openProjectLocation(int row)
{
    QAbstractItemModel* model = global_DBObjects.projectlocationsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return;
    // Rows saved by the old save-time rewrite hold ms-excel:ofe|u|<url>;
    // unwrap them so they open exactly like a canonical row.
    const QString path = unwrapOfficeDeepLink(
        model->data(model->index(row, 4)).toString());
    if (path.isEmpty()) return;

    // full_path is stored canonically (a plain http(s)/SharePoint URL, not an
    // ms-office deep link — see the note in ProjectLocationsModel::setData()).
    // Try the native Word/Excel/PowerPoint/Project app first and fall back to
    // opening the canonical URL itself when nothing answers (e.g. the Office
    // app isn't installed). openRawUri() returns false when no handler is
    // registered for the ms-* scheme.
    //
    // Settings > Office 365 Integration lets the user turn this off entirely;
    // in that case open the browser-based Office Online viewer instead of
    // handing the file to a desktop app, rather than just opening the bare
    // document URL (which SharePoint/OneDrive can otherwise offer to download
    // or open in the desktop app itself, depending on tenant settings).
    if (!m_fileFinder || m_fileFinder->office365OpenLinksInDesktop()) {
        const QString deepLink = officeDeepLinkFor(path);
        if (!deepLink.isEmpty() && openRawUri(deepLink))
            return;
    } else {
        const QString viewerUrl = officeWebViewerUrlFor(path);
        if (!viewerUrl.isEmpty()) {
            QDesktopServices::openUrl(QUrl(viewerUrl, QUrl::TolerantMode));
            return;
        }
    }

    // A stored path with a URL scheme — http(s), an ms-office deep link
    // (kept only for rows saved before that rewrite was removed), mailto,
    // file, … — is handed to the OS as a URL so the browser or the
    // registered handler opens it; a bare filesystem path is opened as a
    // local file.
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
        if (auto* m = global_DBObjects.contactsmodel()) m->refresh();
    } else if (t == QLatin1String("clients")) {
        if (auto* m = global_DBObjects.companiesmodel()) m->refresh();
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

    // An .eml draft carries the archive as a real attachment, so the user has
    // nothing to attach by hand.
    PN::Comm::EmailRequest request;
    request.operationId = QUuid::createUuid();
    request.backend = PN::Comm::BackendId::DefaultApp;
    request.recipients.append({tr("Project Notes Support"), supportEmail, PN::Comm::RecipientRole::To});
    request.subject = tr("ProjectNotes Support Logs (v%1)").arg(appVersion());
    request.html = tr("<p>Please describe the problem you are seeing below.</p>"
                      "<p>The log archive is attached.</p>"
                      "<p>--- describe your issue here ---</p>");
    PN::Comm::Artifact archive;
    archive.artifactId = QUuid::createUuid();
    archive.absolutePath = zipPath;
    archive.displayName = QFileInfo(zipPath).fileName();
    archive.mimeType = QStringLiteral("application/zip");
    archive.generatedByApp = true;
    request.attachments.append(archive);

    const QByteArray message = PN::Comm::EmlWriter::write(request);
    const QString messagePath = QDir(destDir).filePath(
        QStringLiteral("ProjectNotes-logs-%1.eml").arg(stamp));
    QFile messageFile(messagePath);
    bool opened = false;
    if (!message.isEmpty() && messageFile.open(QIODevice::WriteOnly)
        && messageFile.write(message) == message.size()) {
        messageFile.close();
        opened = QDesktopServices::openUrl(QUrl::fromLocalFile(messagePath));
    }
    if (!opened) {
        // Without a mail application for .eml files there is still a usable
        // result: the archive itself, and where to send it.
        emit infoOccurred(tr("Send Logs to Support"),
            tr("A log archive was created here:\n\n%1\n\nPlease email it to %2.")
                .arg(QDir::toNativeSeparators(zipPath), supportEmail));
        return;
    }

    emit infoOccurred(tr("Send Logs to Support"),
        tr("A log archive was created here:\n\n%1\n\n"
           "An email to %2 has been started with the archive attached. Review it and send it.")
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
    // Switching it off must stop an already-running engine immediately —
    // otherwise a sync started earlier this session keeps going despite the
    // setting now reading "disabled".
    else stopSync();
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
    // An enabled-but-unconfigured setup (e.g. just after install) must not
    // initialize the sync engine with empty credentials.
    if (syncEmail().isEmpty() || syncPassword().isEmpty())
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
    if (syncEmail().isEmpty() || syncPassword().isEmpty())
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
    // A bare 401 is a routine JWT expiration: the sync loop reauthenticates with
    // the stored credentials and retries on its own, logging an error and
    // stopping only if that reauthentication fails. Don't blink the sync status
    // indicator red on every token refresh.
    const bool authOnlyFailure = !result.success && result.hasAuthError()
                                 && !result.hasNetworkError()
                                 && result.totalDecryptionFailures() == 0;
    if (result.success) {
        m_syncHasError = false;
        if (SqliteSyncPro* api = m_syncApi)
            QMetaObject::invokeMethod(api, [api, result]() { api->checkSyncStatus(result); },
                                      Qt::QueuedConnection);
    } else {
        m_syncHasError = !authOnlyFailure;
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
