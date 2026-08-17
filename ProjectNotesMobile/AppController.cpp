// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "AppController.h"
#include "MailComposer.h"

#include "QLogger.h"
#include "QLoggerWriter.h"

#include "sqlitesyncpro.h"
// Used by verifySyncSettings() to check the saved credentials and encryption
// phrase directly, without spinning up the sync engine.
#include "authmanager.h"
#include "httpclient.h"
#include "rowencryption.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSet>
#include <QSqlQuery>
#include <QTextDocument>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>

#include <algorithm>

using namespace QLogger;

// ── Singleton plumbing ───────────────────────────────────────────────────────

static AppController* s_instance = nullptr;

static void appendEmailRecipient(QStringList& emails,
                                 const QString& email,
                                 const QString& personId,
                                 const QString& excludedPersonId)
{
    if (!excludedPersonId.isEmpty() && personId == excludedPersonId)
        return;

    const QString trimmedEmail = email.trimmed();
    if (!trimmedEmail.isEmpty() && !emails.contains(trimmedEmail, Qt::CaseInsensitive))
        emails.append(trimmedEmail);
}

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

// Reapplies a model's persisted column filters (application_settings-backed);
// defined further down alongside applyColumnFilters()/clearColumnFilters().
static void restoreColumnFilters(SqlQueryModel* src);
// Reapplies a model's persisted sort order; defined further down alongside
// applySort()/clearSort().
static void restoreSort(QAbstractItemModel* model);

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

    // Initial load for the models QML needs populated as soon as the
    // database is ready.
    global_DBObjects.clientsmodel()->refresh();
    global_DBObjects.peoplemodel()->refresh();
    global_DBObjects.projectinformationmodel()->refresh();
    global_DBObjects.projectteammembersmodel()->refresh();

    // Restore any column filters left active in the Filter sheet before the
    // last quit (persisted to application_settings, keyed per table). No-op
    // for models with nothing persisted; allitemsmodel is otherwise loaded
    // lazily, so this forces it early only when a filter needs restoring.
    restoreColumnFilters(global_DBObjects.projectinformationmodel());
    restoreColumnFilters(global_DBObjects.clientsmodel());
    restoreColumnFilters(global_DBObjects.peoplemodel());
    restoreColumnFilters(global_DBObjects.allitemsmodel());
    ++m_filterRev;
    emit filterRevChanged();

    // Restore any persisted sort choice for every filterable section. Unlike
    // restoreColumnFilters() above, this is NOT called synchronously here.
    // openOrCreateDatabase() runs directly from Main.qml's Component.onCompleted,
    // which is still inside QQmlApplicationEngine::load() — before main.cpp's
    // window->show() and before iOS's asynchronous delegate incubation for the
    // SwipeView's ListViews has finished. proxy->sort() rebuilds the proxy's
    // whole row mapping (layoutAboutToBeChanged/layoutChanged); doing that while
    // a ListView's initial delegates are still being incubated crashes deep in
    // Qt with no app-code frames on the stack — the same failure this was
    // previously synchronous specifically to dodge, just reached from the other
    // direction. Deferring restoreAllSorts() with a 0ms singleShot posts it as
    // a real event on the event loop, so it only runs once control has fully
    // returned to app.exec() — after window->show() and after the first frame's
    // delegates have finished incubating — instead of racing them.
    QTimer::singleShot(0, this, &AppController::restoreAllSorts);

    // Tell QML the view-option properties are now readable.
    emit viewOptionsChanged();
    emit databaseReady();
    return true;
}

// Applies every filterable section's persisted sort order. Split out of
// openOrCreateDatabase() so it can be posted via QTimer::singleShot() instead
// of running inline — see the call site above for why.
void AppController::restoreAllSorts()
{
    for (const QString& section : { QStringLiteral("projects"), QStringLiteral("items"),
                                     QStringLiteral("people"), QStringLiteral("clients"),
                                     QStringLiteral("statusreport"), QStringLiteral("trackeritems"),
                                     QStringLiteral("team"), QStringLiteral("locations"),
                                     QStringLiteral("notes") })
        restoreSort(modelForSection(section));
    ++m_sortRev;
    emit sortRevChanged();
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
    m_syncApi->setSyncHostType(1);  // always Supabase
    m_syncApi->setPostgrestUrl(supabaseUrl());
    m_syncApi->setSupabaseKey(supabaseAnonKey());
    m_syncApi->setEmail(global_MobileSettings.getSyncEmail());
    m_syncApi->setPassword(global_MobileSettings.getSyncPassword());
    m_syncApi->setEncryptionPhrase(global_MobileSettings.getSyncEncryptionPhrase());
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
            return;
        }

        // Check subscription status after a successful init (still on API thread).
        if (api->isAuthenticated()) {
            const SubscriptionStatus sub = api->getSubscriptionStatus();
            QMetaObject::invokeMethod(this, [this, api, sub]() {
                // Build display text matching the desktop style
                QString text;
                if (sub.valid) {
                    const bool isActive =
                        sub.status.compare(QLatin1String("active"),   Qt::CaseInsensitive) == 0 ||
                        sub.status.compare(QLatin1String("trialing"), Qt::CaseInsensitive) == 0;
                    const QString color = isActive ? QStringLiteral("green") : QStringLiteral("red");
                    const QString statusWord = sub.status.isEmpty() ? tr("None")
                        : (sub.status.at(0).toUpper() + sub.status.mid(1).toLower());
                    const QString statusHtml = QStringLiteral("<span style=\"color:%1\">%2</span>")
                        .arg(color, statusWord);
                    if (sub.hasActiveSubscription) {
                        text = tr("Subscription: %1 — %2").arg(sub.planName, statusHtml);
                        if (sub.currentPeriodEnd.isValid())
                            text += tr(" (renews %1)").arg(sub.currentPeriodEnd.toString(QStringLiteral("MMM d, yyyy")));
                    } else {
                        text = tr("Subscription: %1").arg(statusHtml);
                    }
                } else {
                    text = QStringLiteral("<span style=\"color:red\">%1</span>")
                        .arg(tr("Subscription status unavailable"));
                }
                setSubscriptionStatusText(text);

                if (sub.valid && !sub.hasActiveSubscription) {
                    emit subscriptionExpired();
                    // Stop sync but keep the API alive for status display
                    QMetaObject::invokeMethod(api, [api]() { api->shutdown(); },
                                              Qt::QueuedConnection);
                }
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
    if (global_MobileSettings.getSyncEmail().isEmpty())    return;
    if (global_MobileSettings.getSyncPassword().isEmpty()) return;

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

QString AppController::supabaseUrl()
{
    return MobileSettings::isTestSupabase()
        ? QStringLiteral("https://lsulnvxgrlpuqtzonner.supabase.co")
        : QStringLiteral("https://nrtjpzkrldwydkbopsml.supabase.co");
}

QString AppController::supabaseAnonKey()
{
    return MobileSettings::isTestSupabase()
        ? QStringLiteral("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImxzdWxudnhncmxwdXF0em9ubmVyIiwicm9sZSI6ImFub24iLCJpYXQiOjE3Nzg1ODY0OTIsImV4cCI6MjA5NDE2MjQ5Mn0.AyEQHLZadhj5r0BNkvPASaMZ0gTr4LAueq0SGVuua3s")
        : QStringLiteral("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im5ydGpwemtybGR3eWRrYm9wc21sIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzM4NTU0NTQsImV4cCI6MjA4OTQzMTQ1NH0.hzzyb5bFKDIFbrJ7Fa8INh57pWIkz52csQ2gQ_L302E");
}

QString AppController::supabaseConnectionInfo() const
{
    const bool isTest = MobileSettings::isTestSupabase();
    const QString projectId = isTest ? QStringLiteral("lsulnvxgrlpuqtzonner")
                                     : QStringLiteral("nrtjpzkrldwydkbopsml");
    const QString env = isTest ? tr("Test") : tr("Production");
    return tr("Project ID: %1 (%2)").arg(projectId, env);
}

void AppController::setSubscriptionStatusText(const QString& text)
{
    if (m_subscriptionStatusText == text)
        return;
    m_subscriptionStatusText = text;
    emit subscriptionStatusChanged();
}

// ── Sync settings + verification ─────────────────────────────────────────────
//
// Mirrors DesktopAppController's implementation so both frontends report the
// same problems in the same words.

// Each setter is a no-op when the value is unchanged: the Cloud Sync Settings
// page commits every field whenever it loses focus (and again on the way out),
// so without the guard simply opening the page would mark the settings
// unverified and trigger a pointless round trip to the host.
void AppController::setSyncEnabled(bool v)
{
    if (syncEnabled() == v) return;
    global_MobileSettings.setSyncEnabled(v);
    // Switching sync on is the moment the stored credentials start to matter.
    if (v) setSyncSettingsUnverified(true);
    emit syncSettingsChanged();
}

void AppController::setSyncEmail(const QString& v)
{
    if (syncEmail() == v) return;
    global_MobileSettings.setSyncEmail(v);
    setSyncSettingsUnverified(true);
    emit syncSettingsChanged();
}

void AppController::setSyncPassword(const QString& v)
{
    if (syncPassword() == v) return;
    global_MobileSettings.setSyncPassword(v);
    setSyncSettingsUnverified(true);
    emit syncSettingsChanged();
}

void AppController::setSyncEncryptionPhrase(const QString& v)
{
    if (syncEncryptionPhrase() == v) return;
    global_MobileSettings.setSyncEncryptionPhrase(v);
    setSyncSettingsUnverified(true);
    emit syncSettingsChanged();
}

void AppController::setSyncSettingsUnverified(bool unverified)
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

void AppController::verifySyncSettings()
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
    QPointer<AppController> self(this);

    // Its own thread: HttpClient/AuthManager block on a nested event loop, and
    // the sync engine's thread may well be mid-cycle. The verdict is posted back
    // through the application object and the guard is only dereferenced on the
    // GUI thread, so a controller torn down mid-check just drops the answer.
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

        QMetaObject::invokeMethod(QCoreApplication::instance(), [self, status, detail]() {
            if (self)
                self->finishSyncVerification(status, detail);
        }, Qt::QueuedConnection);
    });
    worker->setObjectName(QStringLiteral("SyncSettingsVerify"));
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void AppController::finishSyncVerification(const QString& status, const QString& detail)
{
    m_syncVerifyInProgress = false;
    emit syncVerifyInProgressChanged();

    // One check per round of edits, whatever the verdict: the user has been told,
    // and re-asking every time they leave the page (especially while offline)
    // helps nobody. Editing a field marks the settings unverified again.
    setSyncSettingsUnverified(false);

    QString message;
    if (status == QLatin1String("credentials")) {
        message = tr("The sync host rejected your sync email and password.\n\n"
                     "Cloud sync will not run until they are corrected. Check the "
                     "Email and Password fields in Cloud Sync Settings.");
    } else if (status == QLatin1String("encryption")) {
        message = tr("Your encryption phrase does not match the one this account's "
                     "data was encrypted with.\n\n"
                     "Records synced from your other devices cannot be decrypted and "
                     "will be skipped. Check the Encryption Phrase field in Cloud "
                     "Sync Settings.");
    } else if (status == QLatin1String("offline")) {
        message = tr("Your cloud sync settings were saved, but the sync host could not "
                     "be reached to check them.\n\n"
                     "They will be checked again the next time sync runs.");
    }

    // ERRORLOG rather than the desktop's SYNCERRORLOG — it is the only
    // file destination this app registers (see the constructor).
    if (!message.isEmpty()) {
        QLog_Error(ERRORLOG,
            QString("Cloud sync settings check reported '%1'%2")
                .arg(status, detail.isEmpty() ? QString() : QStringLiteral(": ") + detail));
    }

    emit syncSettingsVerified(status, message);
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

    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    bool ok = true;
    ok &= model->setData(model->index(pIdx.row(), 1), name);
    ok &= model->setData(model->index(pIdx.row(), 2), email);
    ok &= model->setData(model->index(pIdx.row(), 3), officePhone);
    ok &= model->setData(model->index(pIdx.row(), 4), cellPhone);
    ok &= model->setData(model->index(pIdx.row(), 5), clientId);
    ok &= model->setData(model->index(pIdx.row(), 6), role);
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
    auto* proxy = global_DBObjects.projectinformationmodelproxy();
    QAbstractItemModel* model = proxy;
    if (row < 0 || row >= model->rowCount())
        return false;

    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    // A row addProject() only staged has no id yet, and can't be written one
    // column at a time: the first setData() would try to INSERT it while the
    // other required column is still null. Write the whole row at once.
    auto* src = global_DBObjects.projectinformationmodel();
    const QModelIndex srcIdx = proxy->mapToSource(model->index(row, 0));
    if (srcIdx.isValid() && src->isNewRecord(srcIdx))
        return insertStagedProject(srcIdx.row(),
                { { 1, projectNumber},   { 2, projectName},      { 3, lastStatusDate},
                  { 4, lastInvoiceDate}, { 5, primaryContactId}, {11, invoicingPeriod},
                  {12, statusReportPeriod}, {13, clientId},      {14, projectStatus} });

    bool ok = true;
    ok &= model->setData(model->index(pIdx.row(),  1), projectNumber);
    ok &= model->setData(model->index(pIdx.row(),  2), projectName);
    ok &= model->setData(model->index(pIdx.row(),  3), lastStatusDate);
    ok &= model->setData(model->index(pIdx.row(),  4), lastInvoiceDate);
    ok &= model->setData(model->index(pIdx.row(),  5), primaryContactId);
    ok &= model->setData(model->index(pIdx.row(), 11), invoicingPeriod);
    ok &= model->setData(model->index(pIdx.row(), 12), statusReportPeriod);
    ok &= model->setData(model->index(pIdx.row(), 13), clientId);
    ok &= model->setData(model->index(pIdx.row(), 14), projectStatus);
    if (!ok) {
        const QString err = global_DBObjects.lastSaveError();
        emit errorOccurred(tr("Could Not Save"), err);
    }
    return ok;
}

// Write a staged project row (see addProject) as one INSERT, then finish what
// ProjectsModel::setData() would have done for a row it inserted itself: give
// the project its default project manager.
bool AppController::insertStagedProject(int srcRow, const QVector<QPair<int, QVariant>>& fields)
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

    // The EVM columns are computed in the SELECT, not stored — re-query the row
    // so the page reads them back.
    src->reloadRecord(src->index(srcRow, 0));
    return true;
}

bool AppController::saveStatusItem(int row, const QString& category, const QString& description)
{
    global_DBObjects.setLastSaveError("");
    QAbstractItemModel* model = global_DBObjects.statusreportitemsmodelproxy();
    if (row < 0 || row >= model->rowCount()) return false;

    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    bool ok = true;
    ok &= model->setData(model->index(pIdx.row(), 2), category);
    ok &= model->setData(model->index(pIdx.row(), 3), description);
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

    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    bool ok = true;
    ok &= model->setData(model->index(pIdx.row(), 2), peopleId);
    ok &= model->setData(model->index(pIdx.row(), 4), receiveStatusReport ? "1" : "0");
    ok &= model->setData(model->index(pIdx.row(), 5), role);
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

    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    bool ok = true;
    ok &= model->setData(model->index(pIdx.row(), 2), locationType);
    ok &= model->setData(model->index(pIdx.row(), 3), description);
    ok &= model->setData(model->index(pIdx.row(), 4), path);
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

    // Pin the record by persistent index so a re-sort triggered by an earlier
    // setData()'s side effects can't leave later setData() calls writing to the
    // wrong row.
    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    bool ok = true;
    ok &= model->setData(model->index(pIdx.row(), 2), title);
    ok &= model->setData(model->index(pIdx.row(), 3), date);
    ok &= model->setData(model->index(pIdx.row(), 4), note);
    ok &= model->setData(model->index(pIdx.row(), 5), internalItem ? "1" : "0");
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

QString AppController::peopleNameForId(const QString& personId) const
{
    auto* src = global_DBObjects.peoplemodel();
    if (!src || personId.isEmpty()) return {};
    QVariant key(personId);
    return src->findValue(key, 0, 1).toString();  // col 1 = name
}

QString AppController::peopleEmailForId(const QString& personId) const
{
    auto* src = global_DBObjects.peoplemodel();
    if (!src || personId.isEmpty()) return {};
    QVariant key(personId);
    return src->findValue(key, 0, 2).toString();  // col 2 = email
}

QString AppController::clientNameForId(const QString& clientId) const
{
    auto* src = global_DBObjects.clientsmodel();
    if (!src || clientId.isEmpty()) return {};
    QVariant key(clientId);
    return src->findValue(key, 0, 1).toString();  // col 1 = client_name
}

QString AppController::projectNumberForId(const QString& projectId) const
{
    auto* src = global_DBObjects.projectinformationmodel();
    if (!src || projectId.isEmpty()) return {};
    QVariant key(projectId);
    return src->findValue(key, 0, 1).toString();  // col 1 = project_number
}

QString AppController::projectNameForId(const QString& projectId) const
{
    auto* src = global_DBObjects.projectinformationmodel();
    if (!src || projectId.isEmpty()) return {};
    QVariant key(projectId);
    return src->findValue(key, 0, 2).toString();  // col 2 = project_name
}

QVariantList AppController::teamMemberList(const QString& projectId, const QStringList& includeIds) const
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
    // existing assignment still shows (matches desktop's team combo).
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

QVariantList AppController::clientList() const
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

QVariantList AppController::peopleList() const
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

QString AppController::teamMemberEmailList() const
{
    QAbstractItemModel* model = global_DBObjects.projectteammembersmodelproxy();
    QStringList emails;
    const QString projectManagerId = global_DBObjects.getProjectManager();
    for (int row = 0; row < model->rowCount(); ++row) {
        const QString personId = model->data(model->index(row, 2)).toString(); // col 2 = people_id
        const QString email = model->data(model->index(row, 6)).toString();  // col 6 = email
        appendEmailRecipient(emails, email, personId, projectManagerId);
    }
    return emails.join(",");
}

QString AppController::attendeeEmailList() const
{
    QAbstractItemModel* model = global_DBObjects.meetingattendeesmodelproxy();
    QStringList emails;
    const QString projectManagerId = global_DBObjects.getProjectManager();
    for (int row = 0; row < model->rowCount(); ++row) {
        const QString personId = model->data(model->index(row, 2)).toString(); // col 2 = person_id
        const QString email = model->data(model->index(row, 5)).toString();  // col 5 = email
        appendEmailRecipient(emails, email, personId, projectManagerId);
    }
    return emails.join(",");
}

void AppController::sendMeetingNotesEmail(const QString& commaSeparatedEmails,
                                          const QString& subject,
                                          const QString& body)
{
    QStringList addresses;
    const auto parts = commaSeparatedEmails.split(u',', Qt::SkipEmptyParts);
    for (const QString& p : parts)
        addresses.append(p.trimmed());

    MailComposer::present(addresses, subject, body);
}

QString AppController::attendeeNameList() const
{
    QAbstractItemModel* model = global_DBObjects.meetingattendeesmodelproxy();
    QStringList names;
    for (int row = 0; row < model->rowCount(); ++row) {
        const QString name = model->data(model->index(row, 3)).toString(); // col 3 = name
        if (!name.isEmpty())
            names.append(name);
    }
    return names.join("\r\n");
}

QString AppController::noteActionItemsSummary() const
{
    QAbstractItemModel* model = global_DBObjects.notesactionitemsmodelproxy();
    QStringList entries;
    for (int row = 0; row < model->rowCount(); ++row) {
        const QString item       = model->data(model->index(row,  3)).toString(); // col 3 = item_name
        const QString assignedId = model->data(model->index(row,  7)).toString(); // col 7 = assigned_to (person_id)
        const QString due        = model->data(model->index(row, 10)).toString(); // col 10 = date_due
        const QString assigned   = assignedId.isEmpty() ? QString() : peopleNameForId(assignedId);

        QString entry = item;
        if (!assigned.isEmpty())
            entry += "  Assigned To: " + assigned;
        if (!due.isEmpty())
            entry += "  Due By: " + due;
        entries.append(entry);
    }
    return entries.join("\r\n");
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

// Resolve the SqlQueryModel behind a proxy model handed over from QML.
static SqlQueryModel* sourceModelOf(QAbstractItemModel* model)
{
    if (auto* proxy = qobject_cast<SortFilterProxyModel*>(model))
        return qobject_cast<SqlQueryModel*>(proxy->sourceModel());
    return nullptr;
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

// Delete the record at proxy |row|, surfacing the model's blocked-delete message
// to the user when an external (non-owned) reference prevents it. Owned child
// records are cascade-deleted instead of blocking. deleteRecord() (via
// deleteCheck) already stores that message in lastSaveError on mobile; here we
// emit it through the existing errorOccurred → errorDialog path used by saves.
bool AppController::deleteAndReport(SortFilterProxyModel* proxy, SqlQueryModel* source, int row)
{
    global_DBObjects.setLastSaveError("");
    if (deleteProxyRow(proxy, source, row))
        return true;

    const QString err = global_DBObjects.lastSaveError();
    if (!err.isEmpty())
        emit errorOccurred(tr("Cannot Delete"), err);
    return false;
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

// Stage a new project row in the model cache without writing it — the schema
// requires a project number and name (both NOT NULL and carrying partial unique
// indexes), so a blank project cannot be INSERTed. saveProject() writes the row
// in one go once ProjectDetailsPage has those two fields; leaving the page
// without them discards the staged row (ProjectDetailsPage._discardNew).
int AppController::addProject()
{
    return proxyRowFromSource(global_DBObjects.projectinformationmodelproxy(),
                              global_DBObjects.projectinformationmodel()->newRecord());
}

QString AppController::nextProjectNumber() const
{
    return global_DBObjects.projectinformationmodel()->nextAvailableProjectNumber();
}

bool AppController::deleteProject(int row)
{
    return deleteAndReport(global_DBObjects.projectinformationmodelproxy(),
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
    return deleteAndReport(global_DBObjects.peoplemodelproxy(),
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
    return deleteAndReport(global_DBObjects.clientsmodelproxy(),
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
    return deleteAndReport(global_DBObjects.statusreportitemsmodelproxy(),
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
    return deleteAndReport(global_DBObjects.projectteammembersmodelproxy(),
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
    return deleteAndReport(global_DBObjects.projectlocationsmodelproxy(),
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
    auto* src = global_DBObjects.projectnotesmodel();
    QModelIndex srcIdx = src->newRecord(&fk);
    if (!srcIdx.isValid()) return -1;

    // Commit the new row to the DB up front so subsequent setData() calls in
    // saveProjectNote() all take the simple UPDATE branch. Without this, the
    // first setData() takes the INSERT branch, which fires addDefaultPMToMeeting
    // and cascades updateDisplayData() back into this same model mid-save —
    // fragile and prone to leaving a stale proxy row behind.
    if (!src->insertCacheRow(srcIdx.row())) return -1;

    const QString newId = src->data(src->index(srcIdx.row(), 0)).toString();
    if (!newId.isEmpty())
        global_DBObjects.addDefaultPMToMeeting(newId);

    return proxyRowFromSource(global_DBObjects.projectnotesmodelproxy(), srcIdx);
}

bool AppController::deleteProjectNote(int row)
{
    return deleteAndReport(global_DBObjects.projectnotesmodelproxy(),
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

    // Filter meetings to the same project (col 1 = project_id)
    if (!projectId.isEmpty())
        global_DBObjects.actionitemsdetailsmeetingsmodel()->setFilter(1, projectId);
    else
        global_DBObjects.actionitemsdetailsmeetingsmodel()->clearFilter(1);
    global_DBObjects.actionitemsdetailsmeetingsmodel()->refresh();

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
    return deleteAndReport(global_DBObjects.actionitemsdetailsmodelproxy(),
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
                                          const QString& itemId,
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
                                          const QString& noteId,
                                          bool           internalItem)
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

    bool ok = true;
    ok &= model->setData(model->index(pIdx.row(),  1), itemNumber);
    ok &= model->setData(model->index(pIdx.row(),  2), itemType);
    ok &= model->setData(model->index(pIdx.row(),  3), itemName);
    ok &= model->setData(model->index(pIdx.row(),  4), identifiedBy);
    ok &= model->setData(model->index(pIdx.row(),  5), dateIdentified);
    ok &= model->setData(model->index(pIdx.row(),  6), description);
    ok &= model->setData(model->index(pIdx.row(),  7), assignedTo);
    ok &= model->setData(model->index(pIdx.row(),  8), priority);
    ok &= model->setData(model->index(pIdx.row(),  9), status);
    ok &= model->setData(model->index(pIdx.row(), 10), dateDue);
    ok &= model->setData(model->index(pIdx.row(), 13), noteId);
    ok &= model->setData(model->index(pIdx.row(), 15), internalItem ? "1" : "0");
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

QAbstractItemModel* AppController::trackerItemMeetingsModel() const
{
    return global_DBObjects.actionitemsdetailsmeetingsmodelproxy();
}

int AppController::meetingRowForNoteId(const QString& noteId) const
{
    if (noteId.isEmpty()) return -1;
    auto* m = global_DBObjects.actionitemsdetailsmeetingsmodelproxy();
    for (int r = 0; r < m->rowCount(); ++r)
        if (m->data(m->index(r, 0)).toString() == noteId) return r;
    return -1;
}

QString AppController::meetingNoteIdAtRow(int row) const
{
    auto* m = global_DBObjects.actionitemsdetailsmeetingsmodelproxy();
    if (row < 0 || row >= m->rowCount()) return {};
    return m->data(m->index(row, 0)).toString();
}

QVariantList AppController::meetingList() const
{
    QVariantList out;
    auto* proxy = global_DBObjects.actionitemsdetailsmeetingsmodelproxy();
    if (!proxy) return out;
    for (int row = 0; row < proxy->rowCount(); ++row) {
        QVariantMap m;
        m.insert("id",   proxy->data(proxy->index(row, 0)).toString()); // col 0 = note id
        m.insert("name", proxy->data(proxy->index(row, 2)).toString()); // col 2 = meeting label
        out.append(m);
    }
    return out;
}

bool AppController::isItemNumberUnique(const QString& projectId, const QString& itemId, const QString& itemNumber) const
{
    if (itemNumber.isEmpty()) return true;
    if (projectId.trimmed().isEmpty()) return true;
    QString safeProject = projectId.trimmed().replace("'", "''");
    QString safeId      = itemId.trimmed().replace("'", "''");
    QString safeNumber  = itemNumber.trimmed().replace("'", "''");
    QString sql = QString(
        "SELECT COUNT(*) FROM item_tracker "
        "WHERE project_id = '%1' "
        "AND item_number = '%2' "
        "AND id != '%3' "
        "AND deleted = 0"
    ).arg(safeProject, safeNumber, safeId);
    return global_DBObjects.execute(sql).toInt() == 0;
}

bool AppController::isItemNameUnique(const QString& projectId, const QString& itemId, const QString& itemName) const
{
    if (itemName.trimmed().isEmpty()) return true;
    if (projectId.trimmed().isEmpty()) return true;
    QString safeProject = projectId.trimmed().replace("'", "''");
    QString safeId      = itemId.trimmed().replace("'", "''");
    QString safeName    = itemName.trimmed().replace("'", "''");
    QString sql = QString(
        "SELECT COUNT(*) FROM item_tracker "
        "WHERE project_id = '%1' "
        "AND item_name = '%2' "
        "AND id != '%3' "
        "AND deleted = 0"
    ).arg(safeProject, safeName, safeId);
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
    return deleteAndReport(global_DBObjects.trackeritemscommentsmodelproxy(),
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

    const QPersistentModelIndex pIdx(model->index(row, 0));
    if (!pIdx.isValid()) return false;

    bool ok = true;
    ok &= model->setData(model->index(pIdx.row(), 2), date);
    ok &= model->setData(model->index(pIdx.row(), 3), note);
    ok &= model->setData(model->index(pIdx.row(), 4), updatedBy);
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
    return deleteAndReport(global_DBObjects.meetingattendeesmodelproxy(),
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

    QString safeNote   = model->data(model->index(row, 1)).toString().replace("'", "''");
    QString safeId     = model->data(model->index(row, 0)).toString().replace("'", "''");
    QString safePerson = QString(personId).replace("'", "''");
    QString sql = QString(
        "SELECT COUNT(*) FROM meeting_attendees "
        "WHERE note_id = '%1' AND person_id = '%2' AND id != '%3' AND deleted = 0"
    ).arg(safeNote, safePerson, safeId);
    if (global_DBObjects.execute(sql).toInt() > 0) {
        const QString msg = tr("Attendee already exists.");
        global_DBObjects.setLastSaveError(msg);
        emit errorOccurred(tr("Could Not Save"), msg);
        return false;
    }

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
    return deleteAndReport(global_DBObjects.notesactionitemsmodelproxy(),
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
    global_DBObjects.projectteammembersmodel()->refreshIfDirty();
}

void AppController::refreshMeetingAttendees()
{
    global_DBObjects.meetingattendeesmodel()->refreshIfDirty();
}

void AppController::refreshTrackerComments()
{
    global_DBObjects.trackeritemscommentsmodel()->refreshIfDirty();
}

void AppController::refreshTrackerItems()
{
    global_DBObjects.trackeritemsmodel()->refreshIfDirty();
}

void AppController::refreshAllItems()
{
    global_DBObjects.allitemsmodel()->refreshIfDirty();
}

void AppController::refreshProjectNotes()
{
    global_DBObjects.projectnotesmodel()->refreshIfDirty();
}

void AppController::refreshNoteActionItems()
{
    global_DBObjects.notesactionitemsmodel()->refreshIfDirty();
}

void AppController::refreshProjectsList()
{
    global_DBObjects.projectinformationmodel()->refreshIfDirty();
}

void AppController::refreshPeople()
{
    global_DBObjects.peoplemodel()->refreshIfDirty();
}

void AppController::refreshClients()
{
    global_DBObjects.clientsmodel()->refreshIfDirty();
}

void AppController::setQuickSearch(QAbstractItemModel* model, const QString& text)
{
    if (auto* proxy = dynamic_cast<SortFilterProxyModel*>(model))
        proxy->setQuickSearch(text);
}

// ── Column filter editor (mirrors DesktopAppController) ──────────────────────

QVariantList AppController::filterColumns(QAbstractItemModel* model) const
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

QVariantList AppController::columnDistinctValues(QAbstractItemModel* model,
                                                  const QString& field) const
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return {};
    const int col = src->getColumnNumber(field);
    if (col < 0) return {};

    // Pull distinct values from the base data (context/deleted-row filters
    // kept, interactive user filter dropped) rather than the model's
    // currently-loaded rows — otherwise an already-active filter would
    // narrow the value list and you could never pick a different value.
    const QString dbcol = src->getColumnName(col);
    QString where = src->constructWhereClause(false);   // "" or " WHERE ... "
    where = where.isEmpty() ? QStringLiteral(" WHERE ") : (where + QStringLiteral(" AND "));

    const QString sql = "SELECT DISTINCT " + dbcol + " FROM ( " + src->BaseSQL()
                        + where + dbcol + " IS NOT NULL )";

    const SqlQueryModel::DBColumnType type = src->getType(col);

    // Foreign-key columns (e.g. client_id) store the id as the filterable
    // value; resolve it through the lookup table's display column, matching
    // the desktop Filter Editor's behavior.
    const QString lookupTable  = src->getLookupTable(col);
    const QString lookupFkCol  = src->getLookupFkColumnName(col);
    const QString lookupValCol = src->getLookupValueColumnName(col);

    QVariantList values;
    QSet<QString> seen;
    QSqlQuery q(src->getDBOs()->getDb());
    if (q.exec(sql)) {
        while (q.next() && values.size() < 500) {
            QVariant raw = q.value(0);
            src->reformatValue(raw, type);   // match the list's display formatting
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
        QLog_Error(ERRORLOG, QString("columnDistinctValues query failed: %1 — %2")
                                  .arg(q.lastError().text(), sql));
    }

    std::sort(values.begin(), values.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("label").toString().compare(
                   b.toMap().value("label").toString(), Qt::CaseInsensitive) < 0;
    });
    return values;
}

// Scope string used to key a model's persisted UI state (column filters, sort
// order) in application_settings. trackeritemsmodel() (a project's own
// Tracker Items tab) and allitemsmodel() (the master cross-project Items
// list) both report tablename() == "item_tracker" — disambiguate the
// project-scoped one so filtering/sorting one doesn't silently overwrite the
// other's persisted state (same fix as DesktopAppController).
static QString settingsScopeForModel(SqlQueryModel* src)
{
    QString scope = src->tablename();
    if (src == global_DBObjects.trackeritemsmodel())
        scope += ":project";
    return scope;
}

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

void AppController::applyColumnFilters(QAbstractItemModel* model, const QVariantList& specs)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return;

    applyFilterSpecsToModel(src, specs);
    src->activateUserFilter(QString());   // empty name → skip QSettings-backed persistence; application_settings below is canonical

    const QByteArray json = QJsonDocument(QJsonArray::fromVariantList(specs)).toJson(QJsonDocument::Compact);
    global_DBObjects.saveParameter(columnFilterSettingKey(src), QString::fromUtf8(json));

    ++m_filterRev;
    emit filterRevChanged();
}

void AppController::clearColumnFilters(QAbstractItemModel* model)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return;
    src->clearAllUserSearches();
    src->deactivateUserFilter(QString());
    global_DBObjects.saveParameter(columnFilterSettingKey(src), QString());

    ++m_filterRev;
    emit filterRevChanged();
}

QVariantList AppController::activeColumnFilters(QAbstractItemModel* model)
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

void AppController::applyQuickFilter(QAbstractItemModel* model, const QString& field,
                                     const QVariantList& values)
{
    QVariantList specs = activeColumnFilters(model);

    // Remove whatever's currently set for this field (if any) — either to
    // replace it below, or, if it already matched exactly, to leave it
    // removed (toggle-off: picking an already-active Quick Filter again
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

bool AppController::hasActiveColumnFilters(QAbstractItemModel* model) const
{
    SqlQueryModel* src = sourceModelOf(model);
    return src && src->hasUserFilters();
}

QAbstractItemModel* AppController::modelForSection(const QString& section) const
{
    if (section == "projects")     return projectsListModel();
    if (section == "items")        return allItemsModel();
    if (section == "people")       return peopleModel();
    if (section == "clients")      return clientsModel();
    if (section == "statusreport") return statusReportItemsModel();
    if (section == "trackeritems") return trackerItemsModel();
    if (section == "team")         return projectTeamMembersModel();
    if (section == "locations")    return projectLocationsModel();
    if (section == "notes")        return projectNotesModel();
    return nullptr;
}

void AppController::refreshModel(QAbstractItemModel* model)
{
    if (SqlQueryModel* src = sourceModelOf(model))
        src->refresh();
}

// ── Sort ─────────────────────────────────────────────────────────────────────

QVariantList AppController::sortColumns(QAbstractItemModel* model) const
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
// scope resolver the column filters use above.
static QString sortSettingKey(SqlQueryModel* src)
{
    return "UI:SortOrder:" + settingsScopeForModel(src);
}

void AppController::applySort(QAbstractItemModel* model, const QString& field, bool descending)
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

void AppController::clearSort(QAbstractItemModel* model)
{
    auto* proxy = qobject_cast<SortFilterProxyModel*>(model);
    SqlQueryModel* src = sourceModelOf(model);
    if (!proxy || !src) return;

    proxy->sort(-1);   // restores the model's base ORDER BY
    global_DBObjects.saveParameter(sortSettingKey(src), QString());

    ++m_sortRev;
    emit sortRevChanged();
}

QVariantMap AppController::activeSort(QAbstractItemModel* model) const
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return {};
    const QString json = global_DBObjects.loadParameter(sortSettingKey(src));
    if (json.isEmpty()) return {};
    return QJsonDocument::fromJson(json.toUtf8()).object().toVariantMap();
}

int AppController::rowForId(QAbstractItemModel* model, const QString& id) const
{
    auto* proxy = qobject_cast<SortFilterProxyModel*>(model);
    SqlQueryModel* src = sourceModelOf(model);
    if (!proxy || !src) return -1;

    // An empty id is not "no record": a row one of the add* methods only staged
    // has no id until its first save, and the detail page editing it still has to
    // find it (see ProjectDetailsPage._saveNow and friends). findIndex() keys
    // rows on their id column, so it locates the staged row by that empty key.
    // At most one row per model is ever in that state — the page that staged it
    // either saves it or discards it before another can be added.
    QVariant key(id);
    const QModelIndex srcIdx = src->findIndex(key, 0);
    if (!srcIdx.isValid()) return -1;

    const QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
    return proxyIdx.isValid() ? proxyIdx.row() : -1;
}

// Reapply a model's persisted column filters (if any) after it's been
// created. Called once per filterable model right after the database opens,
// so lists stay filtered across an app restart the same way the Filter
// Editor left them.
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

// Reapply a model's persisted sort (if any) after it's been created. Sorting
// an unloaded model is cheap — it just sets the pending column/order, which
// the proxy applies when it next builds its row mapping — so every
// filterable section is restored here.
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
