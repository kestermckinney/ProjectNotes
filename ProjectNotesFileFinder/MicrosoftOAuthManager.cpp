// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MicrosoftOAuthManager.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>

namespace {
const QUrl kAuthority(QStringLiteral("https://login.microsoftonline.com/"));
}

MicrosoftOAuthManager::MicrosoftOAuthManager(QObject *parent) : QObject(parent)
{
    m_status = tr("Not signed in");
    m_network = new QNetworkAccessManager(this);
    m_pollTimer = new QTimer(this);
    m_pollTimer->setSingleShot(true);
    connect(m_pollTimer, &QTimer::timeout, this,
            [this] { pollForDeviceToken(m_operationId); });
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    connect(m_refreshTimer, &QTimer::timeout, this,
            [this] { refreshAccessToken(++m_operationId); });
}

void MicrosoftOAuthManager::setSecretStore(SecretReader reader, SecretWriter writer,
                                            SecretRemover remover)
{
    m_secretReader = std::move(reader);
    m_secretWriter = std::move(writer);
    m_secretRemover = std::move(remover);
}

void MicrosoftOAuthManager::configure(const QString &tenantId, const QString &clientId)
{
    const QString tenant = tenantId.trimmed().isEmpty()
        ? QStringLiteral("organizations") : tenantId.trimmed();
    const QString client = clientId.trimmed();
    if (tenant == m_tenantId && client == m_clientId)
        return;

    ++m_operationId;
    m_pollTimer->stop();
    m_refreshTimer->stop();
    m_tenantId = tenant;
    m_clientId = client;
    m_accessToken.clear();
    m_refreshToken.clear();
    clearDeviceCode();
    QString error;
    if (m_secretReader && !client.isEmpty())
        m_refreshToken = m_secretReader(secretAccount(), &error);
    emit accessTokenChanged({});
    setState(false, false, !error.isEmpty() ? error
              : (m_refreshToken.isEmpty() ? tr("Not signed in")
                                          : tr("Saved sign-in is ready to restore")));
}

void MicrosoftOAuthManager::restoreSession()
{
    if (!configurationIsComplete()) {
        setState(false, false, tr("Enter the Microsoft Entra tenant and application client ID."));
        return;
    }
    if (m_refreshToken.isEmpty() && m_secretReader) {
        QString error;
        m_refreshToken = m_secretReader(secretAccount(), &error);
        if (!error.isEmpty()) {
            setState(false, false, error);
            return;
        }
    }
    if (m_refreshToken.isEmpty()) {
        setState(false, false, tr("Not signed in"));
        return;
    }
    refreshAccessToken(++m_operationId);
}

void MicrosoftOAuthManager::startSignIn()
{
    if (!configurationIsComplete()) {
        setState(false, false, tr("Enter the Microsoft Entra tenant and application client ID."));
        return;
    }

    ++m_operationId;
    const quint64 operationId = m_operationId;
    m_pollTimer->stop();
    clearDeviceCode();
    setState(m_authenticated, true, tr("Requesting a Microsoft sign-in code…"));

    QUrlQuery form;
    form.addQueryItem(QStringLiteral("client_id"), m_clientId);
    form.addQueryItem(QStringLiteral("scope"), requestedScopes());
    postForm(oauthEndpoint(QStringLiteral("devicecode")), form,
             [this, operationId](const QJsonObject &response, const QString &networkError) {
        if (operationId != m_operationId)
            return;
        const QString oauthError = response.value(QStringLiteral("error_description")).toString();
        if (!networkError.isEmpty() || !oauthError.isEmpty()) {
            setState(m_authenticated, false,
                     tr("Microsoft sign-in could not start: %1")
                         .arg(!oauthError.isEmpty() ? oauthError : networkError));
            return;
        }
        m_deviceCode = response.value(QStringLiteral("device_code")).toString();
        m_userCode = response.value(QStringLiteral("user_code")).toString();
        m_verificationUrl = QUrl(response.value(QStringLiteral("verification_uri")).toString());
        m_pollIntervalSeconds = qMax(1, response.value(QStringLiteral("interval")).toInt(5));
        m_deviceCodeExpiresAt = QDateTime::currentMSecsSinceEpoch()
            + qint64(response.value(QStringLiteral("expires_in")).toInt(900)) * 1000;
        const QString message = response.value(QStringLiteral("message")).toString(
            tr("Open %1 and enter code %2.").arg(m_verificationUrl.toString(), m_userCode));
        if (m_deviceCode.isEmpty() || m_userCode.isEmpty() || !m_verificationUrl.isValid()) {
            clearDeviceCode();
            setState(m_authenticated, false,
                     tr("Microsoft returned an incomplete device sign-in response."));
            return;
        }
        setState(m_authenticated, true, message);
        m_pollTimer->start(m_pollIntervalSeconds * 1000);
    });
}

void MicrosoftOAuthManager::signOut()
{
    ++m_operationId;
    m_pollTimer->stop();
    m_refreshTimer->stop();
    m_accessToken.clear();
    m_refreshToken.clear();
    clearDeviceCode();
    QString error;
    const bool removed = !m_secretRemover || m_secretRemover(secretAccount(), &error);
    emit accessTokenChanged({});
    setState(false, false, removed ? tr("Signed out") : error);
}

void MicrosoftOAuthManager::postForm(const QUrl &url, const QUrlQuery &form,
                                     JsonHandler handler)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    QNetworkReply *reply = m_network->post(request, form.query(QUrl::FullyEncoded).toUtf8());
    QTimer::singleShot(30000, reply, [reply] {
        if (reply->isRunning())
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this,
            [reply, handler = std::move(handler)]() mutable {
        const QByteArray body = reply->readAll();
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        QString error;
        if (parseError.error != QJsonParseError::NoError)
            error = QObject::tr("Invalid response from Microsoft: %1").arg(parseError.errorString());
        else if (reply->error() != QNetworkReply::NoError
                 && document.object().value(QStringLiteral("error")).toString().isEmpty())
            error = reply->errorString();
        const QJsonObject object = document.object();
        reply->deleteLater();
        handler(object, error);
    });
}

void MicrosoftOAuthManager::pollForDeviceToken(quint64 operationId)
{
    if (operationId != m_operationId || m_deviceCode.isEmpty())
        return;
    if (QDateTime::currentMSecsSinceEpoch() >= m_deviceCodeExpiresAt) {
        clearDeviceCode();
        setState(m_authenticated, false, tr("The Microsoft sign-in code expired. Sign in again."));
        return;
    }

    QUrlQuery form;
    form.addQueryItem(QStringLiteral("grant_type"),
                      QStringLiteral("urn:ietf:params:oauth:grant-type:device_code"));
    form.addQueryItem(QStringLiteral("client_id"), m_clientId);
    form.addQueryItem(QStringLiteral("device_code"), m_deviceCode);
    postForm(oauthEndpoint(QStringLiteral("token")), form,
             [this, operationId](const QJsonObject &response, const QString &networkError) {
        if (operationId != m_operationId)
            return;
        const QString error = response.value(QStringLiteral("error")).toString();
        if (error == QLatin1String("authorization_pending")) {
            m_pollTimer->start(m_pollIntervalSeconds * 1000);
            return;
        }
        if (error == QLatin1String("slow_down")) {
            m_pollIntervalSeconds += 5;
            m_pollTimer->start(m_pollIntervalSeconds * 1000);
            return;
        }
        if (!networkError.isEmpty() || error == QLatin1String("temporarily_unavailable")
            || error == QLatin1String("server_error")) {
            setState(m_authenticated, true,
                     tr("Waiting for Microsoft sign-in; the last check failed temporarily: %1")
                         .arg(!networkError.isEmpty() ? networkError : error));
            m_pollTimer->start(m_pollIntervalSeconds * 1000);
            return;
        }
        if (!error.isEmpty()) {
            clearDeviceCode();
            setState(m_authenticated, false,
                     tr("Microsoft sign-in failed: %1")
                         .arg(response.value(QStringLiteral("error_description")).toString(error)));
            return;
        }
        acceptTokenResponse(response);
    });
}

void MicrosoftOAuthManager::refreshAccessToken(quint64 operationId)
{
    if (operationId != m_operationId || m_refreshToken.isEmpty()
        || !configurationIsComplete())
        return;
    setState(m_authenticated, true, m_authenticated ? tr("Refreshing Microsoft sign-in…")
                                                    : tr("Restoring saved Microsoft sign-in…"));
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    form.addQueryItem(QStringLiteral("client_id"), m_clientId);
    form.addQueryItem(QStringLiteral("refresh_token"), m_refreshToken);
    form.addQueryItem(QStringLiteral("scope"), requestedScopes());
    postForm(oauthEndpoint(QStringLiteral("token")), form,
             [this, operationId](const QJsonObject &response, const QString &networkError) {
        if (operationId != m_operationId)
            return;
        const QString oauthError = response.value(QStringLiteral("error")).toString();
        if (!networkError.isEmpty() || oauthError == QLatin1String("temporarily_unavailable")
            || oauthError == QLatin1String("server_error")) {
            setState(m_authenticated, false,
                     tr("Microsoft token refresh failed temporarily; retrying: %1")
                         .arg(!networkError.isEmpty() ? networkError : oauthError));
            m_refreshTimer->start(60000);
            return;
        }
        if (!oauthError.isEmpty()) {
            m_accessToken.clear();
            if (oauthError == QLatin1String("invalid_grant")
                || oauthError == QLatin1String("interaction_required")) {
                m_refreshToken.clear();
                QString ignored;
                if (m_secretRemover)
                    m_secretRemover(secretAccount(), &ignored);
            }
            emit accessTokenChanged({});
            setState(false, false, tr("Microsoft sign-in must be renewed. Sign in again."));
            return;
        }
        acceptTokenResponse(response);
    });
}

void MicrosoftOAuthManager::acceptTokenResponse(const QJsonObject &response)
{
    const QString accessToken = response.value(QStringLiteral("access_token")).toString();
    if (accessToken.isEmpty()) {
        setState(false, false, tr("Microsoft did not return an access token."));
        return;
    }
    m_accessToken = accessToken;
    const QString replacement = response.value(QStringLiteral("refresh_token")).toString();
    if (!replacement.isEmpty()) {
        QString error;
        if (m_secretWriter && !m_secretWriter(secretAccount(), replacement, &error)) {
            setState(false, false, error);
            return;
        }
        m_refreshToken = replacement;
    }
    clearDeviceCode();
    emit accessTokenChanged(m_accessToken);
    setState(true, false, tr("Signed in to Microsoft 365"));
    scheduleRefresh(response.value(QStringLiteral("expires_in")).toInt(3600));
}

void MicrosoftOAuthManager::setState(bool authenticated, bool inProgress,
                                     const QString &status)
{
    const bool changed = authenticated != m_authenticated || inProgress != m_inProgress
        || status != m_status;
    m_authenticated = authenticated;
    m_inProgress = inProgress;
    m_status = status;
    if (changed)
        emit stateChanged();
    if (!status.isEmpty())
        emit diagnostic(status);
}

void MicrosoftOAuthManager::clearDeviceCode()
{
    m_deviceCode.clear();
    m_userCode.clear();
    m_verificationUrl = QUrl();
    m_deviceCodeExpiresAt = 0;
}

void MicrosoftOAuthManager::scheduleRefresh(int expiresInSeconds)
{
    m_refreshTimer->start(qMax(30, expiresInSeconds - 300) * 1000);
}

QString MicrosoftOAuthManager::requestedScopes() const
{
    return QStringLiteral("offline_access https://graph.microsoft.com/Team.ReadBasic.All "
                          "https://graph.microsoft.com/Channel.ReadBasic.All "
                          "https://graph.microsoft.com/Files.Read.All");
}

QUrl MicrosoftOAuthManager::oauthEndpoint(const QString &name) const
{
    return kAuthority.resolved(QUrl(QStringLiteral("%1/oauth2/v2.0/%2")
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(m_tenantId)), name)));
}

QString MicrosoftOAuthManager::secretAccount() const
{
    return m_tenantId + QLatin1Char('/') + m_clientId;
}

bool MicrosoftOAuthManager::configurationIsComplete() const
{
    return !m_tenantId.isEmpty() && !m_clientId.isEmpty();
}
