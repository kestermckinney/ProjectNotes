// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "PluginTypes.h"

#include <QVariant>

#include <memory>

namespace PN::Plugins {

// Windows COM automation for the plugin `com` API (see js/prelude.js for the
// JavaScript Proxy built on it). Objects are referred to by handle strings and
// cross the boundary as {"$handle": "..."} maps. A property get on a member
// that is a method returns {"$method": true} so the Proxy can hand back a
// callable. Lives on the plugin thread, which it initialises as a
// single-threaded apartment. Elsewhere available() is false.
class ComBridge {
public:
    static std::unique_ptr<ComBridge> create();
    virtual ~ComBridge() = default;

    virtual bool available() const = 0;
    virtual QVariant createObject(const QString& progId, PluginError* error) = 0;
    virtual QVariant getActiveObject(const QString& progId, PluginError* error) = 0;   // null when none runs
    virtual QVariant getObject(const QString& moniker, PluginError* error) = 0;
    virtual QVariant call(const QString& handle, const QString& member, const QVariantList& args, PluginError* error) = 0;
    virtual QVariant get(const QString& handle, const QString& member, const QVariantList& index, PluginError* error) = 0;
    virtual void set(const QString& handle, const QString& member, const QVariant& value,
                     const QVariantList& index, PluginError* error) = 0;
    virtual QVariant enumerate(const QString& handle, PluginError* error) = 0;
    virtual void release(const QString& handle) = 0;
    virtual void releaseAll() = 0;
};

} // namespace PN::Plugins
