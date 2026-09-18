// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "Office365SettingsModel.h"

#include "FileFinderService.h"
#include "ProjectNotesIntegrations/Office365Service.h"

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

bool Office365SettingsModel::emailDraftsGranted() const
{
    return m_service && m_service->emailDraftsGranted();
}

void Office365SettingsModel::startSignIn()
{
    // Keep this identical to the formerly separate draft-consent action: it
    // requests only the Graph capabilities needed to create a draft, never
    // Mail.Send. File Finder requests its own scopes when that feature is used.
    requestEmailDraftConsent();
}

void Office365SettingsModel::requestEmailDraftConsent()
{
    if (!m_service)
        return;
    m_service->requestEmailDraftConsent();
    m_service->startSignIn();
}

void Office365SettingsModel::signOut()
{
    if (m_fileFinder)
        m_fileFinder->signOutOffice365();
}
