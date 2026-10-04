// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "EmailSettingsStore.h"

namespace PN::Comm {

bool isBackendAvailableOnThisPlatform(BackendId backend)
{
    switch (backend) {
    case BackendId::Outlook:
    case BackendId::OutlookClassic:
#if defined(Q_OS_WIN)
        return true;
#else
        return false;
#endif
    case BackendId::MacMail:
#if defined(Q_OS_DARWIN)
        return true;
#else
        return false;
#endif
    case BackendId::Graph:
    case BackendId::DefaultApp:
    case BackendId::Thunderbird:
    case BackendId::ICloud:
        return true;
    }
    return false;
}

EmailSettingsStore::EmailSettingsStore(QSettings &settings) : m_settings(settings)
{
    m_settings.setFallbacksEnabled(false);
}

BackendId EmailSettingsStore::preferredBackend() const
{
    if (const auto parsed = backendIdFromStableString(
            m_settings.value(QStringLiteral("Email/v1/PreferredBackend")).toString());
        parsed && isBackendAvailableOnThisPlatform(*parsed))
        return *parsed;
    // Nothing has been configured yet. macOS ships a Mail/Calendar pair whose
    // From/organizer field is otherwise left blank (see MacMail), so it is a
    // better first-run default there than the generic file-opening backend.
#if defined(Q_OS_DARWIN)
    return BackendId::MacMail;
#else
    return BackendId::DefaultApp;
#endif
}

void EmailSettingsStore::setPreferredBackend(BackendId backend)
{
    if (!isBackendAvailableOnThisPlatform(backend)) return;
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

QString EmailSettingsStore::senderAddress() const
{
    return m_settings.value(QStringLiteral("Email/v1/SenderAddress")).toString();
}

void EmailSettingsStore::setSenderAddress(const QString &address)
{
    m_settings.setValue(QStringLiteral("Email/v1/SenderAddress"), address.trimmed());
    m_settings.sync();
}

} // namespace PN::Comm
