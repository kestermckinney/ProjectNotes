// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "Office365Service.h"
#include "MicrosoftOAuthManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

namespace PN::Comm {

Office365Service::Office365Service(QObject *parent) : QObject(parent)
{
    setOAuthManager(new MicrosoftOAuthManager(this));
}

void Office365Service::setOAuthManager(MicrosoftOAuthManager *manager)
{
    if (m_oauth == manager)
        return;
    if (m_oauth)
        disconnect(m_oauth, nullptr, this, nullptr);
    m_oauth = manager;
    m_accessToken.clear();
    if (m_oauth) {
        m_oauth->setHttpTransport(m_transport);
        connect(m_oauth, &MicrosoftOAuthManager::accessTokenChanged, this,
                [this](const QString &token) {
                    m_accessToken = token;
                    if (token.isEmpty()) {
                        m_identityLabel.clear();
                        m_identityGeneration = 0;
                    }
                    emit accountChanged();
                    if (!token.isEmpty())
                        refreshIdentity();
                });
        connect(m_oauth, &MicrosoftOAuthManager::stateChanged, this,
                [this] { emit accountChanged(); });
        connect(m_oauth, &MicrosoftOAuthManager::diagnostic, this,
                [this](const QString &message) { emit diagnostic(message); });
    }
    emit accountChanged();
}

void Office365Service::setSecretStore(SecretReader reader, SecretWriter writer, SecretRemover remover)
{
    if (m_oauth)
        m_oauth->setSecretStore(std::move(reader), std::move(writer), std::move(remover));
}

void Office365Service::setGrantedScopesStore(GrantedScopesReader reader,
                                              GrantedScopesWriter writer,
                                              GrantedScopesRemover remover)
{
    if (m_oauth)
        m_oauth->setGrantedScopesStore(std::move(reader), std::move(writer), std::move(remover));
}

void Office365Service::setSecretNamespace(QString secretNamespace)
{
    if (m_oauth)
        m_oauth->setSecretNamespace(std::move(secretNamespace));
}

void Office365Service::setHttpTransport(HttpTransport *transport)
{
    m_transport = transport;
    if (m_oauth)
        m_oauth->setHttpTransport(transport);
}

void Office365Service::configureIdentity(const QString &settingsOrganization,
                                         QString tenantId, QString clientId)
{
    m_settingsOrganization = settingsOrganization;
    m_tenantId = tenantId.trimmed().isEmpty() ? QStringLiteral("organizations") : tenantId.trimmed();
    m_clientId = clientId.trimmed();
    if (m_oauth)
        m_oauth->configure(m_tenantId, m_clientId);
}

void Office365Service::setIdentity(QString tenantId, QString clientId)
{
    configureIdentity(m_settingsOrganization, std::move(tenantId), std::move(clientId));
    persistIdentity();
}

void Office365Service::persistIdentity()
{
    if (m_settingsOrganization.isEmpty())
        return;
    QSettings settings(m_settingsOrganization, QStringLiteral("AppSettings"));
    settings.setFallbacksEnabled(false);
    settings.setValue(QStringLiteral("FileFinder/tenantId"), m_tenantId);
    settings.setValue(QStringLiteral("FileFinder/clientId"), m_clientId);
    settings.sync();

}

bool Office365Service::authenticated() const { return m_oauth && m_oauth->authenticated(); }
bool Office365Service::authenticationInProgress() const { return m_oauth && m_oauth->authenticationInProgress(); }
QString Office365Service::authenticationStatus() const
{
    return m_oauth ? m_oauth->status() : tr("Not signed in");
}
QString Office365Service::userCode() const { return m_oauth ? m_oauth->userCode() : QString(); }
QUrl Office365Service::verificationUrl() const { return m_oauth ? m_oauth->verificationUrl() : QUrl(); }
void Office365Service::restoreSession() { if (m_oauth) m_oauth->restoreSession(); }
void Office365Service::requestFileFinderConsent()
{
    if (m_oauth) {
        m_oauth->setRequestedScopes({QStringLiteral("offline_access"),
                                     QStringLiteral("https://graph.microsoft.com/Team.ReadBasic.All"),
                                     QStringLiteral("https://graph.microsoft.com/Channel.ReadBasic.All"),
                                     QStringLiteral("https://graph.microsoft.com/Files.Read.All")});
    }
}

void Office365Service::requestEmailDraftConsent()
{
    if (m_oauth) {
        m_oauth->setRequestedScopes({QStringLiteral("offline_access"),
                                     QStringLiteral("User.Read"),
                                     QStringLiteral("Mail.ReadWrite")});
    }
}

void Office365Service::startSignIn() { if (m_oauth) m_oauth->startSignIn(); }
void Office365Service::signOut() { if (m_oauth) m_oauth->signOut(); }

Office365Account Office365Service::account() const
{
    if (!m_oauth)
        return {};
    return {m_identityGeneration == m_oauth->sessionGeneration() ? m_identityLabel : QString(),
            m_oauth->grantedScopeList(), m_oauth->sessionGeneration(),
            m_oauth->authenticated() && !m_accessToken.isEmpty()};
}

void Office365Service::refreshIdentity()
{
    if (!m_oauth)
        return;
    const quint64 generation = m_oauth->sessionGeneration();
    HttpRequest request;
    request.operationId = QUuid::createUuid();
    request.method = "GET";
    request.url = QUrl(QStringLiteral("https://graph.microsoft.com/v1.0/me?$select=displayName,mail,userPrincipalName"));
    sendGraphRequest(std::move(request), [this, generation](HttpResponse response) {
        if (!m_oauth || generation != m_oauth->sessionGeneration())
            return;
        if (!response.error.code.isEmpty() || response.statusCode < 200 || response.statusCode >= 300) {
            emit diagnostic(response.error.displayText.isEmpty()
                                ? tr("Microsoft account identity lookup failed.")
                                : response.error.displayText);
            return;
        }
        const QJsonObject object = QJsonDocument::fromJson(response.body).object();
        const QString displayName = object.value(QStringLiteral("displayName")).toString().trimmed();
        const QString address = object.value(QStringLiteral("mail")).toString().trimmed().isEmpty()
            ? object.value(QStringLiteral("userPrincipalName")).toString().trimmed()
            : object.value(QStringLiteral("mail")).toString().trimmed();
        m_identityLabel = displayName.isEmpty() ? address
            : (address.isEmpty() ? displayName : displayName + QStringLiteral(" <") + address + QStringLiteral(">"));
        if (m_identityLabel.isEmpty()) {
            emit diagnostic(tr("Microsoft did not return an account name or address."));
            return;
        }
        m_identityGeneration = generation;
        emit accountChanged();
    });
}

void Office365Service::sendGraphRequest(HttpRequest request, HttpTransport::Completion completion)
{
    if (m_accessToken.isEmpty()) {
        HttpResponse response;
        response.operationId = request.operationId;
        response.error = {QStringLiteral("office365-not-authenticated"),
                          tr("Sign in to Microsoft 365 before continuing.")};
        completion(std::move(response));
        return;
    }
    if (!m_transport) {
        HttpResponse response;
        response.operationId = request.operationId;
        response.error = {QStringLiteral("office365-transport-unavailable"),
                          tr("Microsoft 365 transport is unavailable.")};
        completion(std::move(response));
        return;
    }
    request.headers.remove("Authorization");
    request.headers.insert("Authorization", "Bearer " + m_accessToken.toUtf8());
    m_transport->send(std::move(request), std::move(completion));
}

void Office365Service::sendUploadRequest(HttpRequest request, HttpTransport::Completion completion)
{
    if (m_accessToken.isEmpty() || !m_transport) {
        HttpResponse response;
        response.operationId = request.operationId;
        response.error = {m_accessToken.isEmpty() ? QStringLiteral("office365-not-authenticated")
                                                   : QStringLiteral("office365-transport-unavailable"),
                          tr("Microsoft 365 upload is unavailable.")};
        completion(std::move(response));
        return;
    }
    request.headers.remove("Authorization");
    m_transport->send(std::move(request), std::move(completion));
}

} // namespace PN::Comm
