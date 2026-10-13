// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "PluginManifest.h"
#include "PluginTypes.h"

#include <QHash>
#include <QJSValue>
#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QVariant>

#include <atomic>
#include <memory>

class QJSEngine;
class QNetworkAccessManager;
class QSettings;
class QTimer;

namespace PN::Plugins {

class DataService;
class PluginServices;
class ComBridge;

struct PluginHostConfig {
    PluginManifest manifest;
    QString databasePath;
    QString dataPath;               // writable private folder
    QString tempPath;               // per-plugin temporary folder (app cache)
    QString settingsOrganization;   // "ProjectNotes" + developer profile
    QString profile;
    QString appVersion;
    QString buildFlavor;            // "standard" or "appstore"
    QStringList capabilities;
};

// The JavaScript side calls into C++ only through this object, which the
// prelude keeps private (see js/prelude.js).
class HostBridge final : public QObject {
    Q_OBJECT
public:
    explicit HostBridge(class PluginRuntime* runtime) : m_runtime(runtime) {}
    Q_INVOKABLE int call(const QString& method, const QJSValue& args);
    Q_INVOKABLE QJSValue callSync(const QString& method, const QJSValue& args);
    Q_INVOKABLE void log(const QString& level, const QString& message);
    Q_INVOKABLE void reportError(const QString& where, const QString& message, const QString& stack);
    Q_INVOKABLE void reply(int replyId, bool ok, const QJSValue& value);
private:
    class PluginRuntime* m_runtime;
};

// Runs one plugin: owns its QJSEngine, timers, network access and database
// connection, all on the plugin's thread (see PluginHost). Every method is
// called on that thread.
class PluginRuntime final : public QObject {
    Q_OBJECT
public:
    PluginRuntime(PluginHostConfig config, PluginServices* services);
    ~PluginRuntime() override;

    // Milliseconds since epoch at which the current synchronous call into
    // JavaScript started, or 0 when idle. Read by the watchdog thread.
    qint64 busySince() const { return m_busySince.load(); }
    // Thread-safe: interrupts a runaway synchronous call.
    void interrupt();

    // Called on the plugin thread.
    bool start(QString* error);
    void stop();
    void dispatchMenu(const QString& menuId, const QString& entity, const QString& key,
                      const QString& parameter, const QStringList& include);
    void invoke(const QString& token, const QString& name, const QVariantList& args, Reply reply);
    void deliver(const QString& kind, const QVariant& payload);
    void setProgressCancelled(const QString& token);
    void setGrantedPath(const QString& path);

    const PluginHostConfig& config() const { return m_config; }

signals:
    void logMessage(const QString& level, const QString& message);
    void errorReported(const QString& where, const QString& message);
    void dynamicMenusChanged(const QVariantList& menus);
    // Row changes this plugin wrote, after the write or commit.
    void rowsChanged(const QVariantList& changes);
    void hung(const QString& where);

private:
    friend class HostBridge;
    int callAsync(const QString& method, const QVariantList& args);
    QJSValue callSync(const QString& method, const QVariantList& args);
    void handleAsync(const QString& method, const QVariantList& args, Reply reply);
    QVariant handleSync(const QString& method, const QVariantList& args, PluginError* error);
    void forward(const QString& method, const QVariantList& args, Reply reply);
    bool checkPermission(const QString& method, const QVariantList& args, PluginError* error) const;
    void complete(int id, const QVariant& value, const PluginError& error);
    void deliverReply(int replyId, bool ok, const QVariant& value);
    bool callHook(const char* name, const QJSValueList& args, QJSValue* result = nullptr);
    QJSValue toJs(const QVariant& value) const;

    // Paths
    bool resolveStoragePath(const QString& relative, QString* absolute, PluginError* error) const;
    bool checkFilePath(const QString& path, bool write, PluginError* error) const;

    // http
    void fetch(const QString& url, const QVariantMap& options, Reply reply);

    PluginHostConfig m_config;
    QPointer<PluginServices> m_services;
    QMutex m_engineMutex;
    QJSEngine* m_engine = nullptr;
    HostBridge* m_bridge = nullptr;
    QJSValue m_hooks;
    std::unique_ptr<DataService> m_data;
    std::unique_ptr<QSettings> m_settings;
    QNetworkAccessManager* m_network = nullptr;
    std::unique_ptr<ComBridge> m_com;
    QHash<int, QTimer*> m_timers;
    QHash<int, QByteArray> m_bodies;
    int m_nextBody = 1;
    int m_nextRequest = 1;
    int m_nextReply = 1;
    QHash<int, Reply> m_replies;
    QSet<QString> m_cancelledProgress;
    QStringList m_grantedPaths;
    QVariantList m_dynamicMenus;
    std::atomic<qint64> m_busySince {0};
    QString m_busyWhere;
    bool m_started = false;
    bool m_stopping = false;
};

} // namespace PN::Plugins
