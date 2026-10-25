// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "appidentity.h"
#include "ProjectNotesPlugins/PluginRegistry.h"
#include "ProjectNotesPlugins/PluginTypes.h"

#include <QHash>
#include <QJSValue>
#include <QSet>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QVariant>

class QQmlEngine;
class QJSEngine;
class QQuickItem;



// The QML side of the plugin system. PluginUiHost.qml listens for
// requestOpened() and shows the matching dialog (message, confirm, prompt,
// form, plugin QML dialog, file pickers, progress, review dialogs), then calls
// respond(). The Settings > Plugins pane uses the management and settings
// methods. Registered as the QML singleton "PluginUiController".
class PluginUiController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY pluginsChanged)
    Q_PROPERTY(QVariantList settingsSections READ settingsSections NOTIFY pluginsChanged)
    Q_PROPERTY(QString userPluginsPath READ userPluginsPath NOTIFY pluginsChanged)
    Q_PROPERTY(bool userPluginsAllowed READ userPluginsAllowed NOTIFY pluginsChanged)
    Q_PROPERTY(int settingsRevision READ settingsRevision NOTIFY settingsRevisionChanged)
public:
    explicit PluginUiController(QObject* parent = nullptr);
    static PluginUiController* instance();
    static PluginUiController* create(QQmlEngine* engine, QJSEngine* scriptEngine);

    void setRegistry(PN::Plugins::PluginRegistry* registry);
    PN::Plugins::PluginRegistry* registry() const { return m_registry; }

    // Called by DesktopPluginServices (GUI thread). Shows a dialog of `kind`
    // and calls `reply` with the result (null when cancelled).
    int openRequest(const QString& pluginId, const QString& kind, const QVariantMap& params, PN::Plugins::Reply reply);
    void closeRequest(int requestId);
    void updateProgress(const QString& token, double fraction, const QString& text);
    void notify(const QString& text, const QString& kind, int seconds);

    QVariantList plugins() const;
    QVariantList settingsSections() const;
    QString userPluginsPath() const;
    bool userPluginsAllowed() const;

    // ── Dialog requests ────────────────────────────────────────────────────
    Q_INVOKABLE void respond(int requestId, const QVariant& result);
    Q_INVOKABLE void cancelRequest(int requestId);
    // Creates a plugin QML file's root item inside `parent`, with the context
    // object `pluginDialog` (dialogs) or `pluginSettings` (Settings sections).
    Q_INVOKABLE QQuickItem* createDialogContent(int requestId, QQuickItem* parent);
    Q_INVOKABLE QQuickItem* createSettingsContent(const QString& pluginId, const QString& qmlPath, QQuickItem* parent);
    Q_INVOKABLE void cancelProgress(int requestId);
    Q_INVOKABLE QString urlToPath(const QUrl& url) const;

    // ── Plugin management (Settings > Plugins) ─────────────────────────────
    Q_INVOKABLE QString setEnabled(const QString& id, bool enabled);
    Q_INVOKABLE QString approve(const QString& id);
    Q_INVOKABLE QVariantMap inspect(const QUrl& path);
    Q_INVOKABLE QString install(const QUrl& path);
    Q_INVOKABLE QString uninstall(const QString& id);
    Q_INVOKABLE void reload(const QString& id);
    Q_INVOKABLE void openPluginsFolder();

    // ── Plugin settings (schema-form sections) ─────────────────────────────
    Q_INVOKABLE QVariantMap settingValues(const QString& pluginId) const;
    Q_INVOKABLE void setSetting(const QString& pluginId, const QString& key, const QVariant& value);
    // Bumped when settings change outside a Settings section (an import), so
    // open sections reread their values.
    int settingsRevision() const { return m_settingsRevision; }

    // ── Settings backup (export to / import from a JSON file) ──────────────
    // Installed plugins with saved settings: {id, name, count}.
    Q_INVOKABLE QVariantList settingsBackupCandidates() const;
    // Writes the settings of `pluginIds` to `file`. Password fields are left
    // out; secrets stay in the system credential store. Returns an error or "".
    Q_INVOKABLE QString exportSettings(const QUrl& file, const QStringList& pluginIds);
    // Reads a file written by exportSettings, or by the Python plugins'
    // Settings Migrator, and writes its values over the current ones.
    // Returns {ok, error, plugins (names), count}.
    Q_INVOKABLE QVariantMap importSettings(const QUrl& file);
    // File > Export Settings: application settings plus every plugin's settings
    // in one file. Passwords, the sync encryption phrase, window layout and
    // other per-computer state are left out. Returns an error or "".
    Q_INVOKABLE QString exportAllSettings(const QUrl& file);
    // Reads a file from exportAllSettings (or exportSettings) and writes its
    // values over the current ones. Application settings take full effect
    // after a restart. Returns {ok, error, plugins (names), count, appCount}.
    Q_INVOKABLE QVariantMap importAllSettings(const QUrl& file);
    // Calls an exported plugin function from a Settings section or dialog.
    QJSValue invokePlugin(QJSEngine* engine, const QString& pluginId, const QString& token, const QString& name, const QJSValue& args);

    void setSettingsOrganization(const QString& organization) { m_organization = organization; }

    // Where "password" form fields go instead of the plugin settings: the
    // plugin's own credential-store namespace, which secrets.get(key) reads.
    // Tests substitute an in-memory writer so they never touch the keyring.
    using SecretWriter = std::function<bool(const QString& pluginId, const QString& key,
                                            const QString& secret, QString* error)>;
    static void setSecretWriter(SecretWriter writer) { s_secretWriter = std::move(writer); }

signals:
    void requestOpened(int requestId, const QString& kind, const QVariantMap& params);
    void requestClosed(int requestId);
    void progressUpdated(const QString& token, double fraction, const QString& text);
    void notificationRequested(const QString& text, const QString& kind, int seconds);
    void pluginsChanged();
    void settingsRevisionChanged();
    void pluginError(const QString& id, const QString& name, const QString& message);

private:
    struct Request {
        QString pluginId;
        QString kind;
        QVariantMap params;
        PN::Plugins::Reply reply;
    };
    QJsonObject pluginSettingsJson(const QStringList& pluginIds) const;
    QJsonObject appSettingsJson() const;
    // Writes {plugin id: {key: value}} groups to the plugin settings and tells
    // running plugins. `legacy` maps Python groups through legacySettings.
    // Returns {plugins (names), count}.
    QVariantMap applyPluginSettings(const QJsonObject& groups, bool legacy);
    // Saves a password field through s_secretWriter, reporting a failure to the
    // user. A blank value keeps the stored one. True once nothing is left to keep
    // in the plugin settings.
    bool storePasswordField(const QString& pluginId, const QString& key, const QString& secret);
    QSet<QString> passwordFields(const QString& pluginId) const;

    QPointer<PN::Plugins::PluginRegistry> m_registry;
    QHash<int, Request> m_requests;
    int m_nextRequest = 1;
    int m_settingsRevision = 0;
    QString m_organization = PN::Identity::settingsOrganization();
    static SecretWriter s_secretWriter;
};

// `pluginDialog` / `pluginSettings` in plugin QML.
class PluginQmlContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant data READ data CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged)
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
public:
    PluginQmlContext(PluginUiController* controller, QJSEngine* engine, int requestId, QString pluginId, QString token,
                     QVariant data, QString title, QObject* parent = nullptr);

    QVariant data() const { return m_data; }
    QString title() const { return m_title; }
    bool busy() const { return m_busy; }
    void setBusy(bool busy) { if (m_busy != busy) { m_busy = busy; emit busyChanged(); } }
    QVariantMap values() const;

    Q_INVOKABLE void accept(const QVariant& result = QVariant());
    Q_INVOKABLE void reject();
    Q_INVOKABLE QJSValue invoke(const QString& name, const QJSValue& args = QJSValue());
    // Settings sections.
    Q_INVOKABLE QVariant get(const QString& key) const;
    Q_INVOKABLE void set(const QString& key, const QVariant& value);

signals:
    void busyChanged();
    void valuesChanged();

private:
    PluginUiController* m_controller;
    QJSEngine* m_engine;
    int m_requestId;
    QString m_pluginId;
    QString m_token;
    QVariant m_data;
    QString m_title;
    bool m_busy = false;
};
