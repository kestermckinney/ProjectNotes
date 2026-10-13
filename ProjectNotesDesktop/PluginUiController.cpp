// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginUiController.h"

#include "ProjectNotesPlugins/JsConvert.h"
#include "ProjectNotesPlugins/PluginRegistry.h"

#include "QLogger.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSettings>

using namespace QLogger;
using PN::Plugins::PluginError;
using PN::Plugins::PluginInfo;
using PN::Plugins::Reply;

namespace {
PluginUiController* s_instance = nullptr;

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

QJSValue makeDeferred(QJSEngine* engine)
{
    return engine->evaluate(QStringLiteral(
        "(function(){ var d = {}; d.promise = new Promise(function(res, rej){ d.resolve = res; d.reject = rej; }); return d; })()"));
}
} // namespace

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
    emit requestOpened(id, kind, p);
    return id;
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

void PluginUiController::respond(int requestId, const QVariant& result)
{
    auto it = m_requests.find(requestId);
    if (it == m_requests.end())
        return;
    const Reply reply = it->reply;
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
    {
        QSettings settings(m_organization, QStringLiteral("PluginSettings"));
        settings.beginGroup(pluginId);
        if (value.isValid() && !value.isNull())
            settings.setValue(key, value);
        else
            settings.remove(key);
    }
    if (m_registry)
        m_registry->notifySettingsChanged(pluginId, {key});
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
