// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "FileFinderService.h"
#include "FileFinderWorker.h"
#include "MicrosoftOAuthManager.h"
#include "../credentialstore.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QReadWriteLock>
#include <QSettings>
#include <QThread>
#include <QUrl>

namespace {
constexpr auto kSettingsApplication = "AppSettings";
constexpr auto kLegacyPluginSettings = "PluginSettings";
constexpr auto kSettingsPrefix = "FileFinder/";
constexpr auto kOffice365SettingsPrefix = "Office365/";
constexpr auto kCredentialService = "Office365FileFinder";

QStringList defaultSearchRoots()
{
    return {QStringLiteral("~")};
}

QStringList defaultFolderExclusions()
{
    return {QStringLiteral(R"(Engineering/.*)")};
}

QStringList normalizedRoots(QStringList roots)
{
    for (QString &root : roots) {
        const QString trimmed = root.trimmed();
        const QUrl url(trimmed);
        QString localPath = url.isLocalFile() ? url.toLocalFile() : trimmed;
        if (localPath.startsWith(QLatin1String("~\\")))
            localPath.replace(0, 2, QStringLiteral("~/"));
        localPath = QDir::fromNativeSeparators(localPath);
        const bool homeRelative = localPath == QLatin1String("~")
            || localPath.startsWith(QLatin1String("~/"));
        root = localPath.isEmpty() ? QString()
             : homeRelative ? QDir::cleanPath(localPath)
                            : QDir::fromNativeSeparators(QDir(localPath).absolutePath());
    }
    roots.removeAll(QString());
    roots.removeDuplicates();
    return roots;
}

QStringList expandedRoots(QStringList roots)
{
    for (QString &root : roots) {
        if (root == QLatin1String("~"))
            root = QDir::homePath();
        else if (root.startsWith(QLatin1String("~/")))
            root = QDir::homePath() + root.mid(1);
    }
    roots.removeDuplicates();
    return roots;
}

QHash<QString, QString> graphFolderStateFromJson(const QByteArray &json)
{
    QHash<QString, QString> state;
    const QJsonObject object = QJsonDocument::fromJson(json).object();
    for (auto it = object.constBegin(); it != object.constEnd(); ++it)
        state.insert(it.key(), it.value().toString());
    return state;
}

QByteArray graphFolderStateToJson(const QHash<QString, QString> &state)
{
    QJsonObject object;
    for (auto it = state.constBegin(); it != state.constEnd(); ++it)
        object.insert(it.key(), it.value());
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}
}

FileFinderService::FileFinderService(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<FileFinderConfiguration>();
    qRegisterMetaType<FileFinderScanSummary>();
    qRegisterMetaType<QHash<QString, QString>>();
    qRegisterMetaType<QReadWriteLock *>("QReadWriteLock*");

    m_oauth = new MicrosoftOAuthManager(this);
    m_oauth->setSecretStore(
        [](const QString &account, QString *error) {
            return CredentialStore::read(QString::fromLatin1(kCredentialService), account, error);
        },
        [](const QString &account, const QString &secret, QString *error) {
            return CredentialStore::write(QString::fromLatin1(kCredentialService), account, secret, error);
        },
        [](const QString &account, QString *error) {
            return CredentialStore::remove(QString::fromLatin1(kCredentialService), account, error);
        });
    connect(m_oauth, &MicrosoftOAuthManager::stateChanged,
            this, &FileFinderService::authenticationChanged);
    connect(m_oauth, &MicrosoftOAuthManager::diagnostic, this,
            [this](const QString &message) {
        m_status = message;
        emit statusChanged();
    });
    connect(m_oauth, &MicrosoftOAuthManager::accessTokenChanged, this,
            [this](const QString &token) {
        m_accessToken = token;
        applyConfiguration();
        if (!token.isEmpty() && m_enabled && m_office365Enabled)
            scanNow();
    });
}

FileFinderService::~FileFinderService()
{
    if (m_worker && m_thread && m_thread->isRunning()) {
        // scanNow() performs the traversal in the worker thread. Request
        // interruption directly so its inner loops can stop before the queued
        // shutdown slots are serviced.
        m_thread->requestInterruption();
        QMetaObject::invokeMethod(m_worker, &FileFinderWorker::stop,
                                  Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(m_worker, &FileFinderWorker::closeDatabase,
                                  Qt::BlockingQueuedConnection);
        m_thread->quit();
        m_thread->wait();
    }
}

void FileFinderService::initialize(const QString &databasePath, QReadWriteLock *databaseLock,
                                   const QString &settingsOrganization)
{
    if (m_initialized)
        return;
    m_settingsOrganization = settingsOrganization;
    loadAndMigrateSettings();
    m_oauth->configure(m_tenantId, m_clientId);

    m_thread = new QThread(this);
    m_thread->setObjectName(QStringLiteral("ProjectNotesFileFinderThread"));
    m_worker = new FileFinderWorker;
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &FileFinderWorker::diagnostic, this,
            [this](const QString &message) {
        emit diagnostic(message);
        m_status = message;
        emit statusChanged();
    });
    connect(m_worker, &FileFinderWorker::scanStarted, this, [this] {
        m_scanning = true;
        m_status = tr("Scanning active projects…");
        m_scanningLocation.clear();
        emit statusChanged();
        emit scanningLocationChanged();
    });
    connect(m_worker, &FileFinderWorker::scanningLocation, this,
            [this](const QString &description) {
        m_scanningLocation = description;
        emit scanningLocationChanged();
    });
    connect(m_worker, &FileFinderWorker::scanFinished, this,
            [this](const FileFinderScanSummary &summary) {
        m_scanning = false;
        m_scanningLocation.clear();
        emit scanningLocationChanged();
        if (summary.error.isEmpty()) {
            m_status = summary.warning.isEmpty() ? tr("File Finder is ready")
                                                 : summary.warning;
            m_lastScanSummary = tr("%1 active projects · %2 files · %3 matches · "
                                   "%4 inserted · %5 updated · %6 unchanged · %7 ms")
                .arg(summary.projects).arg(summary.files).arg(summary.matched)
                .arg(summary.inserted).arg(summary.updated).arg(summary.unchanged)
                .arg(summary.elapsedMs);
        } else {
            m_status = summary.error;
            m_lastScanSummary = tr("Last scan failed after %1 ms").arg(summary.elapsedMs);
        }
        emit statusChanged();
    });
    connect(m_worker, &FileFinderWorker::locationsCommitted,
            this, &FileFinderService::locationsChanged);
    connect(m_worker, &FileFinderWorker::graphFolderStateChanged, this,
            [this](const QHash<QString, QString> &state) {
        if (m_graphFolderState == state)
            return;
        m_graphFolderState = state;
        saveSettings();
    });
    m_thread->start(QThread::LowPriority);
    QMetaObject::invokeMethod(m_worker, "initializeDatabase", Qt::QueuedConnection,
                              Q_ARG(QString, databasePath),
                              Q_ARG(QReadWriteLock*, databaseLock));
    m_initialized = true;
    applyConfiguration();
    QMetaObject::invokeMethod(m_worker, &FileFinderWorker::start, Qt::QueuedConnection);
    if (m_office365Enabled)
        m_oauth->restoreSession();
    m_status = m_enabled ? tr("File Finder is ready") : tr("File Finder is disabled");
    emit settingsChanged();
    emit statusChanged();
}

void FileFinderService::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    saveSettings();
    applyConfiguration();
    m_status = enabled ? tr("File Finder is ready") : tr("File Finder is disabled");
    emit settingsChanged();
    emit statusChanged();
}

QVariantList FileFinderService::fileRules() const
{
    QVariantList result;
    for (const FileFinderRule &rule : m_rules)
        result.append(QVariantMap{{QStringLiteral("classification"), rule.classification},
                                  {QStringLiteral("pattern"), rule.pattern}});
    return result;
}

void FileFinderService::setOffice365Enabled(bool enabled)
{
    if (m_office365Enabled == enabled)
        return;
    m_office365Enabled = enabled;
    saveSettings();
    applyConfiguration();
    if (enabled)
        m_oauth->restoreSession();
    emit settingsChanged();
}

void FileFinderService::setOffice365OpenLinksInDesktop(bool enabled)
{
    if (m_office365OpenLinksInDesktop == enabled)
        return;
    m_office365OpenLinksInDesktop = enabled;
    saveSettings();
    emit settingsChanged();
}

void FileFinderService::setOffice365TenantId(const QString &tenantId)
{
    const QString value = tenantId.trimmed().isEmpty()
        ? QStringLiteral("organizations") : tenantId.trimmed();
    if (m_tenantId == value)
        return;
    m_tenantId = value;
    invalidateGraphFolderState();
    saveSettings();
    m_oauth->configure(m_tenantId, m_clientId);
    emit settingsChanged();
}

void FileFinderService::setOffice365ClientId(const QString &clientId)
{
    const QString value = clientId.trimmed();
    if (m_clientId == value)
        return;
    m_clientId = value;
    invalidateGraphFolderState();
    saveSettings();
    m_oauth->configure(m_tenantId, m_clientId);
    emit settingsChanged();
}

bool FileFinderService::office365Authenticated() const
{
    return m_oauth && m_oauth->authenticated();
}

bool FileFinderService::office365AuthenticationInProgress() const
{
    return m_oauth && m_oauth->authenticationInProgress();
}

QString FileFinderService::office365AuthenticationStatus() const
{
    return m_oauth ? m_oauth->status() : tr("Not signed in");
}

QString FileFinderService::office365UserCode() const
{
    return m_oauth ? m_oauth->userCode() : QString();
}

QUrl FileFinderService::office365VerificationUrl() const
{
    return m_oauth ? m_oauth->verificationUrl() : QUrl();
}

void FileFinderService::addSearchRoot(const QString &path)
{
    const QString value = normalizedRoots({path}).value(0);
    if (value.isEmpty() || m_roots.contains(value))
        return;
    m_roots.append(value);
    saveSettings();
    applyConfiguration();
    emit settingsChanged();
}

void FileFinderService::removeSearchRoot(int index)
{
    if (index < 0 || index >= m_roots.size())
        return;
    m_roots.removeAt(index);
    saveSettings();
    applyConfiguration();
    emit settingsChanged();
}

void FileFinderService::addFolderExclusion(const QString &pattern)
{
    const QString value = pattern.trimmed();
    if (value.isEmpty() || m_folderExclusions.contains(value))
        return;
    m_folderExclusions.append(value);
    invalidateGraphFolderState();
    saveSettings();
    applyConfiguration();
    emit settingsChanged();
}

void FileFinderService::updateFolderExclusion(int index, const QString &pattern)
{
    const QString value = pattern.trimmed();
    if (index < 0 || index >= m_folderExclusions.size() || value.isEmpty())
        return;
    m_folderExclusions[index] = value;
    m_folderExclusions.removeDuplicates();
    invalidateGraphFolderState();
    saveSettings();
    applyConfiguration();
    emit settingsChanged();
}

void FileFinderService::removeFolderExclusion(int index)
{
    if (index < 0 || index >= m_folderExclusions.size())
        return;
    m_folderExclusions.removeAt(index);
    invalidateGraphFolderState();
    saveSettings();
    applyConfiguration();
    emit settingsChanged();
}

void FileFinderService::addFileRule(const QString &classification, const QString &pattern)
{
    if (pattern.trimmed().isEmpty())
        return;
    m_rules.append({classification.trimmed(), pattern.trimmed()});
    invalidateGraphFolderState();
    saveSettings();
    applyConfiguration();
    emit settingsChanged();
}

void FileFinderService::updateFileRule(int index, const QString &classification,
                                       const QString &pattern)
{
    if (index < 0 || index >= m_rules.size() || pattern.trimmed().isEmpty())
        return;
    m_rules[index] = {classification.trimmed(), pattern.trimmed()};
    invalidateGraphFolderState();
    saveSettings();
    applyConfiguration();
    emit settingsChanged();
}

void FileFinderService::removeFileRule(int index)
{
    if (index < 0 || index >= m_rules.size())
        return;
    m_rules.removeAt(index);
    invalidateGraphFolderState();
    saveSettings();
    applyConfiguration();
    emit settingsChanged();
}

void FileFinderService::resetDefaultRules()
{
    m_rules = defaultRules();
    invalidateGraphFolderState();
    saveSettings();
    applyConfiguration();
    emit settingsChanged();
}

void FileFinderService::scanNow()
{
    if (m_worker && m_enabled)
        QMetaObject::invokeMethod(m_worker, &FileFinderWorker::scanNow, Qt::QueuedConnection);
}

void FileFinderService::reconsiderAllFiles()
{
    if (!m_worker || !m_enabled)
        return;
    m_lastScanSummary.clear();
    m_status = tr("Resetting File Finder state and reconsidering all files…");
    invalidateGraphFolderState();
    saveSettings();
    applyConfiguration();
    emit statusChanged();
    // The empty Graph folder-state cache forces every remote subtree to be read.
    QMetaObject::invokeMethod(m_worker, &FileFinderWorker::scanNow, Qt::QueuedConnection);
}

void FileFinderService::startOffice365SignIn()
{
    if (!m_office365Enabled)
        setOffice365Enabled(true);
    m_oauth->startSignIn();
}

void FileFinderService::signOutOffice365()
{
    invalidateGraphFolderState();
    saveSettings();
    applyConfiguration();
    m_oauth->signOut();
}

void FileFinderService::invalidateGraphFolderState()
{
    m_graphFolderState.clear();
}

void FileFinderService::loadAndMigrateSettings()
{
    QSettings settings(m_settingsOrganization, QString::fromLatin1(kSettingsApplication));
    const QString prefix = QString::fromLatin1(kSettingsPrefix);
    QSettings legacy(m_settingsOrganization, QString::fromLatin1(kLegacyPluginSettings));
    legacy.setFallbacksEnabled(false);

    if (!settings.value(prefix + QStringLiteral("migrationComplete"), false).toBool()) {
        QStringList roots;
        const QJsonDocument rootDocument = QJsonDocument::fromJson(
            legacy.value(QStringLiteral("File Finder/SearchLocations")).toByteArray());
        for (const QJsonValue &value : rootDocument.array())
            roots.append(value.toObject().value(QStringLiteral("Location")).toString());

        QList<FileFinderRule> rules;
        const QJsonDocument ruleDocument = QJsonDocument::fromJson(
            legacy.value(QStringLiteral("File Finder/Classifications")).toByteArray());
        for (const QJsonValue &value : ruleDocument.array()) {
            const QJsonObject object = value.toObject();
            rules.append({object.value(QStringLiteral("Classification")).toString(),
                          object.value(QStringLiteral("Pattern Match")).toString()});
        }
        const bool hasLegacySearchSettings = !roots.isEmpty() || !rules.isEmpty();
        if (roots.isEmpty())
            roots = defaultSearchRoots();
        if (rules.isEmpty())
            rules = defaultRules();
        settings.setValue(prefix + QStringLiteral("roots"), normalizedRoots(roots));
        if (!hasLegacySearchSettings) {
            settings.setValue(prefix + QStringLiteral("folderExclusions"),
                              defaultFolderExclusions());
        }
        QJsonArray array;
        for (const FileFinderRule &rule : rules)
            array.append(QJsonObject{{QStringLiteral("classification"), rule.classification},
                                     {QStringLiteral("pattern"), rule.pattern}});
        settings.setValue(prefix + QStringLiteral("rules"),
                          QJsonDocument(array).toJson(QJsonDocument::Compact));
        settings.setValue(prefix + QStringLiteral("enabled"), true);
        // Deliberately do NOT copy Outlook Integration's TenantID/ApplicationID
        // here. File Finder requests Team.ReadBasic.All, Channel.ReadBasic.All,
        // and Files.Read.All, which Microsoft Graph marks as requiring admin
        // consent; Outlook Integration's app registration was only ever
        // consented for Mail/Calendar/Contacts/Tasks scopes. Reusing it makes
        // sign-in appear to succeed while Azure AD then rejects the new scopes
        // with AADSTS90094. File Finder needs its own Entra app registration,
        // entered by the user under Settings > Office 365 Integration.
        settings.setValue(prefix + QStringLiteral("migrationComplete"), true);
    }

    const QString office365Prefix = QString::fromLatin1(kOffice365SettingsPrefix);
    m_enabled = settings.value(prefix + QStringLiteral("enabled"), false).toBool();
    m_office365Enabled = settings.value(prefix + QStringLiteral("office365Enabled"), false).toBool();
    m_tenantId = settings.value(office365Prefix + QStringLiteral("tenantId"),
                                QStringLiteral("organizations")).toString();
    m_clientId = settings.value(office365Prefix + QStringLiteral("clientId")).toString();
    m_office365OpenLinksInDesktop = settings.value(
        office365Prefix + QStringLiteral("openLinksInDesktop"), true).toBool();
    m_roots = normalizedRoots(settings.value(prefix + QStringLiteral("roots")).toStringList());
    m_folderExclusions = settings.value(
        prefix + QStringLiteral("folderExclusions")).toStringList();
    for (QString &pattern : m_folderExclusions)
        pattern = pattern.trimmed();
    m_folderExclusions.removeAll(QString());
    m_folderExclusions.removeDuplicates();
    m_graphFolderState = graphFolderStateFromJson(
        settings.value(prefix + QStringLiteral("graphFolderState")).toByteArray());
    const QJsonDocument rules = QJsonDocument::fromJson(
        settings.value(prefix + QStringLiteral("rules")).toByteArray());
    m_rules.clear();
    for (const QJsonValue &value : rules.array()) {
        const QJsonObject object = value.toObject();
        m_rules.append({object.value(QStringLiteral("classification")).toString(),
                        object.value(QStringLiteral("pattern")).toString()});
    }
}

void FileFinderService::saveSettings() const
{
    if (m_settingsOrganization.isEmpty())
        return;
    QSettings settings(m_settingsOrganization, QString::fromLatin1(kSettingsApplication));
    const QString prefix = QString::fromLatin1(kSettingsPrefix);
    const QString office365Prefix = QString::fromLatin1(kOffice365SettingsPrefix);
    settings.setValue(prefix + QStringLiteral("enabled"), m_enabled);
    settings.setValue(prefix + QStringLiteral("office365Enabled"), m_office365Enabled);
    settings.setValue(office365Prefix + QStringLiteral("tenantId"), m_tenantId);
    settings.setValue(office365Prefix + QStringLiteral("clientId"), m_clientId);
    settings.setValue(office365Prefix + QStringLiteral("openLinksInDesktop"),
                      m_office365OpenLinksInDesktop);
    settings.remove(prefix + QStringLiteral("tenantId"));
    settings.remove(prefix + QStringLiteral("clientId"));
    settings.setValue(prefix + QStringLiteral("roots"), m_roots);
    settings.setValue(prefix + QStringLiteral("folderExclusions"), m_folderExclusions);
    settings.setValue(prefix + QStringLiteral("graphFolderState"),
                      graphFolderStateToJson(m_graphFolderState));
    QJsonArray array;
    for (const FileFinderRule &rule : m_rules)
        array.append(QJsonObject{{QStringLiteral("classification"), rule.classification},
                                 {QStringLiteral("pattern"), rule.pattern}});
    settings.setValue(prefix + QStringLiteral("rules"),
                      QJsonDocument(array).toJson(QJsonDocument::Compact));
}

void FileFinderService::applyConfiguration()
{
    if (!m_worker)
        return;
    FileFinderConfiguration configuration;
    configuration.roots = expandedRoots(m_roots);
    configuration.folderExclusions = m_folderExclusions;
    configuration.graphFolderState = m_graphFolderState;
    configuration.rules = m_rules;
    configuration.enabled = m_enabled;
    configuration.office365Enabled = m_office365Enabled;
    configuration.accessToken = m_accessToken;
    QMetaObject::invokeMethod(m_worker, "configure", Qt::QueuedConnection,
                              Q_ARG(FileFinderConfiguration, configuration));
}

QList<FileFinderRule> FileFinderService::defaultRules()
{
    return {
        {"Project Schedule", R"(.*\.mpp$)"},
        {"Quote", R"(.*Quote.*\.pdf$)"},
        {"Issues List", R"(^(?!.*\bTemplate\b).*Tracker Report.*\.pdf$)"},
        {"Issues List", R"(^(?!.*\bTemplate\b).*Issues List.*\.xlsx$)"},
        {"Meeting Presentation", R"(^(?!.*\bTemplate\b).*Meeting Minutes.*\.pptx$)"},
        {"Meeting Presentation", R"(^(?!.*\bTemplate\b).*Meeting Minutes.*\.ppt$)"},
        {"Meeting Notes", R"(^(?!.*\bTemplate\b).*Meeting Minutes.*\.doc$)"},
        {"Meeting Notes", R"(^(?!.*\bTemplate\b).*Meeting Minutes.*\.docx$)"},
        {"Change Request", R"(.*PCR\d{1}.*\.pdf$)"},
        {"Change Request", R"(.*PCR\d{1}.*\.docx$)"},
        {"Change Request", R"(.*PCR\d{1}.*\.xlsx$)"},
        {"PM Plan", R"(.*PM Plan.*\.docx$)"},
        {"Purchase Order", R"(.*/Purchase Orders/.*\.pdf$)"},
        {"Estimate", R"(.*Estimate.*\.xlsx$)"},
        {"Quote", R"(.*Quote.*\.docx$)"},
        {"Risk Register", R"(^(?!.*\bTemplate\b).*Risk.*\.xlsx$)"},
        {"Risk Register", R"(^(?!.*\bTemplate\b).*Risk Management.*\.docx$)"},
        {"Quote", R"(.*Proposal.*\.docx$)"},
        {"Quote", R"(.*Proposal.*\.pdf$)"},
        {"Stakeholders", R"(.*Stakeholder.*\.docx$)"},
        {"Stakeholders", R"(.*Stakeholder.*\.xlsx$)"}
    };
}
