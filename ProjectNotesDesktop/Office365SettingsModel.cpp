// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "Office365SettingsModel.h"

#include "FileFinderService.h"
#include "ProjectNotesIntegrations/Office365Service.h"
#include "ProjectNotesContacts/ContactSyncSettingsModel.h"

Office365SettingsModel::Office365SettingsModel(FileFinderService *fileFinder,
                                                PN::Comm::Office365Service *service,
                                                QObject *parent)
    : QObject(parent), m_fileFinder(fileFinder), m_service(service)
{
    if (m_fileFinder) {
        connect(m_fileFinder, &FileFinderService::settingsChanged, this,
                &Office365SettingsModel::changed);
        connect(m_fileFinder, &FileFinderService::authenticationChanged, this,
                &Office365SettingsModel::changed);
    }
    if (m_service)
        connect(m_service, &PN::Comm::Office365Service::accountChanged, this,
                &Office365SettingsModel::changed);
}

QString Office365SettingsModel::tenantId() const
{
    return m_service ? m_service->tenantId() : QStringLiteral("organizations");
}

void Office365SettingsModel::setTenantId(const QString &value)
{
    if (m_fileFinder)
        m_fileFinder->setOffice365TenantId(value);
}

QString Office365SettingsModel::clientId() const
{
    return m_service ? m_service->clientId() : QString();
}

void Office365SettingsModel::setClientId(const QString &value)
{
    if (m_fileFinder)
        m_fileFinder->setOffice365ClientId(value);
}

bool Office365SettingsModel::authenticated() const
{
    return m_service && m_service->authenticated();
}

bool Office365SettingsModel::authenticationInProgress() const
{
    return m_service && m_service->authenticationInProgress();
}

QString Office365SettingsModel::authenticationStatus() const
{
    return m_service ? m_service->authenticationStatus() : tr("Not initialized");
}

QString Office365SettingsModel::userCode() const
{
    return m_service ? m_service->userCode() : QString();
}

QUrl Office365SettingsModel::verificationUrl() const
{
    return m_service ? m_service->verificationUrl() : QUrl();
}

QString Office365SettingsModel::accountLabel() const
{
    return m_service ? m_service->account().label : QString();
}

bool Office365SettingsModel::teamsFileWrite() const
{
    return !m_service || m_service->teamsFileWriteEnabled();
}

void Office365SettingsModel::setTeamsFileWrite(bool enabled)
{
    if (m_service)
        m_service->setTeamsFileWriteEnabled(enabled);
}

bool Office365SettingsModel::fileWriteSignInFailed() const
{
    return m_service && m_service->fileWriteSignInFailed();
}

bool Office365SettingsModel::teamsFileWriteNotGranted() const
{
    if (!m_service || !m_service->authenticated() || !m_service->teamsFileWriteEnabled())
        return false;
    // Only when this account uses Teams files at all.
    bool teamsFiles = m_fileFinder && m_fileFinder->office365Enabled();
    for (const QString &scope : m_service->account().grantedScopes)
        teamsFiles = teamsFiles || scope.contains(QLatin1String("Files.Read"), Qt::CaseInsensitive);
    return teamsFiles && !m_service->teamsFilesWritable();
}

void Office365SettingsModel::startSignIn()
{
    // The single account sign-in also requests the enabled contact capability,
    // and Teams file access when File Finder searches Microsoft Teams (write
    // access when saving into Teams folders is on). Never Mail.Send.
    requestCommunicationsConsent();
}

void Office365SettingsModel::signInWithoutTeamsFileWrite()
{
    setTeamsFileWrite(false);
    requestCommunicationsConsent();
}

void Office365SettingsModel::requestCommunicationsConsent()
{
    if (!m_service)
        return;
    const auto config = m_contacts ? m_contacts->configuration() : QVariantMap{};
    const bool contactsEnabled = config.value("provider").toString() == "office365";
    const bool teamsFolders = m_fileFinder && m_fileFinder->office365Enabled();
    m_service->requestCommunicationsConsent(contactsEnabled, contactsEnabled, teamsFolders);
    m_service->startSignIn();
}

void Office365SettingsModel::signOut()
{
    if (m_fileFinder)
        m_fileFinder->signOutOffice365();
}
