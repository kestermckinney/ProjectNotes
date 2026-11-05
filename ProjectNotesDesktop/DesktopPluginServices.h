// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesPlugins/PluginServices.h"

#include <QHash>
#include <QMutex>
#include <QPointer>

class DesktopAppController;
class PluginUiController;

// The desktop application's side of the plugin API: dialogs (through
// PluginUiController and PluginUiHost.qml), navigation, secrets, email,
// calendar, contacts, Office 365 and launching programs. Lives on the GUI
// thread; see PN::Plugins::PluginServices.
class DesktopPluginServices final : public PN::Plugins::PluginServices {
    Q_OBJECT
public:
    DesktopPluginServices(DesktopAppController* app, PluginUiController* ui, QObject* parent = nullptr);

    void request(const PN::Plugins::PluginManifest& plugin, const QString& method,
                 const QVariantList& args, PN::Plugins::Reply reply) override;
    QStringList capabilities() const override;
    QVariantMap currentView() const override;

    // Set by the application whenever the visible page changes.
    void setCurrentView(const QVariantMap& view);

    static QString secretService(const QString& pluginId);

private:
    void requestUi(const PN::Plugins::PluginManifest& plugin, const QString& method,
                   const QVariantList& args, PN::Plugins::Reply reply);
    void requestApp(const PN::Plugins::PluginManifest& plugin, const QString& method,
                    const QVariantList& args, PN::Plugins::Reply reply);
    void requestContacts(const PN::Plugins::PluginManifest& plugin, const QString& method,
                         const QVariantList& args, PN::Plugins::Reply reply);
    void requestOffice365(const PN::Plugins::PluginManifest& plugin, const QString& method,
                          const QVariantList& args, PN::Plugins::Reply reply);
    void requestProjectFiles(const QString& method, const QVariantList& args, PN::Plugins::Reply reply);

    QPointer<DesktopAppController> m_app;
    QPointer<PluginUiController> m_ui;
    mutable QMutex m_viewMutex;
    QVariantMap m_view;
    QHash<QString, int> m_progressRequests;   // progress token -> request id
};
