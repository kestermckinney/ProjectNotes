// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "FileFinderTypes.h"

#include <QObject>
#include <QUrl>
#include <QVariantList>

class FileFinderWorker;
class MicrosoftOAuthManager;
class QReadWriteLock;
class QThread;

class FileFinderService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QStringList searchRoots READ searchRoots NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList fileRules READ fileRules NOTIFY settingsChanged)
    Q_PROPERTY(bool office365Enabled READ office365Enabled WRITE setOffice365Enabled NOTIFY settingsChanged)
    Q_PROPERTY(QString office365TenantId READ office365TenantId WRITE setOffice365TenantId NOTIFY settingsChanged)
    Q_PROPERTY(QString office365ClientId READ office365ClientId WRITE setOffice365ClientId NOTIFY settingsChanged)
    Q_PROPERTY(bool office365Authenticated READ office365Authenticated NOTIFY authenticationChanged)
    Q_PROPERTY(bool office365AuthenticationInProgress READ office365AuthenticationInProgress NOTIFY authenticationChanged)
    Q_PROPERTY(QString office365AuthenticationStatus READ office365AuthenticationStatus NOTIFY authenticationChanged)
    Q_PROPERTY(QString office365UserCode READ office365UserCode NOTIFY authenticationChanged)
    Q_PROPERTY(QUrl office365VerificationUrl READ office365VerificationUrl NOTIFY authenticationChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY statusChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString lastScanSummary READ lastScanSummary NOTIFY statusChanged)

public:
    explicit FileFinderService(QObject *parent = nullptr);
    ~FileFinderService() override;

    void initialize(const QString &databasePath, QReadWriteLock *databaseLock,
                    const QString &settingsOrganization);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    QStringList searchRoots() const { return m_roots; }
    QVariantList fileRules() const;
    bool office365Enabled() const { return m_office365Enabled; }
    void setOffice365Enabled(bool enabled);
    QString office365TenantId() const { return m_tenantId; }
    void setOffice365TenantId(const QString &tenantId);
    QString office365ClientId() const { return m_clientId; }
    void setOffice365ClientId(const QString &clientId);
    bool office365Authenticated() const;
    bool office365AuthenticationInProgress() const;
    QString office365AuthenticationStatus() const;
    QString office365UserCode() const;
    QUrl office365VerificationUrl() const;
    bool scanning() const { return m_scanning; }
    QString status() const { return m_status; }
    QString lastScanSummary() const { return m_lastScanSummary; }

    Q_INVOKABLE void addSearchRoot(const QString &path);
    Q_INVOKABLE void removeSearchRoot(int index);
    Q_INVOKABLE void addFileRule(const QString &classification, const QString &pattern);
    Q_INVOKABLE void updateFileRule(int index, const QString &classification,
                                    const QString &pattern);
    Q_INVOKABLE void removeFileRule(int index);
    Q_INVOKABLE void resetDefaultRules();
    Q_INVOKABLE void scanNow();
    Q_INVOKABLE void startOffice365SignIn();
    Q_INVOKABLE void signOutOffice365();

signals:
    void settingsChanged();
    void authenticationChanged();
    void statusChanged();
    void locationsChanged(int inserted, int updated);

private:
    void loadAndMigrateSettings();
    void saveSettings() const;
    void applyConfiguration();
    static QList<FileFinderRule> defaultRules();

    QThread *m_thread = nullptr;
    FileFinderWorker *m_worker = nullptr;
    MicrosoftOAuthManager *m_oauth = nullptr;
    QString m_settingsOrganization;
    QString m_accessToken;
    QStringList m_roots;
    QList<FileFinderRule> m_rules;
    bool m_enabled = false;
    bool m_office365Enabled = false;
    bool m_initialized = false;
    bool m_scanning = false;
    QString m_tenantId = QStringLiteral("organizations");
    QString m_clientId;
    QString m_status = tr("File Finder is not initialized");
    QString m_lastScanSummary;
};
