// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "HttpTransport.h"

#include <QObject>

#include <functional>

class MicrosoftOAuthManager;

namespace PN::Comm {

struct Office365Account {
    QString label;
    QStringList grantedScopes;
    quint64 generation = 0;
    bool authenticated = false;
};

// Graph clients use this facade rather than handling OAuth access tokens. The
// facade owns the current token only long enough to decorate a request.
class Office365Service final : public QObject {
    Q_OBJECT
public:
    using SecretReader = std::function<QString(const QString &, QString *)>;
    using SecretWriter = std::function<bool(const QString &, const QString &, QString *)>;
    using SecretRemover = std::function<bool(const QString &, QString *)>;
    using GrantedScopesReader = std::function<QStringList(const QString &)>;
    using GrantedScopesWriter = std::function<void(const QString &, const QStringList &)>;
    using GrantedScopesRemover = std::function<void(const QString &)>;

    explicit Office365Service(QObject *parent = nullptr);
    void setOAuthManager(MicrosoftOAuthManager *manager);
    void setHttpTransport(HttpTransport *transport);
    void setSecretStore(SecretReader reader, SecretWriter writer, SecretRemover remover);
    void setGrantedScopesStore(GrantedScopesReader reader, GrantedScopesWriter writer,
                               GrantedScopesRemover remover);
    void setSecretNamespace(QString secretNamespace);
    // File Finder retains compatibility properties, but this shared facade is
    // the sole writer of the canonical tenant/client configuration.
    void configureIdentity(const QString &settingsOrganization, QString tenantId, QString clientId);
    void setIdentity(QString tenantId, QString clientId);
    void persistIdentity();
    [[nodiscard]] QString tenantId() const { return m_tenantId; }
    [[nodiscard]] QString clientId() const { return m_clientId; }
    [[nodiscard]] bool authenticated() const;
    [[nodiscard]] bool authenticationInProgress() const;
    [[nodiscard]] QString authenticationStatus() const;
    [[nodiscard]] QString userCode() const;
    [[nodiscard]] QUrl verificationUrl() const;
    void restoreSession();

    // Saving into Microsoft Teams project folders needs Files.ReadWrite.All.
    // Some organizations never grant it to their app registration, and asking
    // for a scope that cannot be consented fails the whole sign-in (email
    // drafts, calendar, contacts and File Finder included). With saving
    // turned off, every request for Files.ReadWrite.All becomes
    // Files.Read.All, so Teams stays readable. Stored as
    // Office365/teamsFileWrite; on by default.
    [[nodiscard]] bool teamsFileWriteEnabled() const { return m_teamsFileWrite; }
    void setTeamsFileWriteEnabled(bool enabled);
    // Files.ReadWrite.All, or Files.Read.All with saving turned off.
    [[nodiscard]] QString teamsFileScope() const;
    // What File Finder's Teams search and Teams project folders need:
    // Team.ReadBasic.All and Channel.ReadBasic.All to list joined teams and
    // their channels, plus teamsFileScope() for the channel files.
    [[nodiscard]] QStringList teamsScopes() const;
    // True when saving is on and the grant allows it. A grant that was never
    // recorded counts as allowing it; Graph refuses the upload if not.
    [[nodiscard]] bool teamsFilesWritable() const;
    // The last interactive sign-in failed while it asked for
    // Files.ReadWrite.All, which is the usual sign of an organization that
    // has not approved it. Cleared when a sign-in starts or succeeds.
    [[nodiscard]] bool fileWriteSignInFailed() const { return m_fileWriteSignInFailed; }
    static bool isFileWriteScope(const QString &scope);

    void requestFileFinderConsent();
    // includeFileWrite adds the Teams scopes (see teamsScopes()), for File
    // Finder's Teams search and Microsoft Teams project folders. They are
    // also added when the saved grant already reaches Teams or SharePoint
    // files.
    void requestCommunicationsConsent(bool includeContacts = false, bool exportContacts = false,
                                      bool includeFileWrite = false);
    void requestContactConsent(bool exportEnabled);
    // Adds scopes (for example a plugin's office365.scopes, or the Teams write
    // scope after a refused upload) to those already granted, for the next
    // sign-in. They are kept until sign-out, so the Settings sign-in, which
    // sets its own scope list, still asks for them.
    void requestAdditionalScopes(const QStringList &scopes);
    void startSignIn();
    void signOut();
    // Internal File Finder capability. It is intentionally not a Q_PROPERTY or
    // QML-invokable API and is never exposed by the desktop controller.
    [[nodiscard]] QString fileFinderAccessToken() const { return m_accessToken; }
    [[nodiscard]] Office365Account account() const;
    void refreshIdentity();
    void sendGraphRequest(HttpRequest request, HttpTransport::Completion completion);
    void cancelGraphRequest(const QUuid &operationId);
    // Upload-session URLs are returned by Graph and must not receive the
    // OAuth Authorization header. The facade still owns transport access.
    void sendUploadRequest(HttpRequest request, HttpTransport::Completion completion);

signals:
    void accountChanged();
    void diagnostic(const QString &message);

private:
    // Replaces Files.ReadWrite.All with Files.Read.All when saving is off.
    [[nodiscard]] QStringList applyTeamsFilePolicy(QStringList scopes) const;

    MicrosoftOAuthManager *m_oauth = nullptr;
    HttpTransport *m_transport = nullptr;
    QString m_accessToken;
    QString m_identityLabel;
    QString m_settingsOrganization;
    QString m_tenantId = QStringLiteral("organizations");
    QString m_clientId;
    quint64 m_identityGeneration = 0;
    // Scopes features asked for through requestAdditionalScopes().
    QStringList m_additionalScopes;
    bool m_teamsFileWrite = true;
    bool m_fileWriteSignInFailed = false;
};

} // namespace PN::Comm
