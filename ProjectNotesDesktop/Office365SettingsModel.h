// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QUrl>

class FileFinderService;

namespace PN::Comm { class Office365Service; }

// Token-free QML facade for the one shared Microsoft account.  File Finder
// remains the compatibility owner of its scanner enablement, but no QML view
// reaches its OAuth implementation directly.
class Office365SettingsModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString tenantId READ tenantId WRITE setTenantId NOTIFY changed)
    Q_PROPERTY(QString clientId READ clientId WRITE setClientId NOTIFY changed)
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY changed)
    Q_PROPERTY(bool authenticationInProgress READ authenticationInProgress NOTIFY changed)
    Q_PROPERTY(QString authenticationStatus READ authenticationStatus NOTIFY changed)
    Q_PROPERTY(QString userCode READ userCode NOTIFY changed)
    Q_PROPERTY(QUrl verificationUrl READ verificationUrl NOTIFY changed)
    Q_PROPERTY(QString accountLabel READ accountLabel NOTIFY changed)

public:
    explicit Office365SettingsModel(FileFinderService *fileFinder,
                                   PN::Comm::Office365Service *service,
                                   QObject *parent = nullptr);

    QString tenantId() const;
    void setTenantId(const QString &value);
    QString clientId() const;
    void setClientId(const QString &value);
    bool authenticated() const;
    bool authenticationInProgress() const;
    QString authenticationStatus() const;
    QString userCode() const;
    QUrl verificationUrl() const;
    QString accountLabel() const;

    Q_INVOKABLE void startSignIn();
    Q_INVOKABLE void requestEmailDraftConsent();
    Q_INVOKABLE void signOut();

signals:
    void changed();

private:
    FileFinderService *m_fileFinder = nullptr;
    PN::Comm::Office365Service *m_service = nullptr;
};
