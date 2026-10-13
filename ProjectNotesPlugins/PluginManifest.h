// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace PN::Plugins {

struct MenuContribution {
    QString id;
    QString title;
    QString context;        // "record" or "global"
    QString entity;         // published entity for "record"
    QString submenu;
    QStringList include;    // child entities loaded into the record context
    QString parameter;
    QStringList platforms;
    bool dynamic = false;   // registered at run time through menus.register()
};

struct SettingsContribution {
    QString id;
    QString title;
    QString schema;         // form schema file, relative to the plugin folder
    QString qml;            // or a QML file
};

// plugin.json. See the manifest reference in docs/QJSEnginePluginsPlan.md.
struct PluginManifest {
    static constexpr int kSupportedApiVersion = 1;

    QString id;
    QString name;
    QString version;
    int apiVersion = 0;
    QString description;
    QString author;
    QString homepage;
    QString minAppVersion;
    QStringList platforms;          // empty = all
    QString main;
    QStringList permissions;
    QStringList httpHosts;
    QStringList comObjects;
    QStringList office365Scopes;
    QList<MenuContribution> menus;
    QList<SettingsContribution> settings;

    QString folder;                 // absolute plugin folder
    bool bundled = false;

    // Parses `folder`/plugin.json. Returns false with a reason in `error`.
    static bool load(const QString& folder, PluginManifest* out, QString* error);

    bool supportsPlatform(const QString& platform) const;
    // A stable fingerprint of everything the user approves when enabling a
    // plugin (permissions, hosts, COM objects, scopes, whether it ships QML).
    QString approvalFingerprint() const;
    bool shipsQml() const;

    // Permission checks. data.read / data.write take published entity names.
    bool hasPermission(const QString& permission) const;
    bool canRead(const QString& entity) const;
    bool canWrite(const QString& entity) const;
    bool allowsHost(const QString& host) const;
    bool allowsComObject(const QString& progIdOrMoniker) const;
};

QString currentPlatform();

} // namespace PN::Plugins
