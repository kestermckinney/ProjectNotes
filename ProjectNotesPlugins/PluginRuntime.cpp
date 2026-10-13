// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginRuntime.h"

#include "ComBridge.h"
#include "DataService.h"
#include "JsConvert.h"
#include "PluginServices.h"

#include "QLogger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QUrl>

using namespace QLogger;

namespace PN::Plugins {

namespace {

qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }

QString readResource(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}

QString cleanAbsolute(const QString& path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool isUnder(const QString& path, const QString& root)
{
    if (root.isEmpty())
        return false;
    const QString p = cleanAbsolute(path);
    const QString r = cleanAbsolute(root);
#ifdef Q_OS_WIN
    const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    return p.compare(r, cs) == 0 || p.startsWith(r + QLatin1Char('/'), cs);
}

// Methods that are passed to PluginServices, with the permission each needs.
// An empty permission means none is required.
const QHash<QString, QString>& forwardedMethods()
{
    static const QHash<QString, QString> methods {
        {QStringLiteral("ui.message"), {}}, {QStringLiteral("ui.confirm"), {}}, {QStringLiteral("ui.prompt"), {}},
        {QStringLiteral("ui.choose"), {}}, {QStringLiteral("ui.pickFile"), {}}, {QStringLiteral("ui.pickFolder"), {}},
        {QStringLiteral("ui.saveFile"), {}}, {QStringLiteral("ui.openUrl"), {}},
        {QStringLiteral("ui.progressBegin"), {}},
        {QStringLiteral("ui.showForm"), QStringLiteral("ui.dialogs")},
        {QStringLiteral("ui.showDialog"), QStringLiteral("ui.dialogs")},
        {QStringLiteral("ui.clipboardText"), QStringLiteral("clipboard")},
        {QStringLiteral("app.projectFolder"), {}},
        {QStringLiteral("app.navigate"), QStringLiteral("app.navigate")},
        {QStringLiteral("secrets.get"), {}}, {QStringLiteral("secrets.set"), {}}, {QStringLiteral("secrets.remove"), {}},
        {QStringLiteral("files.openWithSystem"), QStringLiteral("files")},
        {QStringLiteral("files.reveal"), QStringLiteral("files")},
        {QStringLiteral("email.backends"), QStringLiteral("email.compose")},
        {QStringLiteral("email.compose"), QStringLiteral("email.compose")},
        {QStringLiteral("email.audiences"), QStringLiteral("email.compose")},
        {QStringLiteral("email.resolveAudience"), QStringLiteral("email.compose")},
        {QStringLiteral("email.templates"), QStringLiteral("email.compose")},
        {QStringLiteral("email.renderTemplate"), QStringLiteral("email.compose")},
        {QStringLiteral("calendar.backends"), QStringLiteral("calendar.draft")},
        {QStringLiteral("calendar.draftMeeting"), QStringLiteral("calendar.draft")},
        {QStringLiteral("calendar.writeIcs"), QStringLiteral("calendar.draft")},
        {QStringLiteral("contacts.sources"), QStringLiteral("contacts.sync")},
        {QStringLiteral("contacts.syncNow"), QStringLiteral("contacts.sync")},
        {QStringLiteral("contacts.pick"), QStringLiteral("contacts.pick")},
        {QStringLiteral("contacts.toVCard"), QStringLiteral("contacts.pick")},
        {QStringLiteral("contacts.fromVCard"), QStringLiteral("contacts.pick")},
        {QStringLiteral("office365.status"), QStringLiteral("office365")},
        {QStringLiteral("office365.signIn"), QStringLiteral("office365")},
        {QStringLiteral("office365.graph"), QStringLiteral("office365")},
        {QStringLiteral("process.launch"), QStringLiteral("process.launch")},
        {QStringLiteral("reports.generate"), QStringLiteral("reports")},
    };
    return methods;
}

} // namespace

// ── HostBridge ───────────────────────────────────────────────────────────────

int HostBridge::call(const QString& method, const QJSValue& args)
{
    return m_runtime->callAsync(method, args.toVariant().toList());
}

QJSValue HostBridge::callSync(const QString& method, const QJSValue& args)
{
    return m_runtime->callSync(method, args.toVariant().toList());
}

void HostBridge::log(const QString& level, const QString& message)
{
    const QString line = QStringLiteral("[%1] %2").arg(m_runtime->config().manifest.id, message);
    if (level == QLatin1String("error"))
        QLog_Error(ERRORLOG, line);
    else
        QLog_Info(CONSOLELOG, line);
    emit m_runtime->logMessage(level, message);
}

void HostBridge::reportError(const QString& where, const QString& message, const QString& stack)
{
    QLog_Error(ERRORLOG, QStringLiteral("[%1] %2: %3%4").arg(m_runtime->config().manifest.id, where, message,
                                                             stack.isEmpty() ? QString() : QStringLiteral("\n") + stack));
    emit m_runtime->errorReported(where, message);
}

void HostBridge::reply(int replyId, bool ok, const QJSValue& value)
{
    m_runtime->deliverReply(replyId, ok, value.toVariant());
}

// ── PluginRuntime ────────────────────────────────────────────────────────────

PluginRuntime::PluginRuntime(PluginHostConfig config, PluginServices* services)
    : m_config(std::move(config)), m_services(services)
{
}

PluginRuntime::~PluginRuntime()
{
    stop();
}

void PluginRuntime::interrupt()
{
    QMutexLocker locker(&m_engineMutex);
    if (m_engine)
        m_engine->setInterrupted(true);
}

QJSValue PluginRuntime::toJs(const QVariant& value) const
{
    return toJsValue(m_engine, value);
}

bool PluginRuntime::callHook(const char* name, const QJSValueList& args, QJSValue* result)
{
    if (!m_engine || m_hooks.isUndefined())
        return false;
    QJSValue fn = m_hooks.property(QLatin1String(name));
    if (!fn.isCallable())
        return false;
    m_busyWhere = QLatin1String(name);
    m_busySince.store(nowMs());
    QJSValue r = fn.call(args);
    m_busySince.store(0);
    if (m_engine->isInterrupted()) {
        m_engine->setInterrupted(false);
        QLog_Error(ERRORLOG, QStringLiteral("[%1] stopped a call that ran too long (%2).")
                                 .arg(m_config.manifest.id, QLatin1String(name)));
        emit hung(QLatin1String(name));
        return false;
    }
    if (r.isError()) {
        const QString message = r.toString();
        QLog_Error(ERRORLOG, QStringLiteral("[%1] %2: %3").arg(m_config.manifest.id, QLatin1String(name), message));
        emit errorReported(QLatin1String(name), message);
        return false;
    }
    if (result)
        *result = r;
    return true;
}

bool PluginRuntime::start(QString* error)
{
    const PluginManifest& manifest = m_config.manifest;
    QDir().mkpath(m_config.dataPath);
    QDir().mkpath(m_config.tempPath);

    m_settings = std::make_unique<QSettings>(m_config.settingsOrganization, QStringLiteral("PluginSettings"));
    m_settings->setFallbacksEnabled(false);

    // One-time copy of the settings of the Python plugin this replaces.
    for (const auto& [group, prefix] : manifest.legacySettings) {
        const QString flag = QStringLiteral("%1/_migrated/%2").arg(manifest.id, QString(group).replace(QLatin1Char('/'), QLatin1Char('_')));
        if (group.isEmpty() || m_settings->value(flag).toBool())
            continue;
        m_settings->beginGroup(group);
        const QStringList keys = m_settings->allKeys();
        QVariantMap values;
        for (const QString& key : keys)
            values.insert(key, m_settings->value(key));
        m_settings->endGroup();
        m_settings->beginGroup(manifest.id);
        for (auto it = values.constBegin(); it != values.constEnd(); ++it)
            if (!m_settings->contains(prefix + it.key()))
                m_settings->setValue(prefix + it.key(), it.value());
        m_settings->endGroup();
        m_settings->setValue(flag, true);
        QLog_Info(CONSOLELOG, QStringLiteral("[%1] migrated %2 setting(s) from \"%3\".")
                                  .arg(manifest.id).arg(values.size()).arg(group));
    }
    m_settings->sync();

    m_data = std::make_unique<DataService>(manifest);
    if (!m_config.databasePath.isEmpty()) {
        PluginError dataError;
        if (!m_data->open(m_config.databasePath, &dataError))
            QLog_Error(ERRORLOG, QStringLiteral("[%1] %2").arg(manifest.id, dataError.message));
        m_data->setRowChangedCallback([this](const QString& table, const QString& key, int operation) {
            emit rowsChanged({QVariantMap{{QStringLiteral("table"), table}, {QStringLiteral("key"), key},
                                          {QStringLiteral("operation"), operation}}});
        });
    }

    m_network = new QNetworkAccessManager(this);
    m_com = ComBridge::create();

    {
        QMutexLocker locker(&m_engineMutex);
        m_engine = new QJSEngine(this);
    }
    m_bridge = new HostBridge(this);
    // The engine must not adopt the bridge: it is deleted explicitly in stop().
    QJSEngine::setObjectOwnership(m_bridge, QJSEngine::CppOwnership);

    const QString prelude = readResource(QStringLiteral(":/projectnotes/plugins/prelude.js"));
    QJSValue factory = m_engine->evaluate(prelude, QStringLiteral("projectnotes:prelude.js"));
    if (factory.isError() || !factory.isCallable()) {
        *error = QStringLiteral("The plugin API failed to initialise: %1 (prelude line %2)")
                     .arg(factory.toString(), factory.property(QStringLiteral("lineNumber")).toString());
        return false;
    }

    QStringList capabilities = m_config.capabilities;
    capabilities << QStringLiteral("data") << QStringLiteral("files") << QStringLiteral("http");
    if (m_com && m_com->available())
        capabilities << QStringLiteral("com");
    capabilities.removeDuplicates();

    const QVariantMap info {
        {QStringLiteral("id"), manifest.id}, {QStringLiteral("name"), manifest.name},
        {QStringLiteral("version"), manifest.version}, {QStringLiteral("apiVersion"), manifest.apiVersion},
        {QStringLiteral("bundled"), manifest.bundled}, {QStringLiteral("packagePath"), manifest.folder},
        {QStringLiteral("dataPath"), m_config.dataPath}, {QStringLiteral("appVersion"), m_config.appVersion},
        {QStringLiteral("platform"), currentPlatform()}, {QStringLiteral("buildFlavor"), m_config.buildFlavor},
        {QStringLiteral("profile"), m_config.profile}, {QStringLiteral("locale"), QLocale().name()},
        {QStringLiteral("capabilities"), capabilities}};

    m_hooks = factory.call({m_engine->newQObject(m_bridge), toJs(info)});
    if (m_hooks.isError()) {
        *error = QStringLiteral("The plugin API failed to initialise: %1").arg(m_hooks.toString());
        return false;
    }

    // Globals: console and timers. Nothing else of the host is global.
    const QJSValue globals = m_hooks.property(QStringLiteral("globals"));
    for (const QString& name : {QStringLiteral("console"), QStringLiteral("setTimeout"), QStringLiteral("setInterval"),
                                QStringLiteral("clearTimeout"), QStringLiteral("clearInterval")})
        m_engine->globalObject().setProperty(name, globals.property(name));

    m_engine->registerModule(QStringLiteral("projectnotes"), m_hooks.property(QStringLiteral("api")));
    m_engine->registerModule(QStringLiteral("projectnotes/common"), m_hooks.property(QStringLiteral("common")));

    const QString mainPath = QDir(manifest.folder).filePath(manifest.main);
    m_busyWhere = QStringLiteral("module load");
    m_busySince.store(nowMs());
    QJSValue ns = m_engine->importModule(mainPath);
    m_busySince.store(0);
    if (m_engine->isInterrupted()) {
        m_engine->setInterrupted(false);
        *error = QStringLiteral("Loading %1 took too long and was stopped.").arg(manifest.main);
        return false;
    }
    if (ns.isError()) {
        *error = QStringLiteral("%1 failed to load: %2 (line %3)")
                     .arg(manifest.main, ns.toString(), ns.property(QStringLiteral("lineNumber")).toString());
        return false;
    }
    callHook("setModule", {ns});
    m_started = true;
    callHook("callExport", {QJSValue(QStringLiteral("onStart")), m_engine->newArray()});
    return true;
}

void PluginRuntime::stop()
{
    if (m_stopping)
        return;
    m_stopping = true;

    if (m_started && m_engine) {
        // Give onStop up to three seconds to finish (it may be async). Promise
        // reactions run from the event loop, so pump it while waiting.
        QJSValue promise;
        if (callHook("callExport", {QJSValue(QStringLiteral("onStop")), m_engine->newArray()}, &promise)
            && promise.property(QStringLiteral("then")).isCallable()) {
            QJSValue tracker = m_engine->newObject();
            tracker.setProperty(QStringLiteral("done"), false);
            const QJSValue mark = m_engine->evaluate(QStringLiteral("(function(t){ return function(){ t.done = true; }; })"))
                                      .call({tracker});
            promise.property(QStringLiteral("then")).callWithInstance(promise, {mark, mark});
            QElapsedTimer waited;
            waited.start();
            QEventLoop loop;
            while (!tracker.property(QStringLiteral("done")).toBool() && waited.elapsed() < 3000)
                loop.processEvents(QEventLoop::AllEvents, 20);
        }
    }
    m_started = false;

    qDeleteAll(m_timers);
    m_timers.clear();
    if (m_network) {
        const auto replies = m_network->findChildren<QNetworkReply*>();
        for (QNetworkReply* reply : replies)
            reply->abort();
    }
    if (m_com)
        m_com->releaseAll();
    for (auto it = m_replies.begin(); it != m_replies.end(); ++it)
        it.value()({}, PluginError::make(QStringLiteral("cancelled"), QStringLiteral("The plugin stopped.")));
    m_replies.clear();
    if (m_data)
        m_data->close();

    m_hooks = QJSValue();
    {
        QMutexLocker locker(&m_engineMutex);
        delete m_engine;
        m_engine = nullptr;
    }
    delete m_bridge;
    m_bridge = nullptr;
    m_data.reset();
    m_com.reset();
    m_settings.reset();
}

// ── Calls from JavaScript ────────────────────────────────────────────────────

int PluginRuntime::callAsync(const QString& method, const QVariantList& args)
{
    const int id = m_nextRequest++;
    QPointer<PluginRuntime> self(this);
    PluginError error;
    if (!checkPermission(method, args, &error)) {
        // Reject after the promise is registered.
        QTimer::singleShot(0, this, [self, id, error] { if (self) self->complete(id, {}, error); });
        return id;
    }
    handleAsync(method, args, [self, id](const QVariant& value, const PluginError& error) {
        if (!self)
            return;
        if (QThread::currentThread() == self->thread())
            QTimer::singleShot(0, self, [self, id, value, error] { if (self) self->complete(id, value, error); });
        else
            QMetaObject::invokeMethod(self, [self, id, value, error] { if (self) self->complete(id, value, error); },
                                      Qt::QueuedConnection);
    });
    return id;
}

void PluginRuntime::complete(int id, const QVariant& value, const PluginError& error)
{
    if (!m_engine || m_stopping)
        return;
    if (error.isError())
        callHook("reject", {QJSValue(id), toJs(error.toMap())});
    else
        callHook("resolve", {QJSValue(id), toJs(value)});
}

QJSValue PluginRuntime::callSync(const QString& method, const QVariantList& args)
{
    PluginError error;
    QVariant value;
    if (checkPermission(method, args, &error))
        value = handleSync(method, args, &error);
    QJSValue result = m_engine->newObject();
    if (error.isError())
        result.setProperty(QStringLiteral("error"), toJs(error.toMap()));
    else
        result.setProperty(QStringLiteral("value"), toJs(value));
    return result;
}

bool PluginRuntime::checkPermission(const QString& method, const QVariantList& args, PluginError* error) const
{
    const PluginManifest& m = m_config.manifest;
    const auto forwarded = forwardedMethods().constFind(method);
    if (forwarded != forwardedMethods().constEnd()) {
        if (!forwarded->isEmpty() && !m.hasPermission(*forwarded)) {
            *error = PluginError::denied(*forwarded);
            return false;
        }
        return true;
    }
    if (method == QLatin1String("ui.clipboardSetText") && !m.hasPermission(QStringLiteral("clipboard"))) {
        *error = PluginError::denied(QStringLiteral("clipboard"));
        return false;
    }
    if (method.startsWith(QLatin1String("files.")) && method != QLatin1String("files.packagePath")
        && method != QLatin1String("files.tempPath") && !m.hasPermission(QStringLiteral("files"))) {
        *error = PluginError::denied(QStringLiteral("files"));
        return false;
    }
    if (method.startsWith(QLatin1String("http.")) && !m.hasPermission(QStringLiteral("http"))) {
        *error = PluginError::denied(QStringLiteral("http"));
        return false;
    }
    if (method.startsWith(QLatin1String("com.")) && !m.hasPermission(QStringLiteral("com"))) {
        *error = PluginError::denied(QStringLiteral("com"));
        return false;
    }
    Q_UNUSED(args);
    return true;
}

void PluginRuntime::forward(const QString& method, const QVariantList& args, Reply reply)
{
    if (!m_services) {
        reply({}, PluginError::make(QStringLiteral("unavailable"), QStringLiteral("%1 is not available here.").arg(method)));
        return;
    }
    QPointer<PluginServices> services = m_services;
    const PluginManifest manifest = m_config.manifest;
    QPointer<PluginRuntime> self(this);
    QMetaObject::invokeMethod(services, [services, manifest, method, args, reply, self]() {
        if (!services)
            return;
        services->request(manifest, method, args, [reply, self, method](const QVariant& value, const PluginError& error) {
            // Paths the user picked, or a resolved project folder, become
            // readable and writable for the rest of this run.
            if (self && !error.isError()
                && (method == QLatin1String("ui.pickFile") || method == QLatin1String("ui.pickFolder")
                    || method == QLatin1String("ui.saveFile") || method == QLatin1String("app.projectFolder"))) {
                QStringList paths = value.typeId() == QMetaType::QVariantList ? value.toStringList()
                                                                              : QStringList{value.toString()};
                for (const QString& p : std::as_const(paths))
                    if (!p.isEmpty())
                        QMetaObject::invokeMethod(self, [self, p] { if (self) self->setGrantedPath(p); }, Qt::QueuedConnection);
            }
            reply(value, error);
        });
    }, Qt::QueuedConnection);
}

void PluginRuntime::setGrantedPath(const QString& path)
{
    const QString clean = cleanAbsolute(path);
    if (!m_grantedPaths.contains(clean))
        m_grantedPaths.append(clean);
}

void PluginRuntime::setProgressCancelled(const QString& token)
{
    m_cancelledProgress.insert(token);
}

void PluginRuntime::handleAsync(const QString& method, const QVariantList& args, Reply reply)
{
    auto arg = [&args](int i) { return i < args.size() ? args.at(i) : QVariant(); };
    PluginError error;

    if (method.startsWith(QLatin1String("data."))) {
        if (!m_data || !m_data->database()) {
            reply({}, PluginError::make(QStringLiteral("unavailable"), QStringLiteral("No database is open.")));
            return;
        }
        const QString op = method.mid(5);
        QVariant value;
        if (op == QLatin1String("entities")) value = m_data->entities(&error);
        else if (op == QLatin1String("schema")) value = m_data->schema(arg(0).toString(), &error);
        else if (op == QLatin1String("get")) value = m_data->get(arg(0).toString(), arg(1), arg(2).toMap(), &error);
        else if (op == QLatin1String("list")) value = m_data->list(arg(0).toString(), arg(1).toMap(), &error);
        else if (op == QLatin1String("count")) value = m_data->count(arg(0).toString(), arg(1).toMap(), &error);
        else if (op == QLatin1String("search")) value = m_data->search(arg(0).toString(), arg(1).toMap(), &error);
        else if (op == QLatin1String("save")) value = m_data->save(arg(0).toString(), arg(1).toMap(), &error);
        else if (op == QLatin1String("saveAll")) value = m_data->saveAll(arg(0).toString(), arg(1).toList(), &error);
        else if (op == QLatin1String("remove")) value = m_data->remove(arg(0).toString(), arg(1), &error);
        else if (op == QLatin1String("begin")) m_data->begin(&error);
        else if (op == QLatin1String("commit")) m_data->commit(&error);
        else if (op == QLatin1String("rollback")) m_data->rollback();
        else if (op == QLatin1String("exportXml")) value = m_data->exportXml(arg(0).toString(), arg(1).toString(), arg(2).toMap(), &error);
        else if (op == QLatin1String("importXml")) value = m_data->importXml(arg(0).toString(), &error);
        else error = PluginError::make(QStringLiteral("not-found"), QStringLiteral("Unknown method %1.").arg(method));
        reply(value, error);
        return;
    }

    if (method.startsWith(QLatin1String("storage."))) {
        const QString op = method.mid(8);
        QString path;
        if (!resolveStoragePath(arg(0).toString(), &path, &error)) {
            reply({}, error);
            return;
        }
        if (op == QLatin1String("readText")) {
            QFile file(path);
            if (!file.exists()) { reply(QVariant::fromValue(nullptr), {}); return; }
            if (!file.open(QIODevice::ReadOnly)) { reply({}, PluginError::make(QStringLiteral("io-error"), file.errorString())); return; }
            reply(QString::fromUtf8(file.readAll()), {});
        } else if (op == QLatin1String("writeText")) {
            QDir().mkpath(QFileInfo(path).absolutePath());
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) { reply({}, PluginError::make(QStringLiteral("io-error"), file.errorString())); return; }
            file.write(arg(1).toString().toUtf8());
            reply(QVariant(), {});
        } else if (op == QLatin1String("remove")) {
            QFileInfo info(path);
            const bool ok = info.isDir() ? QDir(path).removeRecursively() : (!info.exists() || QFile::remove(path));
            reply(QVariant(), ok ? PluginError() : PluginError::make(QStringLiteral("io-error"), QStringLiteral("Could not remove %1.").arg(arg(0).toString())));
        } else if (op == QLatin1String("list")) {
            QStringList names;
            QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            while (it.hasNext()) { it.next(); names.append(it.fileName()); }
            names.sort();
            reply(names, {});
        } else {
            reply({}, PluginError::make(QStringLiteral("not-found"), QStringLiteral("Unknown method %1.").arg(method)));
        }
        return;
    }

    if (method.startsWith(QLatin1String("files.")) && !forwardedMethods().contains(method)) {
        const QString op = method.mid(6);
        const QString path = arg(0).toString();
        const bool writes = op == QLatin1String("writeText") || op == QLatin1String("writeBytes") || op == QLatin1String("mkdir")
                         || op == QLatin1String("remove");
        if (op == QLatin1String("copy") || op == QLatin1String("move")) {
            const QString to = arg(1).toString();
            if (!checkFilePath(path, op == QLatin1String("move"), &error) || !checkFilePath(to, true, &error)) {
                reply({}, error);
                return;
            }
            const bool overwrite = arg(2).toMap().value(QStringLiteral("overwrite")).toBool();
            if (QFileInfo::exists(to)) {
                if (!overwrite) { reply({}, PluginError::make(QStringLiteral("exists"), QStringLiteral("%1 already exists.").arg(to))); return; }
                QFile::remove(to);
            }
            QDir().mkpath(QFileInfo(to).absolutePath());
            const bool ok = op == QLatin1String("copy") ? QFile::copy(path, to) : QFile::rename(path, to);
            reply(QVariant(), ok ? PluginError() : PluginError::make(QStringLiteral("io-error"), QStringLiteral("Could not %1 %2 to %3.").arg(op, path, to)));
            return;
        }
        if (!checkFilePath(path, writes, &error)) {
            reply({}, error);
            return;
        }
        const QFileInfo info(path);
        if (op == QLatin1String("exists")) {
            reply(info.exists(), {});
        } else if (op == QLatin1String("stat")) {
            if (!info.exists()) { reply(QVariant::fromValue(nullptr), {}); return; }
            reply(QVariantMap{{QStringLiteral("path"), info.absoluteFilePath()}, {QStringLiteral("name"), info.fileName()},
                              {QStringLiteral("isDir"), info.isDir()}, {QStringLiteral("size"), info.size()},
                              {QStringLiteral("modified"), info.lastModified().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss"))}}, {});
        } else if (op == QLatin1String("list")) {
            const QVariantMap options = arg(1).toMap();
            const QStringList filters = options.value(QStringLiteral("filter")).toStringList();
            QDirIterator it(path, filters, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                            options.value(QStringLiteral("recursive")).toBool() ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
            QVariantList out;
            while (it.hasNext()) {
                it.next();
                const QFileInfo f = it.fileInfo();
                out.append(QVariantMap{{QStringLiteral("path"), f.absoluteFilePath()}, {QStringLiteral("name"), f.fileName()},
                                       {QStringLiteral("isDir"), f.isDir()}, {QStringLiteral("size"), f.size()},
                                       {QStringLiteral("modified"), f.lastModified().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss"))}});
            }
            reply(out, {});
        } else if (op == QLatin1String("readText") || op == QLatin1String("readBytes")) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) { reply({}, PluginError::make(QStringLiteral("io-error"), file.errorString())); return; }
            const QByteArray bytes = file.readAll();
            reply(op == QLatin1String("readText") ? QVariant(QString::fromUtf8(bytes)) : QVariant(bytes), {});
        } else if (op == QLatin1String("writeText") || op == QLatin1String("writeBytes")) {
            QDir().mkpath(info.absolutePath());
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) { reply({}, PluginError::make(QStringLiteral("io-error"), file.errorString())); return; }
            file.write(op == QLatin1String("writeText") ? arg(1).toString().toUtf8() : arg(1).toByteArray());
            reply(QVariant(), {});
        } else if (op == QLatin1String("mkdir")) {
            reply(QVariant(), QDir().mkpath(path) ? PluginError() : PluginError::make(QStringLiteral("io-error"), QStringLiteral("Could not create %1.").arg(path)));
        } else if (op == QLatin1String("remove")) {
            const bool ok = info.isDir() ? QDir(path).removeRecursively() : (!info.exists() || QFile::remove(path));
            reply(QVariant(), ok ? PluginError() : PluginError::make(QStringLiteral("io-error"), QStringLiteral("Could not remove %1.").arg(path)));
        } else {
            reply({}, PluginError::make(QStringLiteral("not-found"), QStringLiteral("Unknown method %1.").arg(method)));
        }
        return;
    }

    if (method == QLatin1String("http.fetch")) {
        fetch(arg(0).toString(), arg(1).toMap(), reply);
        return;
    }
    if (method == QLatin1String("http.bodyBytes")) {
        const int id = arg(0).toInt();
        if (!m_bodies.contains(id)) {
            reply({}, PluginError::make(QStringLiteral("not-found"), QStringLiteral("The response body is no longer available.")));
            return;
        }
        reply(m_bodies.value(id), {});
        return;
    }

    if (forwardedMethods().contains(method)) {
        if ((method == QLatin1String("files.openWithSystem") || method == QLatin1String("files.reveal"))
            && !checkFilePath(arg(0).toString(), false, &error)) {
            reply({}, error);
            return;
        }
        // Paths handed to integrations follow the same rules as files.*.
        if (method == QLatin1String("email.compose")) {
            for (const QString& path : arg(0).toMap().value(QStringLiteral("attachments")).toStringList()) {
                if (!checkFilePath(path, false, &error)) {
                    reply({}, error);
                    return;
                }
            }
        }
        if (method == QLatin1String("calendar.writeIcs") && !checkFilePath(arg(1).toString(), true, &error)) {
            reply({}, error);
            return;
        }
        forward(method, args, reply);
        return;
    }

    reply({}, PluginError::make(QStringLiteral("not-found"), QStringLiteral("Unknown method %1.").arg(method)));
}

QVariant PluginRuntime::handleSync(const QString& method, const QVariantList& args, PluginError* error)
{
    auto arg = [&args](int i) { return i < args.size() ? args.at(i) : QVariant(); };
    const QString pluginGroup = m_config.manifest.id;

    if (method == QLatin1String("timer.start")) {
        const int id = arg(0).toInt();
        delete m_timers.take(id);
        auto* timer = new QTimer(this);
        timer->setSingleShot(!arg(2).toBool());
        timer->setInterval(qMax(0, arg(1).toInt()));
        QPointer<PluginRuntime> self(this);
        connect(timer, &QTimer::timeout, this, [this, id, timer] {
            if (timer->isSingleShot()) {
                m_timers.remove(id);
                timer->deleteLater();
            }
            callHook("fireTimer", {QJSValue(id)});
        });
        m_timers.insert(id, timer);
        timer->start();
        return {};
    }
    if (method == QLatin1String("timer.stop")) {
        if (QTimer* timer = m_timers.take(arg(0).toInt())) {
            timer->stop();
            timer->deleteLater();
        }
        return {};
    }

    if (method.startsWith(QLatin1String("settings."))) {
        const QString op = method.mid(9);
        m_settings->beginGroup(pluginGroup);
        QVariant value;
        if (op == QLatin1String("get")) {
            value = m_settings->value(arg(0).toString());
            if (!value.isValid())
                value = QVariant::fromValue(nullptr);
        } else if (op == QLatin1String("set")) {
            m_settings->setValue(arg(0).toString(), arg(1));
        } else if (op == QLatin1String("remove")) {
            m_settings->remove(arg(0).toString());
        } else if (op == QLatin1String("keys")) {
            QStringList keys = m_settings->allKeys();
            keys.erase(std::remove_if(keys.begin(), keys.end(),
                                      [](const QString& k) { return k.startsWith(QLatin1String("_migrated/")); }), keys.end());
            value = keys;
        } else if (op == QLatin1String("all")) {
            QVariantMap all;
            for (const QString& key : m_settings->allKeys())
                if (!key.startsWith(QLatin1String("_migrated/")))
                    all.insert(key, m_settings->value(key));
            value = all;
        }
        m_settings->endGroup();
        if (op == QLatin1String("set") || op == QLatin1String("remove"))
            m_settings->sync();
        return value;
    }

    if (method == QLatin1String("menus.register")) {
        const QVariantList items = arg(0).toList();
        for (const QVariant& item : items) {
            const QVariantMap menu = item.toMap();
            const QString id = menu.value(QStringLiteral("id")).toString();
            const QString context = menu.value(QStringLiteral("context"), QStringLiteral("global")).toString();
            if (context != QLatin1String("record") && context != QLatin1String("global")) {
                *error = PluginError::make(QStringLiteral("validation-failed"), QStringLiteral("Menu \"%1\" context must be \"record\" or \"global\".").arg(id));
                return {};
            }
            if (context == QLatin1String("record") && menu.value(QStringLiteral("entity")).toString().isEmpty()) {
                *error = PluginError::make(QStringLiteral("validation-failed"), QStringLiteral("Record menu \"%1\" needs an entity.").arg(id));
                return {};
            }
            for (int i = int(m_dynamicMenus.size()) - 1; i >= 0; --i)
                if (m_dynamicMenus.at(i).toMap().value(QStringLiteral("id")).toString() == id)
                    m_dynamicMenus.removeAt(i);
            m_dynamicMenus.append(menu);
        }
        emit dynamicMenusChanged(m_dynamicMenus);
        return {};
    }
    if (method == QLatin1String("menus.unregister")) {
        const QStringList ids = arg(0).toStringList();
        for (int i = int(m_dynamicMenus.size()) - 1; i >= 0; --i)
            if (ids.contains(m_dynamicMenus.at(i).toMap().value(QStringLiteral("id")).toString()))
                m_dynamicMenus.removeAt(i);
        emit dynamicMenusChanged(m_dynamicMenus);
        return {};
    }
    if (method == QLatin1String("menus.clear")) {
        m_dynamicMenus.clear();
        emit dynamicMenusChanged(m_dynamicMenus);
        return {};
    }
    if (method == QLatin1String("menus.list")) {
        QVariantList out;
        for (const MenuContribution& menu : m_config.manifest.menus)
            out.append(QVariantMap{{QStringLiteral("id"), menu.id}, {QStringLiteral("title"), menu.title},
                                   {QStringLiteral("context"), menu.context}, {QStringLiteral("entity"), menu.entity},
                                   {QStringLiteral("submenu"), menu.submenu}, {QStringLiteral("include"), menu.include},
                                   {QStringLiteral("parameter"), menu.parameter}});
        out.append(m_dynamicMenus);
        return out;
    }

    if (method == QLatin1String("files.packagePath")) {
        const QString path = QDir::cleanPath(QDir(m_config.manifest.folder).filePath(arg(0).toString()));
        if (!isUnder(path, m_config.manifest.folder)) {
            *error = PluginError::make(QStringLiteral("validation-failed"), QStringLiteral("The path leaves the plugin folder."));
            return {};
        }
        return path;
    }
    if (method == QLatin1String("files.tempPath")) {
        const QString name = QFileInfo(arg(0).toString()).fileName();
        return QDir(m_config.tempPath).filePath(name.isEmpty() ? QStringLiteral("temp") : name);
    }

    if (method == QLatin1String("app.currentView"))
        return m_services ? m_services->currentView() : QVariantMap();

    // Fire-and-forget calls to the application.
    if (method == QLatin1String("app.refresh") || method == QLatin1String("ui.notify")
        || method == QLatin1String("ui.clipboardSetText") || method == QLatin1String("ui.progressUpdate")
        || method == QLatin1String("ui.progressEnd")) {
        forward(method, args, [](const QVariant&, const PluginError&) {});
        if (method == QLatin1String("ui.progressUpdate"))
            return m_cancelledProgress.contains(arg(0).toString());
        if (method == QLatin1String("ui.progressEnd"))
            m_cancelledProgress.remove(arg(0).toString());
        return {};
    }
    if (method == QLatin1String("ui.progressCancelled"))
        return m_cancelledProgress.contains(arg(0).toString());

    if (method.startsWith(QLatin1String("com."))) {
        if (!m_com || !m_com->available()) {
            *error = PluginError::make(QStringLiteral("unavailable"), QStringLiteral("COM automation is only available on Windows."));
            return {};
        }
        const QString op = method.mid(4);
        if (op == QLatin1String("createObject") || op == QLatin1String("getActiveObject") || op == QLatin1String("getObject")) {
            const QString name = arg(0).toString();
            if (!m_config.manifest.allowsComObject(name)) {
                *error = PluginError::make(QStringLiteral("permission-denied"),
                                           QStringLiteral("\"%1\" is not listed in the manifest's \"com\" array.").arg(name),
                                           {{QStringLiteral("object"), name}});
                error->permission = QStringLiteral("com");
                return {};
            }
            if (op == QLatin1String("createObject")) return m_com->createObject(name, error);
            if (op == QLatin1String("getActiveObject")) return m_com->getActiveObject(name, error);
            return m_com->getObject(name, error);
        }
        if (op == QLatin1String("call")) return m_com->call(arg(0).toString(), arg(1).toString(), arg(2).toList(), error);
        if (op == QLatin1String("get")) return m_com->get(arg(0).toString(), arg(1).toString(), arg(2).toList(), error);
        if (op == QLatin1String("set")) { m_com->set(arg(0).toString(), arg(1).toString(), arg(2), arg(3).toList(), error); return {}; }
        if (op == QLatin1String("enumerate")) return m_com->enumerate(arg(0).toString(), error);
        if (op == QLatin1String("release")) { m_com->release(arg(0).toString()); return {}; }
    }

    *error = PluginError::make(QStringLiteral("not-found"), QStringLiteral("Unknown method %1.").arg(method));
    return {};
}

// ── Paths ────────────────────────────────────────────────────────────────────

bool PluginRuntime::resolveStoragePath(const QString& relative, QString* absolute, PluginError* error) const
{
    const QString path = QDir::cleanPath(QDir(m_config.dataPath).filePath(relative));
    if (QDir::isAbsolutePath(relative) || !isUnder(path, m_config.dataPath)) {
        *error = PluginError::make(QStringLiteral("validation-failed"), QStringLiteral("Storage paths must stay inside the plugin's data folder."));
        return false;
    }
    *absolute = path;
    return true;
}

bool PluginRuntime::checkFilePath(const QString& path, bool write, PluginError* error) const
{
    if (path.isEmpty() || !QDir::isAbsolutePath(path)) {
        *error = PluginError::make(QStringLiteral("validation-failed"), QStringLiteral("files.* needs an absolute path."));
        return false;
    }
    if (isUnder(path, m_config.dataPath) || isUnder(path, m_config.tempPath))
        return true;
    if (isUnder(path, m_config.manifest.folder)) {
        if (!write)
            return true;
        *error = PluginError::make(QStringLiteral("permission-denied"), QStringLiteral("The plugin's own folder is read-only."));
        error->permission = QStringLiteral("files");
        return false;
    }
    for (const QString& granted : m_grantedPaths) {
        // A picked file grants that file and its folder; a picked folder
        // grants everything beneath it.
        if (isUnder(path, granted) || isUnder(path, QFileInfo(granted).absolutePath()))
            return true;
    }
    *error = PluginError::make(QStringLiteral("permission-denied"),
                               QStringLiteral("%1 is outside the locations this plugin may use. Ask the user to pick it "
                                              "with ui.pickFile or ui.pickFolder, or use app.projectFolder.").arg(path));
    error->permission = QStringLiteral("files");
    return false;
}

// ── http ─────────────────────────────────────────────────────────────────────

void PluginRuntime::fetch(const QString& urlText, const QVariantMap& options, Reply reply)
{
    const QUrl url(urlText);
    if (!url.isValid() || (url.scheme() != QLatin1String("https") && url.scheme() != QLatin1String("http"))) {
        reply({}, PluginError::make(QStringLiteral("validation-failed"), QStringLiteral("fetch() needs an http or https URL.")));
        return;
    }
    if (!m_config.manifest.allowsHost(url.host())) {
        PluginError error = PluginError::make(QStringLiteral("permission-denied"),
                                              QStringLiteral("%1 is not listed in the manifest's \"http\" array.").arg(url.host()),
                                              {{QStringLiteral("host"), url.host()}});
        error.permission = QStringLiteral("http");
        reply({}, error);
        return;
    }

    QNetworkRequest request(url);
    const QVariantMap headers = options.value(QStringLiteral("headers")).toMap();
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
        request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    const QVariantMap auth = options.value(QStringLiteral("auth")).toMap();
    if (auth.value(QStringLiteral("type")).toString() == QLatin1String("basic")) {
        const QByteArray credentials = (auth.value(QStringLiteral("username")).toString() + QLatin1Char(':')
                                        + auth.value(QStringLiteral("password")).toString()).toUtf8().toBase64();
        request.setRawHeader("Authorization", "Basic " + credentials);
    } else if (auth.value(QStringLiteral("type")).toString() == QLatin1String("bearer")) {
        request.setRawHeader("Authorization", "Bearer " + auth.value(QStringLiteral("token")).toString().toUtf8());
    }
    const int timeout = options.value(QStringLiteral("timeoutSeconds"), 60).toInt();
    request.setTransferTimeout(qMax(1, timeout) * 1000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    const QVariant body = options.value(QStringLiteral("body"));
    const QByteArray payload = body.typeId() == QMetaType::QByteArray ? body.toByteArray() : body.toString().toUtf8();
    const QByteArray verb = options.value(QStringLiteral("method"), QStringLiteral("GET")).toString().toUpper().toUtf8();

    QNetworkReply* networkReply = m_network->sendCustomRequest(request, verb, payload);
    QPointer<PluginRuntime> self(this);
    connect(networkReply, &QNetworkReply::finished, this, [self, networkReply, reply] {
        networkReply->deleteLater();
        if (!self)
            return;
        const int status = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 0 && networkReply->error() != QNetworkReply::NoError) {
            reply({}, PluginError::make(QStringLiteral("network"), networkReply->errorString()));
            return;
        }
        QVariantMap responseHeaders;
        for (const auto& pair : networkReply->rawHeaderPairs())
            responseHeaders.insert(QString::fromUtf8(pair.first).toLower(), QString::fromUtf8(pair.second));
        const QByteArray bytes = networkReply->readAll();
        const int bodyId = self->m_nextBody++;
        self->m_bodies.insert(bodyId, bytes);
        // Keep only the most recent bodies for arrayBuffer().
        while (self->m_bodies.size() > 8)
            self->m_bodies.remove(self->m_bodies.keys().first());
        reply(QVariantMap{{QStringLiteral("status"), status}, {QStringLiteral("headers"), responseHeaders},
                          {QStringLiteral("body"), QString::fromUtf8(bytes)}, {QStringLiteral("bodyId"), bodyId}}, {});
    });
}

// ── Calls into the plugin ────────────────────────────────────────────────────

void PluginRuntime::dispatchMenu(const QString& menuId, const QString& entity, const QString& key,
                                 const QString& parameter, const QStringList& include)
{
    if (!m_started)
        return;
    QVariantMap context {{QStringLiteral("menuId"), menuId},
                         {QStringLiteral("kind"), entity.isEmpty() ? QStringLiteral("global") : QStringLiteral("record")},
                         {QStringLiteral("entity"), entity.isEmpty() ? QVariant::fromValue(nullptr) : QVariant(entity)},
                         {QStringLiteral("key"), key.isEmpty() ? QVariant::fromValue(nullptr) : QVariant(key)},
                         {QStringLiteral("record"), QVariant::fromValue(nullptr)},
                         {QStringLiteral("parameter"), parameter}};
    if (!entity.isEmpty() && !key.isEmpty() && m_data && m_data->database()
        && m_config.manifest.canRead(entity)) {
        PluginError error;
        const QVariant record = m_data->get(entity, key, QVariantMap{{QStringLiteral("include"), include}}, &error);
        if (error.isError())
            QLog_Error(ERRORLOG, QStringLiteral("[%1] menu %2: %3").arg(m_config.manifest.id, menuId, error.message));
        else
            context.insert(QStringLiteral("record"), record);
    }
    callHook("menu", {QJSValue(menuId), toJs(context)});
}

void PluginRuntime::invoke(const QString& token, const QString& name, const QVariantList& args, Reply reply)
{
    if (!m_started) {
        reply({}, PluginError::make(QStringLiteral("unavailable"), QStringLiteral("The plugin is not running.")));
        return;
    }
    const int replyId = m_nextReply++;
    m_replies.insert(replyId, reply);
    if (!callHook("invoke", {QJSValue(replyId), QJSValue(token), QJSValue(name), toJs(args)}))
        deliverReply(replyId, false, PluginError::make(QStringLiteral("plugin-error"), QStringLiteral("The handler failed.")).toMap());
}

void PluginRuntime::deliverReply(int replyId, bool ok, const QVariant& value)
{
    Reply reply = m_replies.take(replyId);
    if (!reply)
        return;
    if (ok) {
        reply(value, {});
    } else {
        const QVariantMap e = value.toMap();
        reply({}, PluginError::make(e.value(QStringLiteral("code"), QStringLiteral("plugin-error")).toString(),
                                    e.value(QStringLiteral("message")).toString()));
    }
}

void PluginRuntime::deliver(const QString& kind, const QVariant& payload)
{
    if (!m_started)
        return;
    QVariant filtered = payload;
    if (kind == QLatin1String("data.changed")) {
        QVariantList visible;
        for (const QVariant& change : payload.toList())
            if (m_config.manifest.canRead(change.toMap().value(QStringLiteral("entity")).toString()))
                visible.append(change);
        if (visible.isEmpty())
            return;
        filtered = visible;
    }
    callHook("deliver", {QJSValue(kind), toJs(filtered)});
}

} // namespace PN::Plugins
