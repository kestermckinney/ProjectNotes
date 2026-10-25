// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QMetaType>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <functional>

namespace PN::Plugins {

// The error a rejected host call or failed plugin operation reports. In
// JavaScript it becomes a PluginError (see js/prelude.js).
struct PluginError {
    QString code;          // "permission-denied", "not-found", "validation-failed", ...
    QString message;
    QString permission;    // the missing permission for "permission-denied"
    QVariantMap details;

    bool isError() const { return !code.isEmpty(); }
    QVariantMap toMap() const
    {
        QVariantMap map {{QStringLiteral("code"), code}, {QStringLiteral("message"), message}};
        if (!permission.isEmpty())
            map.insert(QStringLiteral("permission"), permission);
        if (!details.isEmpty())
            map.insert(QStringLiteral("details"), details);
        return map;
    }
    static PluginError make(const QString& code, const QString& message, const QVariantMap& details = {})
    {
        return {code, message, {}, details};
    }
    static PluginError denied(const QString& permission)
    {
        return {QStringLiteral("permission-denied"),
                QStringLiteral("This plugin did not declare the \"%1\" permission.").arg(permission),
                permission, {}};
    }
};

// Completion for asynchronous host calls: exactly one of value / error.
using Reply = std::function<void(const QVariant& value, const PluginError& error)>;

} // namespace PN::Plugins

Q_DECLARE_METATYPE(PN::Plugins::PluginError)
