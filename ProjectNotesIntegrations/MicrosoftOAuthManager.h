// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "HttpTransport.h"

#include <QJsonObject>
#include <QObject>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;
class QTimer;
class QUrlQuery;

class MicrosoftOAuthManager final : public QObject
{
    Q_OBJECT
public:
    using SecretReader = std::function<QString(const QString &, QString *)>;
    using SecretWriter = std::function<bool(const QString &, const QString &, QString *)>;
    using SecretRemover = std::function<bool(const QString &, QString *)>;
    using GrantedScopesReader = std::function<QStringList(const QString &)>;
    using GrantedScopesWriter = std::function<void(const QString &, const QStringList &)>;
    using GrantedScopesRemover = std::function<void(const QString &)>;

    explicit MicrosoftOAuthManager(QObject *parent = nullptr);

    void setSecretStore(SecretReader reader, SecretWriter writer, SecretRemover remover);
    // Scope metadata is deliberately separate from the secure refresh-token
    // store. It contains only provider-confirmed capability names.
    void setGrantedScopesStore(GrantedScopesReader reader, GrantedScopesWriter writer,
                               GrantedScopesRemover remover);
    // Empty preserves the historic tenant/client key for the production profile.
    void setSecretNamespace(QString secretNamespace);
    // The caller retains ownership. A null transport keeps the production Qt
    // network path; tests can inject a deterministic token-free transport.
    void setHttpTransport(PN::Comm::HttpTransport *transport);
    // Empty retains the legacy File Finder scope set. Callers requesting a new
    // capability must supply its exact consent set explicitly.
    void setRequestedScopes(QStringList scopes);
    [[nodiscard]] QStringList requestedScopeList() const;
    // Scopes echoed by the successful token response, rather than the set a
    // caller asked for. An empty list means the provider did not confirm them.
    [[nodiscard]] QStringList grantedScopeList() const { return m_grantedScopes; }
    [[nodiscard]] quint64 sessionGeneration() const { return m_sessionGeneration; }
    void configure(const QString &tenantId, const QString &clientId);
    void restoreSession();
    Q_INVOKABLE void startSignIn();
    Q_INVOKABLE void signOut();

    bool authenticated() const { return m_authenticated; }
    bool authenticationInProgress() const { return m_inProgress; }
    QString status() const { return m_status; }
    QString userCode() const { return m_userCode; }
    QUrl verificationUrl() const { return m_verificationUrl; }

signals:
    void accessTokenChanged(const QString &accessToken);
    void stateChanged();
    void diagnostic(const QString &message);

private:
    using JsonHandler = std::function<void(const QJsonObject &, const QString &)>;
    void postForm(const QUrl &url, const QUrlQuery &form, JsonHandler handler);
    void pollForDeviceToken(quint64 operationId);
    void refreshAccessToken(quint64 operationId);
    void acceptTokenResponse(const QJsonObject &response, bool interactiveConsent);
    void setState(bool authenticated, bool inProgress, const QString &status);
    void clearDeviceCode();
    void scheduleRefresh(int expiresInSeconds);
    void clearGrantedScopes();
    void persistGrantedScopes();
    QString requestedScopes() const;
    QString refreshScopes() const;
    QUrl oauthEndpoint(const QString &name) const;
    QString secretAccount() const;
    bool configurationIsComplete() const;

    QNetworkAccessManager *m_network = nullptr;
    PN::Comm::HttpTransport *m_transport = nullptr;
    QTimer *m_pollTimer = nullptr;
    QTimer *m_refreshTimer = nullptr;
    SecretReader m_secretReader;
    SecretWriter m_secretWriter;
    SecretRemover m_secretRemover;
    GrantedScopesReader m_grantedScopesReader;
    GrantedScopesWriter m_grantedScopesWriter;
    GrantedScopesRemover m_grantedScopesRemover;
    QString m_secretNamespace;
    QStringList m_requestedScopes;
    QStringList m_grantedScopes;
    QString m_tenantId;
    QString m_clientId;
    QString m_accessToken;
    QString m_refreshToken;
    QString m_deviceCode;
    QString m_userCode;
    QUrl m_verificationUrl;
    qint64 m_deviceCodeExpiresAt = 0;
    int m_pollIntervalSeconds = 5;
    quint64 m_operationId = 0;
    quint64 m_sessionGeneration = 0;
    bool m_authenticated = false;
    bool m_inProgress = false;
    bool m_refreshInFlight = false;
    bool m_scopeChangePending = false;
    QString m_status;
};
