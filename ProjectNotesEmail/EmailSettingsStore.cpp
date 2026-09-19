// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "EmailSettingsStore.h"

namespace PN::Comm {

EmailSettingsStore::EmailSettingsStore(QSettings &settings) : m_settings(settings)
{
    m_settings.setFallbacksEnabled(false);
}

BackendId EmailSettingsStore::preferredBackend() const
{
    return backendIdFromStableString(m_settings.value(QStringLiteral("Email/v1/PreferredBackend")).toString())
        .value_or(BackendId::Mailto);
}

void EmailSettingsStore::setPreferredBackend(BackendId backend)
{
    m_settings.setValue(QStringLiteral("Email/v1/PreferredBackend"), toStableString(backend));
    m_settings.sync();
}

QString EmailSettingsStore::thunderbirdPath() const
{
    return m_settings.value(QStringLiteral("Email/v1/ThunderbirdPath")).toString();
}

void EmailSettingsStore::setThunderbirdPath(const QString &path)
{
    m_settings.setValue(QStringLiteral("Email/v1/ThunderbirdPath"), path);
    m_settings.sync();
}

} // namespace PN::Comm
