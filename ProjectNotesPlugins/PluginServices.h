// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "PluginTypes.h"

#include <QObject>
#include <QStringList>
#include <QVariantList>

namespace PN::Plugins {

struct PluginManifest;

// Application services a plugin reaches through the host: dialogs, navigation,
// email, calendar, contacts, Office 365, secrets, launching programs. The host
// application implements it (DesktopPluginServices); the library only knows
// this interface, so it stays free of Widgets and Quick.
//
// request() is always called on this object's thread (the GUI thread) and the
// reply may be delivered later from that thread. Permissions declared in the
// manifest are checked by the plugin host before request() is called; method
// names match the JavaScript API ("ui.confirm", "email.compose", ...).
class PluginServices : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    virtual void request(const PluginManifest& plugin, const QString& method,
                         const QVariantList& args, Reply reply) = 0;

    // Capability names this build offers (app.capabilities), e.g.
    // "email.compose", "calendar.draft", "office365", "process.launch".
    virtual QStringList capabilities() const = 0;

    // Read synchronously from a plugin thread; must be thread-safe.
    virtual QVariantMap currentView() const { return {}; }

signals:
    // The visible page changed; delivered to plugins as app.onViewChanged.
    void viewChanged(const QVariantMap& view);
};

} // namespace PN::Plugins
