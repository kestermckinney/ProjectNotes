// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "Office365Service.h"
#include "MicrosoftOAuthManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

namespace PN::Comm {

namespace {

const QString kGraphPrefix = QStringLiteral("https://graph.microsoft.com/");
const QString kFilesRead = QStringLiteral("https://graph.microsoft.com/Files.Read.All");
const QString kFilesReadWrite = QStringLiteral("https://graph.microsoft.com/Files.ReadWrite.All");
const QString kTeamsFileWriteKey = QStringLiteral("Office365/teamsFileWrite");

QString bareScope(QString scope)
{
    scope = scope.trimmed();
    if (scope.startsWith(kGraphPrefix, Qt::CaseInsensitive))
        scope = scope.mid(kGraphPrefix.size());
    return scope;
}

} // namespace

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
                    if (!token.isEmpty())
                        m_fileWriteSignInFailed = false;
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
        connect(m_oauth, &MicrosoftOAuthManager::interactiveSignInFailed, this, [this] {
            bool askedForWrite = false;
            for (const QString &scope : m_oauth->requestedScopeList())
                askedForWrite = askedForWrite || isFileWriteScope(scope);
            if (m_fileWriteSignInFailed != askedForWrite) {
                m_fileWriteSignInFailed = askedForWrite;
                emit accountChanged();
            }
        });
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
    if (!m_settingsOrganization.isEmpty()) {
        QSettings settings(m_settingsOrganization, QStringLiteral("AppSettings"));
        settings.setFallbacksEnabled(false);
        m_teamsFileWrite = settings.value(kTeamsFileWriteKey, true).toBool();
    }
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
    // The dedicated Office 365 app registration lives under Office365/. The old
    // FileFinder/ copies are dropped so they cannot be read back as stale values.
    settings.setValue(QStringLiteral("Office365/tenantId"), m_tenantId);
    settings.setValue(QStringLiteral("Office365/clientId"), m_clientId);
    settings.remove(QStringLiteral("FileFinder/tenantId"));
    settings.remove(QStringLiteral("FileFinder/clientId"));
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
bool Office365Service::isFileWriteScope(const QString &scope)
{
    return bareScope(scope).compare(QLatin1String("Files.ReadWrite.All"), Qt::CaseInsensitive) == 0;
}

void Office365Service::setTeamsFileWriteEnabled(bool enabled)
{
    if (m_teamsFileWrite == enabled)
        return;
    m_teamsFileWrite = enabled;
    if (!m_settingsOrganization.isEmpty()) {
        QSettings settings(m_settingsOrganization, QStringLiteral("AppSettings"));
        settings.setFallbacksEnabled(false);
        settings.setValue(kTeamsFileWriteKey, enabled);
        settings.sync();
    }
    // A refused upload may have queued the write scope for the next sign-in.
    m_additionalScopes = applyTeamsFilePolicy(m_additionalScopes);
    // The requested scopes are left alone: changing them would mark a working
    // session as needing new consent. startSignIn() applies the setting.
    emit accountChanged();
}

QString Office365Service::teamsFileScope() const
{
    return m_teamsFileWrite ? kFilesReadWrite : kFilesRead;
}

bool Office365Service::teamsFilesWritable() const
{
    if (!m_teamsFileWrite)
        return false;
    const QStringList granted = m_oauth ? m_oauth->grantedScopeList() : QStringList{};
    if (granted.isEmpty())
        return true;
    for (const QString &scope : granted)
        if (isFileWriteScope(scope))
            return true;
    return false;
}

QStringList Office365Service::applyTeamsFilePolicy(QStringList scopes) const
{
    if (m_teamsFileWrite)
        return scopes;
    for (QString &scope : scopes)
        if (isFileWriteScope(scope))
            scope = kFilesRead;
    scopes.removeDuplicates();
    return scopes;
}

void Office365Service::requestFileFinderConsent()
{
    if (m_oauth) {
        m_oauth->setRequestedScopes({QStringLiteral("offline_access"),
                                     QStringLiteral("https://graph.microsoft.com/Team.ReadBasic.All"),
                                     QStringLiteral("https://graph.microsoft.com/Channel.ReadBasic.All"),
                                     teamsFileScope()});
    }
}

void Office365Service::requestCommunicationsConsent(bool includeContacts, bool exportContacts,
                                                    bool includeFileWrite)
{
    if (m_oauth) {
        m_oauth->setRequestedScopes({QStringLiteral("offline_access"),
                                     QStringLiteral("User.Read"),
                                     QStringLiteral("Mail.ReadWrite"),
                                     // Scheduled meetings are created as draft
                                     // events in the signed-in calendar.
                                     QStringLiteral("Calendars.ReadWrite")});
        auto scopes = m_oauth->requestedScopeList();
        if (includeContacts)
            scopes.append(exportContacts ? QStringLiteral("Contacts.ReadWrite") : QStringLiteral("Contacts.Read"));
        // Teams project folders need the Teams file scope (Files.ReadWrite.All,
        // or Files.Read.All with saving turned off), and a refresh never adds
        // it, so this sign-in is where it is consented. Ask when the caller
        // says Teams folders are in use (File Finder searches Teams), or when
        // the saved grant already reaches Teams or SharePoint files. The
        // grant alone is not enough: Microsoft can revoke it, and then
        // nothing would ask for the scope again.
        if (includeFileWrite)
            scopes.append(teamsFileScope());
        static const QStringList teamsFileScopes{
            QStringLiteral("Files.Read.All"), QStringLiteral("Files.ReadWrite.All"),
            QStringLiteral("Team.ReadBasic.All"), QStringLiteral("Channel.ReadBasic.All")};
        for (const QString &granted : m_oauth->grantedScopeList()) {
            if (teamsFileScopes.contains(bareScope(granted), Qt::CaseInsensitive)) {
                scopes.append(teamsFileScope());
                break;
            }
        }
        // A feature that was refused (a Teams upload needing
        // Files.ReadWrite.All, a plugin's scopes) tells the user to sign in
        // again here; without these that sign-in would never ask for them.
        scopes.append(m_additionalScopes);
        m_oauth->setRequestedScopes(applyTeamsFilePolicy(scopes));
    }
}

void Office365Service::requestContactConsent(bool exportEnabled)
{
    if (m_oauth) {
        QStringList scopes = m_oauth->grantedScopeList();
        scopes.append({QStringLiteral("offline_access"), QStringLiteral("User.Read"),
                       exportEnabled ? QStringLiteral("Contacts.ReadWrite") : QStringLiteral("Contacts.Read")});
        scopes.removeDuplicates();
        m_oauth->setRequestedScopes(applyTeamsFilePolicy(scopes));
    }
}

void Office365Service::requestAdditionalScopes(const QStringList &extra)
{
    if (!m_oauth)
        return;
    QStringList scopes = m_oauth->grantedScopeList();
    scopes.append({QStringLiteral("offline_access"), QStringLiteral("User.Read")});
    const QStringList allowed = applyTeamsFilePolicy(extra);
    scopes.append(allowed);
    scopes.removeDuplicates();
    m_additionalScopes.append(allowed);
    m_additionalScopes.removeDuplicates();
    m_oauth->setRequestedScopes(applyTeamsFilePolicy(scopes));
}

void Office365Service::startSignIn()
{
    if (!m_oauth)
        return;
    // Whatever set the scopes (including the manager's File Finder default),
    // a sign-in never asks for Files.ReadWrite.All with saving turned off.
    const QStringList requested = m_oauth->requestedScopeList();
    const QStringList allowed = applyTeamsFilePolicy(requested);
    if (allowed != requested)
        m_oauth->setRequestedScopes(allowed);
    if (m_fileWriteSignInFailed) {
        m_fileWriteSignInFailed = false;
        emit accountChanged();
    }
    m_oauth->startSignIn();
}
void Office365Service::signOut()
{
    m_additionalScopes.clear();
    if (m_oauth)
        m_oauth->signOut();
}

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

void Office365Service::cancelGraphRequest(const QUuid &operationId)
{
    if (m_transport)
        m_transport->cancel(operationId);
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
