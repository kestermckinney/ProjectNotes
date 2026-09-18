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
    void requestFileFinderConsent();
    void requestEmailDraftConsent();
    void startSignIn();
    void signOut();
    // Internal File Finder capability. It is intentionally not a Q_PROPERTY or
    // QML-invokable API and is never exposed by the desktop controller.
    [[nodiscard]] QString fileFinderAccessToken() const { return m_accessToken; }
    [[nodiscard]] Office365Account account() const;
    void refreshIdentity();
    void sendGraphRequest(HttpRequest request, HttpTransport::Completion completion);
    // Upload-session URLs are returned by Graph and must not receive the
    // OAuth Authorization header. The facade still owns transport access.
    void sendUploadRequest(HttpRequest request, HttpTransport::Completion completion);

signals:
    void accountChanged();
    void diagnostic(const QString &message);

private:
    MicrosoftOAuthManager *m_oauth = nullptr;
    HttpTransport *m_transport = nullptr;
    QString m_accessToken;
    QString m_identityLabel;
    QString m_settingsOrganization;
    QString m_tenantId = QStringLiteral("organizations");
    QString m_clientId;
    quint64 m_identityGeneration = 0;
};

} // namespace PN::Comm
