// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

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

    explicit MicrosoftOAuthManager(QObject *parent = nullptr);

    void setSecretStore(SecretReader reader, SecretWriter writer, SecretRemover remover);
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
    void acceptTokenResponse(const QJsonObject &response);
    void setState(bool authenticated, bool inProgress, const QString &status);
    void clearDeviceCode();
    void scheduleRefresh(int expiresInSeconds);
    QString requestedScopes() const;
    QUrl oauthEndpoint(const QString &name) const;
    QString secretAccount() const;
    bool configurationIsComplete() const;

    QNetworkAccessManager *m_network = nullptr;
    QTimer *m_pollTimer = nullptr;
    QTimer *m_refreshTimer = nullptr;
    SecretReader m_secretReader;
    SecretWriter m_secretWriter;
    SecretRemover m_secretRemover;
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
    bool m_authenticated = false;
    bool m_inProgress = false;
    QString m_status;
};
