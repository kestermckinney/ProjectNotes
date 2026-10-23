// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MobileSettings.h"

#include "credentialstore.h"
#include "QLogger.h"

using namespace QLogger;

namespace {
const QString kSyncCredentialService = QStringLiteral("ProjectNotes sync");
}

MobileSettings global_MobileSettings;
bool MobileSettings::s_testSupabase = false;

void MobileSettings::setTestSupabase(bool test) { s_testSupabase = test; }
bool MobileSettings::isTestSupabase()           { return s_testSupabase; }

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
    return syncSecret(QStringLiteral("Password"));
}

void MobileSettings::setSyncPassword(const QString& password)
{
    setSyncSecret(QStringLiteral("Password"), password);
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
    return syncSecret(QStringLiteral("EncryptionPhrase"));
}

void MobileSettings::setSyncEncryptionPhrase(const QString& phrase)
{
    setSyncSecret(QStringLiteral("EncryptionPhrase"), phrase);
}

// Earlier versions kept the password and phrase in the settings plist in clear
// text ("Sync/<account>"). The first read moves such a value into the Keychain,
// and deletes the plain copy only once the Keychain holds it.
QString MobileSettings::syncSecret(const QString& account) const
{
    const auto cached = m_syncSecrets.constFind(account);
    if (cached != m_syncSecrets.constEnd())
        return *cached;

    ensureSettings();
    QString error;
    QString value = CredentialStore::read(kSyncCredentialService, account, &error);
    const QString legacyKey = QStringLiteral("Sync/") + account;
    const QString legacy = m_settings->value(legacyKey).toString();
    if (!legacy.isEmpty() && error.isEmpty()) {
        if (value.isEmpty() && !CredentialStore::write(kSyncCredentialService, account, legacy, &error))
            QLog_Error(ERRORLOG, QStringLiteral("Could not move the sync %1 to the Keychain: %2").arg(account, error));
        else
            m_settings->remove(legacyKey);
    } else if (!error.isEmpty()) {
        QLog_Error(ERRORLOG, QStringLiteral("Could not read the sync %1 from the Keychain: %2").arg(account, error));
    }
    if (value.isEmpty())
        value = legacy;
    // Before the first unlock after a restart the Keychain can't answer: ask again later.
    if (error.isEmpty() || !value.isEmpty())
        m_syncSecrets.insert(account, value);
    return value;
}

void MobileSettings::setSyncSecret(const QString& account, const QString& value)
{
    ensureSettings();
    QString error;
    const bool stored = value.isEmpty() ? CredentialStore::remove(kSyncCredentialService, account, &error)
                                        : CredentialStore::write(kSyncCredentialService, account, value, &error);
    // Kept for this session even when the Keychain refused it, so sync still runs.
    m_syncSecrets.insert(account, value);
    m_settings->remove(QStringLiteral("Sync/") + account);
    if (!stored)
        QLog_Error(ERRORLOG, QStringLiteral("Could not save the sync %1 in the Keychain: %2").arg(account, error));
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
