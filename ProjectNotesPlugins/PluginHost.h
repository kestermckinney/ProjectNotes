// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "PluginRuntime.h"

#include <QObject>
#include <QPointer>
#include <QThread>

namespace PN::Plugins {

// The GUI-thread handle for one running plugin: owns the plugin's QThread and
// the PluginRuntime living on it, and marshals calls in both directions.
class PluginHost final : public QObject {
    Q_OBJECT
public:
    PluginHost(PluginHostConfig config, PluginServices* services, QObject* parent = nullptr);
    ~PluginHost() override;

    const PluginManifest& manifest() const { return m_config.manifest; }
    bool isRunning() const { return m_running; }
    QVariantList dynamicMenus() const { return m_dynamicMenus; }

    void start();
    // Calls onStop, then tears the engine down and joins the thread.
    void stop();

    // Watchdog support, safe from the GUI thread.
    qint64 busySince() const;
    void interrupt();

    void dispatchMenu(const QString& menuId, const QString& entity, const QString& key,
                      const QString& parameter, const QStringList& include);
    // `reply` is called on the GUI thread.
    void invoke(const QString& token, const QString& name, const QVariantList& args, Reply reply);
    void deliver(const QString& kind, const QVariant& payload);
    void setProgressCancelled(const QString& token);

signals:
    void started(bool ok, const QString& error);
    void dynamicMenusChanged();
    void rowsChanged(const QVariantList& changes);
    void foreignSettingsChanged(const QString& pluginId, const QStringList& keys);
    void errorReported(const QString& where, const QString& message);
    void logMessage(const QString& level, const QString& message);
    void hung(const QString& where);

private:
    PluginHostConfig m_config;
    QPointer<PluginServices> m_services;
    QThread m_thread;
    PluginRuntime* m_runtime = nullptr;
    QVariantList m_dynamicMenus;
    bool m_running = false;
};

} // namespace PN::Plugins
