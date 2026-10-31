// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "appidentity.h"
#include "PluginManifest.h"
#include "PluginTypes.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVariantList>

#include <memory>

class QFileSystemWatcher;
class QTimer;

namespace PN::Plugins {

class PluginHost;
class PluginServices;

struct PluginInfo {
    PluginManifest manifest;
    QString folder;
    bool valid = false;
    QString loadError;          // manifest problem, when !valid
    bool bundled = false;
    bool platformSupported = true;
    bool enabled = false;
    bool needsApproval = false; // a user plugin whose permissions are not yet approved
    bool running = false;
    QString lastError;          // most recent runtime error
};

// Discovers plugins in the bundled and user plugin folders, keeps their
// enabled/approved state, runs the enabled ones (one PluginHost each), and
// exposes their menus and Settings sections to the application. Lives on the
// GUI thread.
class PluginRegistry final : public QObject {
    Q_OBJECT
public:
    struct Paths {
        QString bundled;         // appResourcesPath()/plugins
        QString user;            // <app data location>/plugins
        QString data;            // writable root for per-plugin data folders
        QString temp;            // per-plugin temporary folders (app cache)
    };

    PluginRegistry(PluginServices* services, QObject* parent = nullptr);
    ~PluginRegistry() override;

    void setPaths(const Paths& paths) { m_paths = paths; }
    const Paths& paths() const { return m_paths; }
    void setDatabasePath(const QString& path) { m_databasePath = path; }
    void setSettingsOrganization(const QString& organization) { m_organization = organization; }
    void setProfile(const QString& profile) { m_profile = profile; }
    void setAppVersion(const QString& version) { m_appVersion = version; }
    void setBuildFlavor(const QString& flavor) { m_buildFlavor = flavor; }
    void setUserPluginsAllowed(bool allowed) { m_userPluginsAllowed = allowed; }
    void setWatchdogBudgetMs(int ms) { m_watchdogBudgetMs = ms; }
    void setHotReload(bool enabled) { m_hotReload = enabled; }

    // Scans both folders and starts every enabled plugin.
    void loadAll();
    void stopAll();
    void reload(const QString& id);

    QList<PluginInfo> plugins() const;
    PluginInfo plugin(const QString& id) const;
    bool isRunning(const QString& id) const;

    // Menus: {pluginId, menuId, title, submenu, context, entity}.
    QVariantList menusForEntity(const QString& entity) const;
    QVariantList globalMenus() const;
    void runMenu(const QString& pluginId, const QString& menuId, const QString& entity, const QString& key);

    // Settings sections: {pluginId, id, title, schema (parsed form), qml (absolute path)}.
    QVariantList settingsSections() const;

    bool setEnabled(const QString& id, bool enabled, QString* error = nullptr);
    // Records approval of the plugin's current permissions and enables it.
    bool approve(const QString& id, QString* error = nullptr);

    // Installs a plugin folder or a .pnplugin (zip) into the user folder,
    // replacing an existing copy with the same id.
    bool install(const QString& path, QString* installedId, QString* error);
    bool uninstall(const QString& id, QString* error);
    // Validates without installing; fills `manifest`.
    bool inspectPackage(const QString& path, PluginManifest* manifest, QString* error);
    // Copies each plugin folder in `folder` into the user folder unless a plugin
    // with the same id is already there. Returns the number copied; the source is left as is.
    int importPluginsFrom(const QString& folder, QString* error);

    // QML -> plugin calls; `reply` runs on the GUI thread.
    void invoke(const QString& pluginId, const QString& token, const QString& name,
                const QVariantList& args, Reply reply);
    void progressCancelled(const QString& pluginId, const QString& token);

    // Changes the application made (user edits, sync): delivered to plugins as
    // data.changed with source "app". Entries: {table (stored), key, operation}.
    void notifyDataChanged(const QVariantList& changes);
    void notifySettingsChanged(const QString& pluginId, const QStringList& keys);
    void notifyViewChanged(const QVariantMap& view);

signals:
    void pluginsChanged();
    void menusChanged();
    // Rows a plugin wrote: {table (stored), key, operation}. The application
    // pushes them into its models so lists refresh.
    void pluginDataChanged(const QVariantList& changes);
    void pluginError(const QString& id, const QString& message);

private:
    void discover();
    void startPlugin(const QString& id);
    void stopPlugin(const QString& id);
    void watchPlugin(const PluginInfo& info);
    static QStringList sourceFiles(const QString& folder);
    void scheduleReload(const QString& id);
    void checkWatchdog();
    void broadcast(const QVariantList& changes, const QString& source, const QString& fromPlugin);
    QString stateKey(const QString& id, const char* key) const;
    bool readEnabled(const PluginInfo& info) const;
    bool approved(const PluginInfo& info) const;
    PluginHost* host(const QString& id) const;
    bool extractPackage(const QString& zipPath, QString* folder, QString* error);

    QPointer<PluginServices> m_services;
    Paths m_paths;
    QString m_databasePath;
    QString m_organization = PN::Identity::settingsOrganization();
    QString m_profile;
    QString m_appVersion;
    QString m_buildFlavor = QStringLiteral("standard");
    bool m_userPluginsAllowed = true;
    bool m_hotReload = true;
    int m_watchdogBudgetMs = 10000;

    QList<PluginInfo> m_plugins;
    QHash<QString, PluginHost*> m_hosts;
    QFileSystemWatcher* m_watcher = nullptr;
    // Per plugin folder: its source files with size and modification time, so
    // a folder change that only touched other files (a template, an Office
    // lock file) doesn't reload the plugin.
    QHash<QString, QStringList> m_sourceFiles;
    QHash<QString, QTimer*> m_reloadTimers;
    QTimer* m_rootRescan = nullptr;
    QTimer* m_watchdog = nullptr;
};

} // namespace PN::Plugins
