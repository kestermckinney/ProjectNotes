// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MobileSettings.h"

MobileSettings global_MobileSettings;

MobileSettings::MobileSettings()
{
    // m_settings is intentionally not created here — this object may be constructed
    // as a global before QGuiApplication exists.  ensureSettings() initializes it
    // lazily on first use, by which time the application object is guaranteed live.
}

void MobileSettings::ensureSettings() const
{
    if (!m_settings) {
        m_settings = new QSettings("ProjectNotes", "MobileSettings");
        m_settings->setFallbacksEnabled(false);
    }
}

MobileSettings::~MobileSettings()
{
    if (m_settings) {
        m_settings->sync();
        delete m_settings;
    }
}

QString MobileSettings::dataLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

bool MobileSettings::getSyncEnabled() const
{
    ensureSettings();
    return m_settings->value("Sync/Enabled", false).toBool();
}

void MobileSettings::setSyncEnabled(bool enabled)
{
    ensureSettings();
    m_settings->setValue("Sync/Enabled", enabled);
}

int MobileSettings::getSyncHostType() const
{
    ensureSettings();
    return m_settings->value("Sync/HostType", 0).toInt();
}

void MobileSettings::setSyncHostType(int type)
{
    ensureSettings();
    m_settings->setValue("Sync/HostType", type);
}

QString MobileSettings::getSyncPostgrestUrl() const
{
    ensureSettings();
    return m_settings->value("Sync/PostgrestUrl").toString();
}

void MobileSettings::setSyncPostgrestUrl(const QString& url)
{
    ensureSettings();
    m_settings->setValue("Sync/PostgrestUrl", url);
}

QString MobileSettings::getSyncEmail() const
{
    ensureSettings();
    return m_settings->value("Sync/Email").toString();
}

void MobileSettings::setSyncEmail(const QString& email)
{
    ensureSettings();
    m_settings->setValue("Sync/Email", email);
}

QString MobileSettings::getSyncPassword() const
{
    ensureSettings();
    return m_settings->value("Sync/Password").toString();
}

void MobileSettings::setSyncPassword(const QString& password)
{
    ensureSettings();
    m_settings->setValue("Sync/Password", password);
}

QString MobileSettings::getSyncSupabaseKey() const
{
    ensureSettings();
    return m_settings->value("Sync/SupabaseKey").toString();
}

void MobileSettings::setSyncSupabaseKey(const QString& key)
{
    ensureSettings();
    m_settings->setValue("Sync/SupabaseKey", key);
}

QString MobileSettings::getSyncEncryptionPhrase() const
{
    ensureSettings();
    return m_settings->value("Sync/EncryptionPhrase").toString();
}

void MobileSettings::setSyncEncryptionPhrase(const QString& phrase)
{
    ensureSettings();
    m_settings->setValue("Sync/EncryptionPhrase", phrase);
}

QVariant MobileSettings::getValue(const QString& key, const QVariant& defaultValue) const
{
    ensureSettings();
    return m_settings->value(key, defaultValue);
}

void MobileSettings::setValue(const QString& key, const QVariant& value)
{
    ensureSettings();
    m_settings->setValue(key, value);
}
