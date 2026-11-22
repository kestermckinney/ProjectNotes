// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "DirectDownloadMigration.h"

#include "appidentity.h"

#include <QDir>
#include <QSettings>

namespace DirectDownloadMigration {

void exportSettings()
{
    const QString folder = PN::Identity::dataLocation() + QLatin1Char('/') + QLatin1String(kSettingsFolder);
    if (!QDir().mkpath(folder))
        return;
    for (const QString application : {QStringLiteral("AppSettings"), QStringLiteral("PluginSettings")}) {
        const QSettings from(PN::Identity::settingsOrganization(), application);
        QSettings to(folder + QLatin1Char('/') + application + QStringLiteral(".plist"), QSettings::NativeFormat);
        to.clear();
        for (const QString& key : from.allKeys())
            to.setValue(key, from.value(key));
        to.sync();
    }
}

} // namespace DirectDownloadMigration
