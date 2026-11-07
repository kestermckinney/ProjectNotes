// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QUrl>

class FileFinderService;

namespace PN::Comm { class Office365Service; }
namespace PN::Contacts { class ContactSyncSettingsModel; }

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
    // "Save files into Microsoft Teams folders" (Files.ReadWrite.All).
    Q_PROPERTY(bool teamsFileWrite READ teamsFileWrite WRITE setTeamsFileWrite NOTIFY changed)
    // The last sign-in failed while asking for Files.ReadWrite.All.
    Q_PROPERTY(bool fileWriteSignInFailed READ fileWriteSignInFailed NOTIFY changed)
    // Signed in with saving on, but Microsoft granted only read access.
    Q_PROPERTY(bool teamsFileWriteNotGranted READ teamsFileWriteNotGranted NOTIFY changed)

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
    bool teamsFileWrite() const;
    void setTeamsFileWrite(bool enabled);
    bool fileWriteSignInFailed() const;
    bool teamsFileWriteNotGranted() const;

    Q_INVOKABLE void startSignIn();
    Q_INVOKABLE void requestCommunicationsConsent();
    // Turns Teams file saving off and signs in again without asking for
    // Files.ReadWrite.All.
    Q_INVOKABLE void signInWithoutTeamsFileWrite();
    Q_INVOKABLE void signOut();
    void setContactSyncModel(PN::Contacts::ContactSyncSettingsModel *model) { m_contacts = model; }

signals:
    void changed();

private:
    FileFinderService *m_fileFinder = nullptr;
    PN::Comm::Office365Service *m_service = nullptr;
    PN::Contacts::ContactSyncSettingsModel *m_contacts = nullptr;
};
