// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginUiController.h"

#include "DesktopPluginServices.h"
#include "credentialstore.h"
#include "ProjectNotesPlugins/JsConvert.h"
#include "ProjectNotesPlugins/PluginRegistry.h"

#include "QLogger.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSaveFile>
#include <QSet>
#include <QSettings>

using namespace QLogger;
using PN::Plugins::PluginError;
using PN::Plugins::PluginInfo;
using PN::Plugins::Reply;

namespace {
PluginUiController* s_instance = nullptr;

// A JavaScript object that QML passes to a QVariant parameter arrives as a
// QJSValue of the GUI engine. It must become plain maps and lists here, on
// the GUI thread: the plugin's own engine, on its own thread, cannot use it.
QVariant plainVariant(const QVariant& value)
{
    if (value.metaType() != QMetaType::fromType<QJSValue>())
        return value;
    const QJSValue js = value.value<QJSValue>();
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return js.toVariant(QJSValue::ConvertJSObjects);
#else
    return js.toVariant();
#endif
}

QVariantMap describe(const PluginInfo& info)
{
    const auto& m = info.manifest;
    return {{QStringLiteral("id"), m.id}, {QStringLiteral("name"), m.name.isEmpty() ? m.id : m.name},
            {QStringLiteral("version"), m.version}, {QStringLiteral("description"), m.description},
            {QStringLiteral("author"), m.author}, {QStringLiteral("homepage"), m.homepage},
            {QStringLiteral("bundled"), info.bundled}, {QStringLiteral("valid"), info.valid},
            {QStringLiteral("loadError"), info.loadError}, {QStringLiteral("enabled"), info.enabled},
            {QStringLiteral("needsApproval"), info.needsApproval}, {QStringLiteral("running"), info.running},
            {QStringLiteral("lastError"), info.lastError}, {QStringLiteral("platformSupported"), info.platformSupported},
            {QStringLiteral("platforms"), m.platforms}, {QStringLiteral("permissions"), m.permissions},
            {QStringLiteral("httpHosts"), m.httpHosts}, {QStringLiteral("comObjects"), m.comObjects},
            {QStringLiteral("office365Scopes"), m.office365Scopes},
            {QStringLiteral("shipsQml"), info.valid && m.shipsQml()}, {QStringLiteral("folder"), info.folder}};
}

const QString kSettingsBackupFormat = QStringLiteral("projectnotes-plugin-settings");
const QString kAllSettingsFormat = QStringLiteral("projectnotes-settings");

// Application settings a full settings backup leaves out: secrets, window and
// sidebar layout that belongs to this screen, one-time migration flags, and
// state tied to this computer's sign-ins. Sync stays off until the password
// and encryption phrase are entered again.
bool isPortableAppSettingKey(const QString& key)
{
    const QString name = key.section(QLatin1Char('/'), -1);
    for (const char* secret : {"password", "secret", "token", "phrase"})
        if (name.contains(QLatin1String(secret), Qt::CaseInsensitive))
            return false;
    for (const char* prefix : {"UI:", "ProjectFolders/Collapsed/", "FileFinder/v1/grantedScopes/", "_migrated/"})
        if (key.startsWith(QLatin1String(prefix)))
            return false;
    static const QSet<QString> machineKeys {
        QStringLiteral("Width"), QStringLiteral("Height"), QStringLiteral("X"), QStringLiteral("Y"),
        QStringLiteral("Maximized"), QStringLiteral("UserPluginsMoved"), QStringLiteral("PythonPluginsNoticeShown"),
        QStringLiteral("FileFinder/migrationComplete"), QStringLiteral("FileFinder/graphFolderState"),
        QStringLiteral("Sync/Enabled")};
    return !machineKeys.contains(key);
}

// Reads a settings backup file into `root`. Returns an error or "".
QString readSettingsFile(const QUrl& file, QJsonObject* root)
{
    const QString local = file.isLocalFile() ? file.toLocalFile() : file.toString();
    QFile in(local);
    if (!in.open(QIODevice::ReadOnly))
        return PluginUiController::tr("Could not open %1: %2").arg(QDir::toNativeSeparators(local), in.errorString());
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(in.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return PluginUiController::tr("%1 is not a Project Notes settings file.").arg(QDir::toNativeSeparators(local));
    *root = doc.object();
    return {};
}

// Writes `root` to `file` as indented JSON. Returns an error or "".
QString writeSettingsFile(const QUrl& file, const QJsonObject& root)
{
    const QString local = file.isLocalFile() ? file.toLocalFile() : file.toString();
    QSaveFile out(local);
    if (!out.open(QIODevice::WriteOnly) || out.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 || !out.commit())
        return PluginUiController::tr("Could not write %1: %2").arg(QDir::toNativeSeparators(local), out.errorString());
    return {};
}

// Bookkeeping the runtime writes next to a plugin's settings (legacy copy flags).
bool isInternalSettingKey(const QString& key)
{
    return key.startsWith(QLatin1String("_migrated/"));
}

// Keys of a plugin's "password" form fields, which a settings backup leaves out.
QSet<QString> passwordKeys(const PluginInfo& info)
{
    QSet<QString> keys;
    for (const auto& section : info.manifest.settings) {
        if (section.schema.isEmpty())
            continue;
        QFile file(QDir(info.folder).filePath(section.schema));
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QJsonArray fields = QJsonDocument::fromJson(file.readAll()).object().value(QStringLiteral("fields")).toArray();
        for (const QJsonValue& field : fields)
            if (field.toObject().value(QStringLiteral("type")).toString() == QLatin1String("password"))
                keys.insert(field.toObject().value(QStringLiteral("key")).toString());
    }
    return keys;
}

QJSValue makeDeferred(QJSEngine* engine)
{
    return engine->evaluate(QStringLiteral(
        "(function(){ var d = {}; d.promise = new Promise(function(res, rej){ d.resolve = res; d.reject = rej; }); return d; })()"));
}
} // namespace

PluginUiController::SecretWriter PluginUiController::s_secretWriter =
    [](const QString& pluginId, const QString& key, const QString& secret, QString* error) {
        return CredentialStore::write(DesktopPluginServices::secretService(pluginId), key, secret, error);
    };

PluginUiController::PluginUiController(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
}

PluginUiController* PluginUiController::instance()
{
    if (!s_instance)
        s_instance = new PluginUiController;
    return s_instance;
}

PluginUiController* PluginUiController::create(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);
    PluginUiController* controller = instance();
    QQmlEngine::setObjectOwnership(controller, QQmlEngine::CppOwnership);
    return controller;
}

void PluginUiController::setRegistry(PN::Plugins::PluginRegistry* registry)
{
    m_registry = registry;
    if (!registry)
        return;
    connect(registry, &PN::Plugins::PluginRegistry::pluginsChanged, this, &PluginUiController::pluginsChanged);
    connect(registry, &PN::Plugins::PluginRegistry::pluginError, this, [this](const QString& id, const QString& message) {
        emit pluginError(id, m_registry ? m_registry->plugin(id).manifest.name : id, message);
    });
    emit pluginsChanged();
}

int PluginUiController::openRequest(const QString& pluginId, const QString& kind, const QVariantMap& params, Reply reply)
{
    const int id = m_nextRequest++;
    QVariantMap p = params;
    p.insert(QStringLiteral("pluginId"), pluginId);
    if (m_registry)
        p.insert(QStringLiteral("pluginName"), m_registry->plugin(pluginId).manifest.name);
    m_requests.insert(id, {pluginId, kind, p, std::move(reply)});
    QLog_Info(CONSOLELOG, QStringLiteral("[%1] opening %2 request %3 (%4).")
                              .arg(pluginId, kind).arg(id).arg(p.value(QStringLiteral("title")).toString()));
    emit requestOpened(id, kind, p);
    return id;
}

void PluginUiController::logRequestProblem(int requestId, const QString& message)
{
    const auto it = m_requests.constFind(requestId);
    QLog_Warning(CONSOLELOG, QStringLiteral("[%1] %2 request %3: %4")
                                 .arg(it != m_requests.constEnd() ? it->pluginId : QString(),
                                      it != m_requests.constEnd() ? it->kind : QString())
                                 .arg(requestId).arg(message));
}

void PluginUiController::closeRequest(int requestId)
{
    m_requests.remove(requestId);
    emit requestClosed(requestId);
}

void PluginUiController::updateProgress(const QString& token, double fraction, const QString& text)
{
    emit progressUpdated(token, fraction, text);
}

void PluginUiController::notify(const QString& text, const QString& kind, int seconds)
{
    emit notificationRequested(text, kind, seconds);
}

void PluginUiController::respond(int requestId, const QVariant& answer)
{
    const QVariant result = plainVariant(answer);
    auto it = m_requests.find(requestId);
    if (it == m_requests.end())
        return;
    const Reply reply = it->reply;
    QLog_Info(CONSOLELOG, QStringLiteral("[%1] %2 request %3 answered%4.")
                              .arg(it->pluginId, it->kind).arg(requestId)
                              .arg(result.isNull() || !result.isValid() ? QStringLiteral(" with nothing (cancelled)") : QString()));
    // A progress request stays open until the plugin ends it.
    const bool finished = it->kind != QLatin1String("progress");
    if (finished)
        m_requests.erase(it);
    if (reply)
        reply(result.isValid() ? result : QVariant::fromValue(nullptr), {});
    // Closes the dialog when the answer came from plugin QML (pluginDialog.accept).
    if (finished)
        emit requestClosed(requestId);
}

void PluginUiController::cancelRequest(int requestId)
{
    respond(requestId, QVariant::fromValue(nullptr));
}

void PluginUiController::cancelProgress(int requestId)
{
    const auto it = m_requests.constFind(requestId);
    if (it == m_requests.constEnd() || !m_registry)
        return;
    m_registry->progressCancelled(it->pluginId, it->params.value(QStringLiteral("token")).toString());
}

QString PluginUiController::urlToPath(const QUrl& url) const
{
    return url.isLocalFile() ? QDir::toNativeSeparators(url.toLocalFile()) : url.toString();
}

QQuickItem* PluginUiController::createDialogContent(int requestId, QQuickItem* parent)
{
    const auto it = m_requests.constFind(requestId);
    QQmlEngine* engine = qmlEngine(parent);
    if (it == m_requests.constEnd() || !engine)
        return nullptr;
    const QVariantMap& p = it->params;
    QQmlComponent component(engine, QUrl::fromLocalFile(p.value(QStringLiteral("qmlPath")).toString()));
    if (component.isError()) {
        const QString message = component.errorString();
        QLog_Error(ERRORLOG, QStringLiteral("[%1] dialog failed to load: %2").arg(it->pluginId, message));
        Reply reply = it->reply;
        m_requests.remove(requestId);
        if (reply)
            reply({}, PluginError::make(QStringLiteral("dialog-failed"), message));
        return nullptr;
    }
    auto* context = new QQmlContext(engine->rootContext());
    auto* api = new PluginQmlContext(this, engine, requestId, it->pluginId, p.value(QStringLiteral("token")).toString(),
                                     p.value(QStringLiteral("data")), p.value(QStringLiteral("title")).toString(), context);
    context->setContextProperty(QStringLiteral("pluginDialog"), api);
    QObject* object = component.beginCreate(context);
    auto* item = qobject_cast<QQuickItem*>(object);
    if (!item) {
        component.completeCreate();
        delete object;
        delete context;
        Reply reply = it->reply;
        m_requests.remove(requestId);
        if (reply)
            reply({}, PluginError::make(QStringLiteral("dialog-failed"), QStringLiteral("The dialog's root must be a visual item.")));
        return nullptr;
    }
    item->setParentItem(parent);
    component.completeCreate();
    context->setParent(item);
    return item;
}

QQuickItem* PluginUiController::createSettingsContent(const QString& pluginId, const QString& qmlPath, QQuickItem* parent)
{
    QQmlEngine* engine = qmlEngine(parent);
    if (!engine)
        return nullptr;
    QQmlComponent component(engine, QUrl::fromLocalFile(qmlPath));
    if (component.isError()) {
        QLog_Error(ERRORLOG, QStringLiteral("[%1] settings section failed to load: %2").arg(pluginId, component.errorString()));
        return nullptr;
    }
    auto* context = new QQmlContext(engine->rootContext());
    auto* api = new PluginQmlContext(this, engine, -1, pluginId, QString(), QVariant(), QString(), context);
    context->setContextProperty(QStringLiteral("pluginSettings"), api);
    QObject* object = component.beginCreate(context);
    auto* item = qobject_cast<QQuickItem*>(object);
    if (!item) {
        component.completeCreate();
        delete object;
        delete context;
        return nullptr;
    }
    item->setParentItem(parent);
    component.completeCreate();
    context->setParent(item);
    return item;
}

QVariantList PluginUiController::plugins() const
{
    QVariantList out;
    if (!m_registry)
        return out;
    for (const PluginInfo& info : m_registry->plugins())
        out.append(describe(info));
    return out;
}

QVariantList PluginUiController::settingsSections() const
{
    return m_registry ? m_registry->settingsSections() : QVariantList();
}

QString PluginUiController::userPluginsPath() const
{
    return m_registry ? QDir::toNativeSeparators(m_registry->paths().user) : QString();
}

bool PluginUiController::userPluginsAllowed() const
{
#ifdef PN_APP_STORE_DISABLE_USER_PLUGINS
    return false;
#else
    return true;
#endif
}

QString PluginUiController::setEnabled(const QString& id, bool enabled)
{
    QString error;
    if (!m_registry || !m_registry->setEnabled(id, enabled, &error))
        return error.isEmpty() ? tr("The plugin could not be changed.") : error;
    return {};
}

QString PluginUiController::approve(const QString& id)
{
    QString error;
    if (!m_registry || !m_registry->approve(id, &error))
        return error.isEmpty() ? tr("The plugin could not be enabled.") : error;
    return {};
}

QVariantMap PluginUiController::inspect(const QUrl& path)
{
    PN::Plugins::PluginManifest manifest;
    QString error;
    const QString local = path.isLocalFile() ? path.toLocalFile() : path.toString();
    if (!m_registry || !m_registry->inspectPackage(local, &manifest, &error))
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}};
    PluginInfo info;
    info.manifest = manifest;
    info.valid = true;
    QVariantMap out = describe(info);
    out.insert(QStringLiteral("ok"), true);
    out.insert(QStringLiteral("replaces"), !m_registry->plugin(manifest.id).manifest.id.isEmpty());
    return out;
}

QString PluginUiController::install(const QUrl& path)
{
    QString id, error;
    const QString local = path.isLocalFile() ? path.toLocalFile() : path.toString();
    if (!m_registry || !m_registry->install(local, &id, &error))
        return error.isEmpty() ? tr("The plugin could not be installed.") : error;
    return {};
}

QString PluginUiController::uninstall(const QString& id)
{
    QString error;
    if (!m_registry || !m_registry->uninstall(id, &error))
        return error.isEmpty() ? tr("The plugin could not be removed.") : error;
    return {};
}

void PluginUiController::reload(const QString& id)
{
    if (m_registry)
        m_registry->reload(id);
}

void PluginUiController::openPluginsFolder()
{
    if (!m_registry)
        return;
    QDir().mkpath(m_registry->paths().user);
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_registry->paths().user));
}

QVariantMap PluginUiController::settingValues(const QString& pluginId) const
{
    QSettings settings(m_organization, QStringLiteral("PluginSettings"));
    settings.beginGroup(pluginId);
    QVariantMap out;
    for (const QString& key : settings.allKeys())
        out.insert(key, settings.value(key));
    return out;
}

void PluginUiController::setSetting(const QString& pluginId, const QString& key, const QVariant& value)
{
    // QML passes a JavaScript value (an array of row objects, for a table) wrapped
    // in a QJSValue. QSettings cannot store that wrapper, so unwrap it first.
    const QVariant plain = value.typeId() == QMetaType::fromType<QJSValue>().id() ? value.value<QJSValue>().toVariant() : value;
    const bool password = passwordFields(pluginId).contains(key);
    if (!password || storePasswordField(pluginId, key, plain.toString())) {
        QSettings settings(m_organization, QStringLiteral("PluginSettings"));
        settings.beginGroup(pluginId);
        if (!password && plain.isValid() && !plain.isNull())
            settings.setValue(key, plain);
        else
            settings.remove(key);
    }
    if (m_registry)
        m_registry->notifySettingsChanged(pluginId, {key});
}

QSet<QString> PluginUiController::passwordFields(const QString& pluginId) const
{
    return m_registry ? passwordKeys(m_registry->plugin(pluginId)) : QSet<QString>();
}

bool PluginUiController::storePasswordField(const QString& pluginId, const QString& key, const QString& secret)
{
    if (secret.isEmpty())
        return true;
    QString error;
    if (s_secretWriter(pluginId, key, secret, &error))
        return true;
    const PluginInfo info = m_registry ? m_registry->plugin(pluginId) : PluginInfo();
    emit notificationRequested(tr("%1 could not save %2 in the system credential store: %3")
                                   .arg(info.manifest.name.isEmpty() ? pluginId : info.manifest.name, key, error),
                               QStringLiteral("error"), 10);
    return false;
}

QVariantList PluginUiController::settingsBackupCandidates() const
{
    QVariantList out;
    if (!m_registry)
        return out;
    QSettings settings(m_organization, QStringLiteral("PluginSettings"));
    settings.setFallbacksEnabled(false);
    for (const PluginInfo& info : m_registry->plugins()) {
        if (info.manifest.id.isEmpty())
            continue;
        const QSet<QString> skip = passwordKeys(info);
        settings.beginGroup(info.manifest.id);
        int count = 0;
        for (const QString& key : settings.allKeys())
            if (!isInternalSettingKey(key) && !skip.contains(key))
                ++count;
        settings.endGroup();
        if (count > 0)
            out.append(QVariantMap {{QStringLiteral("id"), info.manifest.id},
                                    {QStringLiteral("name"), info.manifest.name.isEmpty() ? info.manifest.id : info.manifest.name},
                                    {QStringLiteral("count"), count}});
    }
    return out;
}

QJsonObject PluginUiController::pluginSettingsJson(const QStringList& pluginIds) const
{
    QJsonObject plugins;
    QSettings settings(m_organization, QStringLiteral("PluginSettings"));
    settings.setFallbacksEnabled(false);
    for (const QString& id : pluginIds) {
        const QSet<QString> skip = m_registry ? passwordKeys(m_registry->plugin(id)) : QSet<QString>();
        settings.beginGroup(id);
        QJsonObject values;
        for (const QString& key : settings.allKeys())
            if (!isInternalSettingKey(key) && !skip.contains(key))
                values.insert(key, QJsonValue::fromVariant(settings.value(key)));
        settings.endGroup();
        if (!values.isEmpty())
            plugins.insert(id, values);
    }
    return plugins;
}

QJsonObject PluginUiController::appSettingsJson() const
{
    QJsonObject values;
    QSettings settings(m_organization, QStringLiteral("AppSettings"));
    settings.setFallbacksEnabled(false);
    for (const QString& key : settings.allKeys()) {
        const QVariant value = settings.value(key);
        if (isPortableAppSettingKey(key) && value.isValid())
            values.insert(key, QJsonValue::fromVariant(value));
    }
    return values;
}

QString PluginUiController::exportSettings(const QUrl& file, const QStringList& pluginIds)
{
    const QJsonObject plugins = pluginSettingsJson(pluginIds);
    if (plugins.isEmpty())
        return tr("The selected plugins have no saved settings.");

    const QJsonObject root {{QStringLiteral("format"), kSettingsBackupFormat}, {QStringLiteral("version"), 1},
                            {QStringLiteral("exported"), QDateTime::currentDateTime().toString(Qt::ISODate)},
                            {QStringLiteral("plugins"), plugins}};
    if (const QString error = writeSettingsFile(file, root); !error.isEmpty())
        return error;
    QLog_Info(CONSOLELOG, QStringLiteral("Exported settings of %1 plugin(s) to %2.").arg(plugins.size()).arg(urlToPath(file)));
    return {};
}

QString PluginUiController::exportAllSettings(const QUrl& file)
{
    // Every group, not only installed plugins: a plugin removed for now, or an
    // old Python group still waiting for its successor, keeps its settings.
    const QStringList pluginIds = QSettings(m_organization, QStringLiteral("PluginSettings")).childGroups();
    const QJsonObject app = appSettingsJson();
    const QJsonObject plugins = pluginSettingsJson(pluginIds);
    if (app.isEmpty() && plugins.isEmpty())
        return tr("There are no saved settings to export.");

    const QJsonObject root {{QStringLiteral("format"), kAllSettingsFormat}, {QStringLiteral("version"), 1},
                            {QStringLiteral("exported"), QDateTime::currentDateTime().toString(Qt::ISODate)},
                            {QStringLiteral("app"), app}, {QStringLiteral("plugins"), plugins}};
    if (const QString error = writeSettingsFile(file, root); !error.isEmpty())
        return error;
    QLog_Info(CONSOLELOG, QStringLiteral("Exported %1 application setting(s) and settings of %2 plugin(s) to %3.")
                              .arg(app.size()).arg(plugins.size()).arg(urlToPath(file)));
    return {};
}

QVariantMap PluginUiController::applyPluginSettings(const QJsonObject& groups, bool legacy)
{
    // Python plugin groups a JavaScript plugin took over: group -> {plugin id, key prefix}.
    // A group with no installed successor is stored as is, where that plugin's
    // one-time legacy copy finds it once it is installed.
    QHash<QString, QPair<QString, QString>> successors;
    if (legacy && m_registry)
        for (const PluginInfo& info : m_registry->plugins())
            for (const auto& [group, prefix] : info.manifest.legacySettings)
                successors.insert(group, {info.manifest.id, prefix});

    QHash<QString, QStringList> changed;
    int count = 0;
    {
        QSettings settings(m_organization, QStringLiteral("PluginSettings"));
        settings.setFallbacksEnabled(false);
        for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
            if (it.key().isEmpty() || !it.value().isObject())
                continue;
            QString target = it.key();
            QString prefix;
            if (successors.contains(target)) {
                prefix = successors.value(target).second;
                target = successors.value(target).first;
            }
            const QJsonObject values = it.value().toObject();
            // A Python plugin's export can carry its passwords in clear text.
            const QSet<QString> passwords = passwordFields(target);
            settings.beginGroup(target);
            for (auto v = values.constBegin(); v != values.constEnd(); ++v) {
                if (isInternalSettingKey(v.key()))
                    continue;
                if (passwords.contains(prefix + v.key())) {
                    if (storePasswordField(target, prefix + v.key(), v.value().toVariant().toString()))
                        settings.remove(prefix + v.key());
                } else {
                    settings.setValue(prefix + v.key(), v.value().toVariant());
                }
                changed[target].append(prefix + v.key());
                ++count;
            }
            settings.endGroup();
        }
    }

    QStringList names;
    for (auto it = changed.constBegin(); it != changed.constEnd(); ++it) {
        const PluginInfo info = m_registry ? m_registry->plugin(it.key()) : PluginInfo();
        names.append(info.manifest.name.isEmpty() ? it.key() : info.manifest.name);
        if (m_registry)
            m_registry->notifySettingsChanged(it.key(), it.value());
    }
    names.removeDuplicates();
    names.sort(Qt::CaseInsensitive);
    if (count > 0) {
        ++m_settingsRevision;
        emit settingsRevisionChanged();
    }
    return {{QStringLiteral("plugins"), names}, {QStringLiteral("count"), count}};
}

QVariantMap PluginUiController::importSettings(const QUrl& file)
{
    const auto fail = [](const QString& error) {
        return QVariantMap {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}};
    };
    QJsonObject root;
    if (const QString error = readSettingsFile(file, &root); !error.isEmpty())
        return fail(error);

    // Files from exportSettings and exportAllSettings name their format; this
    // reads only the plugin part of the second. Any other object is an export
    // from the Python plugins' Settings Migrator: {group: {key: value}}.
    const QString format = root.value(QStringLiteral("format")).toString();
    const bool current = format == kSettingsBackupFormat || format == kAllSettingsFormat;
    if (!current && root.contains(QStringLiteral("format")))
        return fail(tr("%1 is not a plugin settings file.").arg(urlToPath(file)));
    const QJsonObject groups = current ? root.value(QStringLiteral("plugins")).toObject() : root;

    QVariantMap result = applyPluginSettings(groups, !current);
    const int count = result.value(QStringLiteral("count")).toInt();
    if (count == 0)
        return fail(tr("The file has no plugin settings."));
    QLog_Info(CONSOLELOG, QStringLiteral("Imported %1 plugin setting(s) from %2.").arg(count).arg(urlToPath(file)));
    result.insert(QStringLiteral("ok"), true);
    return result;
}

QVariantMap PluginUiController::importAllSettings(const QUrl& file)
{
    const auto fail = [](const QString& error) {
        return QVariantMap {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}};
    };
    QJsonObject root;
    if (const QString error = readSettingsFile(file, &root); !error.isEmpty())
        return fail(error);

    // A plugin settings backup imports here too; it just has no "app" part.
    const QString format = root.value(QStringLiteral("format")).toString();
    if (format != kAllSettingsFormat && format != kSettingsBackupFormat)
        return fail(tr("%1 is not a Project Notes settings file.").arg(urlToPath(file)));

    int appCount = 0;
    {
        QSettings settings(m_organization, QStringLiteral("AppSettings"));
        const QJsonObject app = root.value(QStringLiteral("app")).toObject();
        for (auto it = app.constBegin(); it != app.constEnd(); ++it) {
            if (!isPortableAppSettingKey(it.key()) || it.value().isNull() || it.value().isUndefined())
                continue;
            settings.setValue(it.key(), it.value().toVariant());
            ++appCount;
        }
    }
    QVariantMap result = applyPluginSettings(root.value(QStringLiteral("plugins")).toObject(), false);
    const int pluginCount = result.value(QStringLiteral("count")).toInt();
    if (appCount == 0 && pluginCount == 0)
        return fail(tr("The file has no settings."));
    QLog_Info(CONSOLELOG, QStringLiteral("Imported %1 application setting(s) and %2 plugin setting(s) from %3.")
                              .arg(appCount).arg(pluginCount).arg(urlToPath(file)));
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("appCount"), appCount);
    return result;
}

QJSValue PluginUiController::invokePlugin(QJSEngine* engine, const QString& pluginId, const QString& token,
                                          const QString& name, const QJSValue& args)
{
    if (!engine)
        return QJSValue();
    QJSValue deferred = makeDeferred(engine);
    const QVariant argValue = args.isUndefined() ? QVariant() : args.toVariant();
    QVariantList list = argValue.typeId() == QMetaType::QVariantList ? argValue.toList()
                      : (argValue.isValid() ? QVariantList{argValue} : QVariantList{});
    if (!m_registry) {
        deferred.property(QStringLiteral("reject")).call({QJSValue(tr("Plugins are not available."))});
        return deferred.property(QStringLiteral("promise"));
    }
    QPointer<PluginUiController> self(this);
    m_registry->invoke(pluginId, token, name, list, [self, deferred, engine](const QVariant& value, const PluginError& error) mutable {
            if (!self)
            return;
        if (error.isError())
            deferred.property(QStringLiteral("reject")).call({PN::Plugins::toJsValue(engine, error.toMap())});
        else
            deferred.property(QStringLiteral("resolve")).call({PN::Plugins::toJsValue(engine, value)});
    });
    return deferred.property(QStringLiteral("promise"));
}

// ── PluginQmlContext ─────────────────────────────────────────────────────────

PluginQmlContext::PluginQmlContext(PluginUiController* controller, QJSEngine* engine, int requestId, QString pluginId,
                                   QString token, QVariant data, QString title, QObject* parent)
    : QObject(parent), m_controller(controller), m_engine(engine), m_requestId(requestId), m_pluginId(std::move(pluginId)),
      m_token(std::move(token)), m_data(std::move(data)), m_title(std::move(title))
{
}

QVariantMap PluginQmlContext::values() const
{
    return m_controller->settingValues(m_pluginId);
}

void PluginQmlContext::accept(const QVariant& result)
{
    QVariant value = result;
    if (value.canConvert<QJSValue>())
        value = value.value<QJSValue>().toVariant();
    m_controller->respond(m_requestId, value.isValid() ? value : QVariant(true));
}

void PluginQmlContext::reject()
{
    m_controller->cancelRequest(m_requestId);
}

QJSValue PluginQmlContext::invoke(const QString& name, const QJSValue& args)
{
    return m_controller->invokePlugin(m_engine, m_pluginId, m_token, name, args);
}

QVariant PluginQmlContext::get(const QString& key) const
{
    return m_controller->settingValues(m_pluginId).value(key);
}

void PluginQmlContext::set(const QString& key, const QVariant& value)
{
    QVariant v = value;
    if (v.canConvert<QJSValue>())
        v = v.value<QJSValue>().toVariant();
    m_controller->setSetting(m_pluginId, key, v);
    emit valuesChanged();
}
