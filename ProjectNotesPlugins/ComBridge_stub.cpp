// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// COM exists only on Windows. On macOS and Linux the `com` namespace is not
// offered (app.capabilities.has("com") is false) and every call is refused.

#include "ComBridge.h"

namespace PN::Plugins {
namespace {

PluginError unavailable()
{
    return PluginError::make(QStringLiteral("unavailable"), QStringLiteral("COM automation is only available on Windows."));
}

class UnavailableComBridge final : public ComBridge {
public:
    bool available() const override { return false; }
    QVariant createObject(const QString&, PluginError* e) override { *e = unavailable(); return {}; }
    QVariant getActiveObject(const QString&, PluginError* e) override { *e = unavailable(); return {}; }
    QVariant getObject(const QString&, PluginError* e) override { *e = unavailable(); return {}; }
    QVariant call(const QString&, const QString&, const QVariantList&, PluginError* e) override { *e = unavailable(); return {}; }
    QVariant get(const QString&, const QString&, const QVariantList&, PluginError* e) override { *e = unavailable(); return {}; }
    void set(const QString&, const QString&, const QVariant&, const QVariantList&, PluginError* e) override { *e = unavailable(); }
    QVariant enumerate(const QString&, PluginError* e) override { *e = unavailable(); return {}; }
    void release(const QString&) override {}
    void releaseAll() override {}
};

} // namespace

std::unique_ptr<ComBridge> ComBridge::create()
{
    return std::make_unique<UnavailableComBridge>();
}

} // namespace PN::Plugins
