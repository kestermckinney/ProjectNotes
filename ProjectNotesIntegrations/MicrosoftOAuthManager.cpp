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

void MicrosoftOAuthManager::setGrantedScopesStore(GrantedScopesReader reader,
                                                    GrantedScopesWriter writer,
                                                    GrantedScopesRemover remover)
{
    m_grantedScopesReader = std::move(reader);
    m_grantedScopesWriter = std::move(writer);
    m_grantedScopesRemover = std::move(remover);
}

void MicrosoftOAuthManager::setSecretNamespace(QString secretNamespace)
{
    m_secretNamespace = std::move(secretNamespace).trimmed();
}

void MicrosoftOAuthManager::setHttpTransport(PN::Comm::HttpTransport *transport)
{
    m_transport = transport;
}

void MicrosoftOAuthManager::setRequestedScopes(QStringList scopes)
{
    for (QString &scope : scopes)
        scope = scope.trimmed();
    scopes.removeAll({});
    scopes.removeDuplicates();
    if (m_requestedScopes == scopes)
        return;
    m_requestedScopes = std::move(scopes);
    m_scopeChangePending = true;
    // Do not disrupt an already-valid File Finder session merely because a
    // separate feature asks the user for additional consent.
    if (m_inProgress) {
        ++m_operationId;
        m_pollTimer->stop();
    }
    if (!m_authenticated && !m_refreshInFlight) {
        ++m_sessionGeneration;
        m_accessToken.clear();
        emit accessTokenChanged({});
    }
    clearDeviceCode();
    setState(m_authenticated, false, tr("Microsoft consent must be renewed for the requested capability."));
}

QStringList MicrosoftOAuthManager::requestedScopeList() const
{
    return m_requestedScopes.isEmpty()
        ? QStringList{QStringLiteral("offline_access"),
                      QStringLiteral("https://graph.microsoft.com/Team.ReadBasic.All"),
                      QStringLiteral("https://graph.microsoft.com/Channel.ReadBasic.All"),
                      QStringLiteral("https://graph.microsoft.com/Files.Read.All")}
        : m_requestedScopes;
}

void MicrosoftOAuthManager::configure(const QString &tenantId, const QString &clientId)
{
    const QString tenant = tenantId.trimmed().isEmpty()
        ? QStringLiteral("organizations") : tenantId.trimmed();
    const QString client = clientId.trimmed();
    if (tenant == m_tenantId && client == m_clientId)
        return;

    ++m_operationId;
    ++m_sessionGeneration;
    m_pollTimer->stop();
    m_refreshTimer->stop();
    m_refreshInFlight = false;
    m_scopeChangePending = false;
    m_tenantId = tenant;
    m_clientId = client;
    m_accessToken.clear();
    m_refreshToken.clear();
    m_grantedScopes.clear();
    clearDeviceCode();
    QString error;
    if (m_secretReader && !client.isEmpty())
        m_refreshToken = m_secretReader(secretAccount(), &error);
    if (m_grantedScopesReader && !client.isEmpty()) {
        m_grantedScopes = m_grantedScopesReader(secretAccount());
        for (QString &scope : m_grantedScopes)
            scope = scope.trimmed();
        m_grantedScopes.removeAll({});
        m_grantedScopes.removeDuplicates();
    }
    emit accessTokenChanged({});
    setState(false, false, !error.isEmpty() ? error
              : (m_refreshToken.isEmpty() ? tr("Not signed in")
                                          : tr("Saved sign-in is ready to restore")));
}

void MicrosoftOAuthManager::restoreSession()
{
    if (m_inProgress || m_refreshInFlight)
        return;
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
    if (m_inProgress || m_refreshInFlight)
        return;
    if (!configurationIsComplete()) {
        setState(false, false, tr("Enter the Microsoft Entra tenant and application client ID."));
        return;
    }

    ++m_operationId;
    const quint64 operationId = m_operationId;
    m_pollTimer->stop();
    m_refreshInFlight = false;
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
    ++m_sessionGeneration;
    m_pollTimer->stop();
    m_refreshTimer->stop();
    m_refreshInFlight = false;
    m_scopeChangePending = false;
    m_accessToken.clear();
    m_refreshToken.clear();
    clearGrantedScopes();
    clearDeviceCode();
    QString error;
    const bool removed = !m_secretRemover || m_secretRemover(secretAccount(), &error);
    emit accessTokenChanged({});
    setState(false, false, removed ? tr("Signed out") : error);
}

void MicrosoftOAuthManager::postForm(const QUrl &url, const QUrlQuery &form,
                                     JsonHandler handler)
{
    const QByteArray body = form.query(QUrl::FullyEncoded).toUtf8();
    if (m_transport) {
        PN::Comm::HttpRequest request;
        request.operationId = QUuid::createUuid();
        request.method = "POST";
        request.url = url;
        request.headers.insert("Content-Type", "application/x-www-form-urlencoded");
        request.body = body;
        m_transport->send(std::move(request), [handler = std::move(handler)](PN::Comm::HttpResponse response) mutable {
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(response.body, &parseError);
            QString error = response.error.displayText;
            if (error.isEmpty() && parseError.error != QJsonParseError::NoError)
                error = QObject::tr("Invalid response from Microsoft: %1").arg(parseError.errorString());
            else if (error.isEmpty() && response.statusCode >= 400
                     && document.object().value(QStringLiteral("error")).toString().isEmpty())
                error = QObject::tr("Microsoft request failed (%1).").arg(response.statusCode);
            handler(document.object(), error);
        });
        return;
    }
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    QNetworkReply *reply = m_network->post(request, body);
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
        acceptTokenResponse(response, true);
    });
}

void MicrosoftOAuthManager::refreshAccessToken(quint64 operationId)
{
    if (operationId != m_operationId || m_refreshToken.isEmpty()
        || !configurationIsComplete() || m_refreshInFlight)
        return;
    m_refreshInFlight = true;
    setState(m_authenticated, true, m_authenticated ? tr("Refreshing Microsoft sign-in…")
                                                    : tr("Restoring saved Microsoft sign-in…"));
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    form.addQueryItem(QStringLiteral("client_id"), m_clientId);
    form.addQueryItem(QStringLiteral("refresh_token"), m_refreshToken);
    form.addQueryItem(QStringLiteral("scope"), refreshScopes());
    postForm(oauthEndpoint(QStringLiteral("token")), form,
             [this, operationId](const QJsonObject &response, const QString &networkError) {
        if (operationId != m_operationId)
            return;
        m_refreshInFlight = false;
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
            clearGrantedScopes();
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
        acceptTokenResponse(response, false);
    });
}

void MicrosoftOAuthManager::acceptTokenResponse(const QJsonObject &response, bool interactiveConsent)
{
    const QString accessToken = response.value(QStringLiteral("access_token")).toString();
    if (accessToken.isEmpty()) {
        setState(m_authenticated, false, tr("Microsoft did not return an access token."));
        return;
    }
    const QString confirmedScopeText = response.value(QStringLiteral("scope")).toString();
    QStringList confirmedScopes = confirmedScopeText.split(u' ', Qt::SkipEmptyParts);
    for (QString &scope : confirmedScopes)
        scope = scope.trimmed();
    confirmedScopes.removeAll({});
    confirmedScopes.removeDuplicates();
    if (interactiveConsent && m_scopeChangePending && confirmedScopes.isEmpty()) {
        // Never replace an active File Finder session with an unconfirmed
        // grant for a separate feature capability.
        setState(m_authenticated, false,
                 tr("Microsoft did not confirm the requested capability scopes."));
        return;
    }
    if (!confirmedScopes.isEmpty()) {
        m_grantedScopes = std::move(confirmedScopes);
        persistGrantedScopes();
    }
    const QString replacement = response.value(QStringLiteral("refresh_token")).toString();
    if (!replacement.isEmpty()) {
        QString error;
        if (m_secretWriter && !m_secretWriter(secretAccount(), replacement, &error)) {
            setState(m_authenticated, false, error);
            return;
        }
        m_refreshToken = replacement;
    }
    m_accessToken = accessToken;
    if (interactiveConsent && m_scopeChangePending) {
        m_scopeChangePending = false;
        ++m_sessionGeneration;
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

void MicrosoftOAuthManager::clearGrantedScopes()
{
    m_grantedScopes.clear();
    if (m_grantedScopesRemover && !m_clientId.isEmpty())
        m_grantedScopesRemover(secretAccount());
}

void MicrosoftOAuthManager::persistGrantedScopes()
{
    if (m_grantedScopesWriter && !m_clientId.isEmpty())
        m_grantedScopesWriter(secretAccount(), m_grantedScopes);
}

QString MicrosoftOAuthManager::requestedScopes() const
{
    return requestedScopeList().join(u' ');
}

QString MicrosoftOAuthManager::refreshScopes() const
{
    // A refresh-token request must retain the provider-confirmed scopes.  On
    // restart there is no requested feature yet, so falling back to File
    // Finder's defaults would silently replace a saved Mail.ReadWrite grant.
    // A separate feature can still request changed scopes through its normal
    // interactive-consent flow.
    if (!m_grantedScopes.isEmpty())
        return m_grantedScopes.join(u' ');
    return requestedScopes();
}

QUrl MicrosoftOAuthManager::oauthEndpoint(const QString &name) const
{
    return kAuthority.resolved(QUrl(QStringLiteral("%1/oauth2/v2.0/%2")
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(m_tenantId)), name)));
}

QString MicrosoftOAuthManager::secretAccount() const
{
    const QString account = m_tenantId + QLatin1Char('/') + m_clientId;
    return m_secretNamespace.isEmpty() ? account : m_secretNamespace + QLatin1Char('/') + account;
}

bool MicrosoftOAuthManager::configurationIsComplete() const
{
    return !m_tenantId.isEmpty() && !m_clientId.isEmpty();
}
