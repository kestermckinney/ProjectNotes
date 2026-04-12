// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MOBILESETTINGS_H
#define MOBILESETTINGS_H

#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>

// Lightweight settings class for the mobile app.
// Provides the sync credentials and data-location helpers used by AppController.
// Mirrors the sync-related methods of the desktop AppSettings class.

class MobileSettings
{
public:
    MobileSettings();
    ~MobileSettings();

    static QString dataLocation();

    bool getSyncEnabled() const;
    void setSyncEnabled(bool enabled);

    int getSyncHostType() const;
    void setSyncHostType(int type);

    QString getSyncPostgrestUrl() const;
    void setSyncPostgrestUrl(const QString& url);

    QString getSyncEmail() const;
    void setSyncEmail(const QString& email);

    QString getSyncPassword() const;
    void setSyncPassword(const QString& password);

    QString getSyncSupabaseKey() const;
    void setSyncSupabaseKey(const QString& key);

    QString getSyncEncryptionPhrase() const;
    void setSyncEncryptionPhrase(const QString& phrase);

    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);

private:
    void ensureSettings() const;
    mutable QSettings* m_settings = nullptr;
};

extern MobileSettings global_MobileSettings;

#endif // MOBILESETTINGS_H
