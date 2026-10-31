// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginRegistry.h"

#include "PluginHost.h"
#include "PluginServices.h"
#include "PublishedSchema.h"

#include "QLogger.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>

#include <private/qzipreader_p.h>

using namespace QLogger;

namespace PN::Plugins {

namespace {

bool copyFolder(const QString& from, const QString& to, QString* error)
{
    QDir source(from);
    if (!QDir().mkpath(to)) {
        *error = QStringLiteral("Could not create %1.").arg(to);
        return false;
    }
    QDirIterator it(from, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QString relative = source.relativeFilePath(it.filePath());
        const QString target = QDir(to).filePath(relative);
        if (it.fileInfo().isDir()) {
            QDir().mkpath(target);
        } else {
            QDir().mkpath(QFileInfo(target).absolutePath());
            if (!QFile::copy(it.filePath(), target)) {
                *error = QStringLiteral("Could not copy %1.").arg(relative);
                return false;
            }
        }
    }
    return true;
}

QVariantMap menuEntry(const QString& pluginId, const QString& pluginName, const QVariantMap& menu)
{
    QVariantMap entry = menu;
    entry.insert(QStringLiteral("pluginId"), pluginId);
    entry.insert(QStringLiteral("pluginName"), pluginName);
    entry.insert(QStringLiteral("menuId"), menu.value(QStringLiteral("id")));
    return entry;
}

QVariantMap menuToMap(const MenuContribution& menu)
{
    return {{QStringLiteral("id"), menu.id}, {QStringLiteral("title"), menu.title},
            {QStringLiteral("context"), menu.context}, {QStringLiteral("entity"), menu.entity},
            {QStringLiteral("submenu"), menu.submenu}, {QStringLiteral("include"), menu.include},
            {QStringLiteral("parameter"), menu.parameter}, {QStringLiteral("platforms"), menu.platforms}};
}

} // namespace

PluginRegistry::PluginRegistry(PluginServices* services, QObject* parent)
    : QObject(parent), m_services(services)
{
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
        for (const PluginInfo& info : std::as_const(m_plugins))
            if (path.startsWith(info.folder + QLatin1Char('/')))
                scheduleReload(info.manifest.id.isEmpty() ? info.folder : info.manifest.id);
    });
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {
        if (!m_hotReload)
            return;
        // Inside a plugin folder: reload that plugin. A plugin root: rescan,
        // since a plugin was added or removed.
        for (const PluginInfo& info : std::as_const(m_plugins)) {
            if (path == info.folder || path.startsWith(info.folder + QLatin1Char('/'))) {
                const QStringList files = sourceFiles(info.folder);
                if (files != m_sourceFiles.value(info.folder)) {
                    m_sourceFiles.insert(info.folder, files);
                    scheduleReload(info.manifest.id);
                }
                return;
            }
        }
        m_rootRescan->start();
    });
    m_rootRescan = new QTimer(this);
    m_rootRescan->setSingleShot(true);
    m_rootRescan->setInterval(400);
    connect(m_rootRescan, &QTimer::timeout, this, &PluginRegistry::loadAll);

    m_watchdog = new QTimer(this);
    m_watchdog->setInterval(250);
    connect(m_watchdog, &QTimer::timeout, this, &PluginRegistry::checkWatchdog);
}

PluginRegistry::~PluginRegistry()
{
    stopAll();
}

QString PluginRegistry::stateKey(const QString& id, const char* key) const
{
    return id + QLatin1Char('/') + QLatin1String(key);
}

bool PluginRegistry::approved(const PluginInfo& info) const
{
    if (info.bundled)
        return true;
    QSettings state(m_organization, QStringLiteral("PluginState"));
    return state.value(stateKey(info.manifest.id, "approved")).toString() == info.manifest.approvalFingerprint();
}

bool PluginRegistry::readEnabled(const PluginInfo& info) const
{
    QSettings state(m_organization, QStringLiteral("PluginState"));
    const QVariant stored = state.value(stateKey(info.manifest.id, "enabled"));
    if (info.bundled)
        return stored.isValid() ? stored.toBool() : true;
    return stored.toBool() && approved(info);
}

void PluginRegistry::discover()
{
    m_plugins.clear();
    if (!m_watcher->files().isEmpty())
        m_watcher->removePaths(m_watcher->files());
    if (!m_watcher->directories().isEmpty())
        m_watcher->removePaths(m_watcher->directories());

    QHash<QString, int> byId;
    auto scan = [&](const QString& root, bool bundled) {
        if (root.isEmpty() || !QFileInfo(root).isDir())
            return;
        if (m_hotReload)
            m_watcher->addPath(root);
        const QStringList folders = QDir(root).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString& name : folders) {
            const QString folder = QDir(root).filePath(name);
            if (!QFileInfo::exists(QDir(folder).filePath(QStringLiteral("plugin.json"))))
                continue;   // not a JS plugin (e.g. a legacy Python folder)
            PluginInfo info;
            info.folder = QDir(folder).absolutePath();
            info.bundled = bundled;
            QString error;
            info.valid = PluginManifest::load(folder, &info.manifest, &error);
            info.loadError = error;
            info.manifest.bundled = bundled;
            if (info.manifest.id.isEmpty())
                info.manifest.id = name;
            info.platformSupported = info.manifest.supportsPlatform(currentPlatform());
            if (!bundled && !m_userPluginsAllowed) {
                info.valid = false;
                info.loadError = QStringLiteral("User plugins are turned off in this build.");
            }
            info.needsApproval = info.valid && !approved(info);
            info.enabled = info.valid && readEnabled(info);

            // A user plugin with the same id replaces the bundled one.
            const auto existing = byId.constFind(info.manifest.id);
            if (existing != byId.constEnd()) {
                m_plugins[*existing] = info;
            } else {
                byId.insert(info.manifest.id, int(m_plugins.size()));
                m_plugins.append(info);
            }
        }
    };
    scan(m_paths.bundled, true);
    scan(m_paths.user, false);

    for (const PluginInfo& info : std::as_const(m_plugins))
        watchPlugin(info);
}

void PluginRegistry::watchPlugin(const PluginInfo& info)
{
    if (!m_hotReload)
        return;
    m_watcher->addPath(info.folder);
    const QStringList files = sourceFiles(info.folder);
    m_sourceFiles.insert(info.folder, files);
    for (const QString& entry : files)
        m_watcher->addPath(entry.section(QLatin1Char('|'), 0, 0));
}

// The plugin's code and manifest files, each as "path|size|modified", sorted.
QStringList PluginRegistry::sourceFiles(const QString& folder)
{
    QStringList out;
    QDirIterator it(folder, {QStringLiteral("*.mjs"), QStringLiteral("*.js"), QStringLiteral("*.json"),
                             QStringLiteral("*.qml")},
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QFileInfo file(it.next());
        out.append(file.filePath() + QLatin1Char('|') + QString::number(file.size()) + QLatin1Char('|')
                   + QString::number(file.lastModified().toMSecsSinceEpoch()));
    }
    out.sort();
    return out;
}

void PluginRegistry::loadAll()
{
    stopAll();
    discover();
    for (const PluginInfo& info : std::as_const(m_plugins))
        if (info.enabled && info.valid && info.platformSupported)
            startPlugin(info.manifest.id);
    m_watchdog->start();
    emit pluginsChanged();
    emit menusChanged();
}

void PluginRegistry::stopAll()
{
    const QStringList ids = m_hosts.keys();
    for (const QString& id : ids)
        stopPlugin(id);
}

PluginHost* PluginRegistry::host(const QString& id) const
{
    return m_hosts.value(id, nullptr);
}

void PluginRegistry::startPlugin(const QString& id)
{
    if (m_hosts.contains(id))
        return;
    const auto it = std::find_if(m_plugins.begin(), m_plugins.end(),
                                 [&id](const PluginInfo& p) { return p.manifest.id == id; });
    if (it == m_plugins.end() || !it->valid)
        return;

    PluginHostConfig config;
    config.manifest = it->manifest;
    config.databasePath = m_databasePath;
    config.dataPath = QDir(m_paths.data).filePath(id);
    config.tempPath = QDir(m_paths.temp).filePath(id);
    config.settingsOrganization = m_organization;
    config.profile = m_profile;
    config.appVersion = m_appVersion;
    config.buildFlavor = m_buildFlavor;
    if (m_services)
        config.capabilities = m_services->capabilities();

    auto* pluginHost = new PluginHost(config, m_services, this);
    m_hosts.insert(id, pluginHost);
    connect(pluginHost, &PluginHost::started, this, [this, id](bool ok, const QString& error) {
        for (PluginInfo& p : m_plugins) {
            if (p.manifest.id == id) {
                p.running = ok;
                if (!ok)
                    p.lastError = error;
            }
        }
        if (!ok)
            emit pluginError(id, error);
        emit pluginsChanged();
        emit menusChanged();
    });
    connect(pluginHost, &PluginHost::dynamicMenusChanged, this, &PluginRegistry::menusChanged);
    connect(pluginHost, &PluginHost::foreignSettingsChanged, this, &PluginRegistry::notifySettingsChanged);
    connect(pluginHost, &PluginHost::errorReported, this, [this, id](const QString& where, const QString& message) {
        for (PluginInfo& p : m_plugins)
            if (p.manifest.id == id)
                p.lastError = where + QStringLiteral(": ") + message;
        emit pluginError(id, message);
        emit pluginsChanged();
    });
    connect(pluginHost, &PluginHost::hung, this, [this, id](const QString& where) {
        const QString message = QStringLiteral("A call (%1) ran longer than %2 seconds and was stopped.")
                                    .arg(where).arg(m_watchdogBudgetMs / 1000.0);
        for (PluginInfo& p : m_plugins)
            if (p.manifest.id == id)
                p.lastError = message;
        emit pluginError(id, message);
        emit pluginsChanged();
    });
    connect(pluginHost, &PluginHost::rowsChanged, this, [this, id](const QVariantList& changes) {
        emit pluginDataChanged(changes);
        broadcast(changes, QStringLiteral("plugin"), id);
    });
    pluginHost->start();
}

void PluginRegistry::stopPlugin(const QString& id)
{
    PluginHost* pluginHost = m_hosts.take(id);
    if (!pluginHost)
        return;
    pluginHost->stop();
    pluginHost->deleteLater();
    for (PluginInfo& p : m_plugins)
        if (p.manifest.id == id)
            p.running = false;
}

void PluginRegistry::scheduleReload(const QString& id)
{
    if (!m_hotReload)
        return;
    QTimer* timer = m_reloadTimers.value(id);
    if (!timer) {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(300);
        connect(timer, &QTimer::timeout, this, [this, id] { reload(id); });
        m_reloadTimers.insert(id, timer);
    }
    timer->start();
}

void PluginRegistry::reload(const QString& id)
{
    const auto it = std::find_if(m_plugins.begin(), m_plugins.end(),
                                 [&id](const PluginInfo& p) { return p.manifest.id == id; });
    if (it == m_plugins.end())
        return;
    QLog_Info(CONSOLELOG, QStringLiteral("Reloading plugin %1.").arg(id));
    stopPlugin(id);

    PluginInfo info = *it;
    QString error;
    info.valid = PluginManifest::load(info.folder, &info.manifest, &error);
    info.loadError = error;
    info.manifest.bundled = info.bundled;
    if (info.manifest.id.isEmpty())
        info.manifest.id = id;
    info.lastError.clear();
    info.needsApproval = info.valid && !approved(info);
    info.enabled = info.valid && readEnabled(info);
    info.platformSupported = info.manifest.supportsPlatform(currentPlatform());
    *it = info;
    watchPlugin(info);
    if (info.enabled && info.valid && info.platformSupported)
        startPlugin(info.manifest.id);
    emit pluginsChanged();
    emit menusChanged();
}

QList<PluginInfo> PluginRegistry::plugins() const
{
    return m_plugins;
}

PluginInfo PluginRegistry::plugin(const QString& id) const
{
    for (const PluginInfo& p : m_plugins)
        if (p.manifest.id == id)
            return p;
    return {};
}

bool PluginRegistry::isRunning(const QString& id) const
{
    PluginHost* h = host(id);
    return h && h->isRunning();
}

QVariantList PluginRegistry::menusForEntity(const QString& entity) const
{
    QVariantList out;
    for (const PluginInfo& info : m_plugins) {
        PluginHost* h = host(info.manifest.id);
        if (!h || !h->isRunning())
            continue;
        QVariantList menus;
        for (const MenuContribution& menu : info.manifest.menus)
            menus.append(menuToMap(menu));
        menus.append(h->dynamicMenus());
        for (const QVariant& m : std::as_const(menus)) {
            const QVariantMap menu = m.toMap();
            const QStringList platforms = menu.value(QStringLiteral("platforms")).toStringList();
            if (!platforms.isEmpty() && !platforms.contains(currentPlatform()))
                continue;
            if (menu.value(QStringLiteral("context")).toString() == QLatin1String("record")
                && menu.value(QStringLiteral("entity")).toString() == entity)
                out.append(menuEntry(info.manifest.id, info.manifest.name, menu));
        }
    }
    return out;
}

QVariantList PluginRegistry::globalMenus() const
{
    QVariantList out;
    for (const PluginInfo& info : m_plugins) {
        PluginHost* h = host(info.manifest.id);
        if (!h || !h->isRunning())
            continue;
        QVariantList menus;
        for (const MenuContribution& menu : info.manifest.menus)
            menus.append(menuToMap(menu));
        menus.append(h->dynamicMenus());
        for (const QVariant& m : std::as_const(menus)) {
            const QVariantMap menu = m.toMap();
            const QStringList platforms = menu.value(QStringLiteral("platforms")).toStringList();
            if (!platforms.isEmpty() && !platforms.contains(currentPlatform()))
                continue;
            if (menu.value(QStringLiteral("context"), QStringLiteral("global")).toString() == QLatin1String("global"))
                out.append(menuEntry(info.manifest.id, info.manifest.name, menu));
        }
    }
    return out;
}

void PluginRegistry::runMenu(const QString& pluginId, const QString& menuId, const QString& entity, const QString& key)
{
    PluginHost* h = host(pluginId);
    if (!h) {
        QLog_Warning(CONSOLELOG, QStringLiteral("[%1] menu %2 ignored: the plugin is not loaded.").arg(pluginId, menuId));
        return;
    }
    if (const qint64 since = h->busySince())
        QLog_Info(CONSOLELOG, QStringLiteral("[%1] menu %2 waits: the plugin has been busy for %3 ms.")
                                  .arg(pluginId, menuId).arg(QDateTime::currentMSecsSinceEpoch() - since));
    const PluginInfo info = plugin(pluginId);
    QStringList include;
    QString parameter;
    for (const MenuContribution& menu : info.manifest.menus) {
        if (menu.id == menuId) {
            include = menu.include;
            parameter = menu.parameter;
        }
    }
    for (const QVariant& m : h->dynamicMenus()) {
        const QVariantMap menu = m.toMap();
        if (menu.value(QStringLiteral("id")).toString() == menuId) {
            include = menu.value(QStringLiteral("include")).toStringList();
            parameter = menu.value(QStringLiteral("parameter")).toString();
        }
    }
    h->dispatchMenu(menuId, entity, key, parameter, include);
}

QVariantList PluginRegistry::settingsSections() const
{
    QVariantList out;
    for (const PluginInfo& info : m_plugins) {
        if (!info.valid || !info.enabled || !info.platformSupported)
            continue;
        for (const SettingsContribution& section : info.manifest.settings) {
            QVariantMap entry {{QStringLiteral("pluginId"), info.manifest.id},
                               {QStringLiteral("pluginName"), info.manifest.name},
                               {QStringLiteral("id"), section.id}, {QStringLiteral("title"), section.title}};
            if (!section.schema.isEmpty()) {
                QFile file(QDir(info.folder).filePath(section.schema));
                if (file.open(QIODevice::ReadOnly))
                    entry.insert(QStringLiteral("schema"), QJsonDocument::fromJson(file.readAll()).toVariant());
            }
            if (!section.qml.isEmpty())
                entry.insert(QStringLiteral("qml"), QDir(info.folder).filePath(section.qml));
            out.append(entry);
        }
    }
    return out;
}

bool PluginRegistry::setEnabled(const QString& id, bool enabled, QString* error)
{
    const PluginInfo info = plugin(id);
    if (info.manifest.id.isEmpty()) {
        if (error) *error = QStringLiteral("No plugin has id \"%1\".").arg(id);
        return false;
    }
    if (enabled && !info.valid) {
        if (error) *error = info.loadError;
        return false;
    }
    if (enabled && !approved(info)) {
        if (error) *error = QStringLiteral("Review and approve this plugin's permissions first.");
        return false;
    }
    QSettings state(m_organization, QStringLiteral("PluginState"));
    state.setValue(stateKey(id, "enabled"), enabled);
    state.sync();
    reload(id);
    return true;
}

bool PluginRegistry::approve(const QString& id, QString* error)
{
    const PluginInfo info = plugin(id);
    if (!info.valid) {
        if (error) *error = info.loadError.isEmpty() ? QStringLiteral("Unknown plugin.") : info.loadError;
        return false;
    }
    QSettings state(m_organization, QStringLiteral("PluginState"));
    state.setValue(stateKey(id, "approved"), info.manifest.approvalFingerprint());
    state.setValue(stateKey(id, "enabled"), true);
    state.sync();
    reload(id);
    return true;
}

bool PluginRegistry::extractPackage(const QString& zipPath, QString* folder, QString* error)
{
    QZipReader zip(zipPath);
    if (!zip.isReadable() || zip.status() != QZipReader::NoError) {
        *error = QStringLiteral("%1 is not a readable .pnplugin package.").arg(QFileInfo(zipPath).fileName());
        return false;
    }
    const QString target = *folder;
    for (const QZipReader::FileInfo& entry : zip.fileInfoList()) {
        const QString clean = QDir::cleanPath(entry.filePath);
        if (clean.startsWith(QLatin1String("..")) || QDir::isAbsolutePath(clean) || clean.contains(QStringLiteral("/../"))) {
            *error = QStringLiteral("The package contains an unsafe path (%1).").arg(entry.filePath);
            return false;
        }
        const QString destination = QDir(target).filePath(clean);
        if (entry.isDir) {
            QDir().mkpath(destination);
        } else if (entry.isFile) {
            QDir().mkpath(QFileInfo(destination).absolutePath());
            QFile out(destination);
            if (!out.open(QIODevice::WriteOnly)) {
                *error = QStringLiteral("Could not write %1.").arg(clean);
                return false;
            }
            out.write(zip.fileData(entry.filePath));
        }
    }
    // Accept plugin.json at the root or inside one top-level folder.
    if (!QFileInfo::exists(QDir(target).filePath(QStringLiteral("plugin.json")))) {
        const QStringList dirs = QDir(target).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        if (dirs.size() == 1 && QFileInfo::exists(QDir(target).filePath(dirs.first() + QStringLiteral("/plugin.json"))))
            *folder = QDir(target).filePath(dirs.first());
    }
    return true;
}

bool PluginRegistry::inspectPackage(const QString& path, PluginManifest* manifest, QString* error)
{
    QTemporaryDir temp;
    QString folder = path;
    if (QFileInfo(path).isFile()) {
        folder = temp.path();
        if (!extractPackage(path, &folder, error))
            return false;
    }
    return PluginManifest::load(folder, manifest, error);
}

bool PluginRegistry::install(const QString& path, QString* installedId, QString* error)
{
    if (!m_userPluginsAllowed) {
        *error = QStringLiteral("User plugins are turned off in this build.");
        return false;
    }
    QTemporaryDir temp;
    QString folder = path;
    if (QFileInfo(path).isFile()) {
        folder = temp.path();
        if (!extractPackage(path, &folder, error))
            return false;
    }
    PluginManifest manifest;
    if (!PluginManifest::load(folder, &manifest, error))
        return false;

    const QString target = QDir(m_paths.user).filePath(manifest.id);
    if (QDir(target).absolutePath() == QDir(folder).absolutePath()) {
        *installedId = manifest.id;
        loadAll();
        return true;
    }
    stopPlugin(manifest.id);
    if (QFileInfo::exists(target) && !QDir(target).removeRecursively()) {
        *error = QStringLiteral("Could not replace the installed copy in %1.").arg(target);
        return false;
    }
    if (!copyFolder(folder, target, error))
        return false;
    *installedId = manifest.id;
    loadAll();
    return true;
}

int PluginRegistry::importPluginsFrom(const QString& folder, QString* error)
{
    int copied = 0;
    const QDir source(folder);
    for (const QString& name : source.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString from = source.filePath(name);
        PluginManifest manifest;
        QString ignored;
        if (!PluginManifest::load(from, &manifest, &ignored))
            continue;   // not a plugin folder
        const QString target = QDir(m_paths.user).filePath(manifest.id);
        if (QFileInfo::exists(target))
            continue;
        if (!copyFolder(from, target, error))
            return copied;
        ++copied;
    }
    return copied;
}

bool PluginRegistry::uninstall(const QString& id, QString* error)
{
    const PluginInfo info = plugin(id);
    if (info.manifest.id.isEmpty() || info.bundled) {
        *error = QStringLiteral("Only plugins installed in the user plugin folder can be removed.");
        return false;
    }
    stopPlugin(id);
    if (!QDir(info.folder).removeRecursively()) {
        *error = QStringLiteral("Could not remove %1.").arg(info.folder);
        return false;
    }
    QSettings state(m_organization, QStringLiteral("PluginState"));
    state.remove(id);
    loadAll();
    return true;
}

void PluginRegistry::invoke(const QString& pluginId, const QString& token, const QString& name,
                            const QVariantList& args, Reply reply)
{
    PluginHost* h = host(pluginId);
    if (!h) {
        reply({}, PluginError::make(QStringLiteral("unavailable"), QStringLiteral("The plugin is not running.")));
        return;
    }
    h->invoke(token, name, args, std::move(reply));
}

void PluginRegistry::progressCancelled(const QString& pluginId, const QString& token)
{
    if (PluginHost* h = host(pluginId))
        h->setProgressCancelled(token);
}

void PluginRegistry::broadcast(const QVariantList& changes, const QString& source, const QString& fromPlugin)
{
    QVariantList published;
    for (const QVariant& c : changes) {
        const QVariantMap change = c.toMap();
        static const QStringList operations {QStringLiteral("insert"), QStringLiteral("update"), QStringLiteral("delete")};
        QVariantMap out {{QStringLiteral("entity"), PublishedSchema::instance().publishedEntity(change.value(QStringLiteral("table")).toString())},
                         {QStringLiteral("key"), change.value(QStringLiteral("key"))},
                         {QStringLiteral("operation"), operations.value(change.value(QStringLiteral("operation")).toInt())},
                         {QStringLiteral("source"), source}};
        if (!fromPlugin.isEmpty())
            out.insert(QStringLiteral("pluginId"), fromPlugin);
        published.append(out);
    }
    if (published.isEmpty())
        return;
    for (auto it = m_hosts.constBegin(); it != m_hosts.constEnd(); ++it)
        if (it.key() != fromPlugin)
            it.value()->deliver(QStringLiteral("data.changed"), published);
}

void PluginRegistry::notifyDataChanged(const QVariantList& changes)
{
    broadcast(changes, QStringLiteral("app"), QString());
}

void PluginRegistry::notifySettingsChanged(const QString& pluginId, const QStringList& keys)
{
    if (PluginHost* h = host(pluginId))
        h->deliver(QStringLiteral("settings.changed"), keys);
}

void PluginRegistry::notifyViewChanged(const QVariantMap& view)
{
    for (PluginHost* h : std::as_const(m_hosts))
        h->deliver(QStringLiteral("app.viewChanged"), view);
}

void PluginRegistry::checkWatchdog()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (PluginHost* h : std::as_const(m_hosts)) {
        const qint64 since = h->busySince();
        if (since > 0 && now - since > m_watchdogBudgetMs)
            h->interrupt();
    }
}

} // namespace PN::Plugins
