// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "updatemanager.h"
#include "version.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QProgressDialog>
#include <QProcess>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>
#include <QDir>

#include "QLogger.h"

using namespace QLogger;

namespace
{
// GitHub repository that publishes ProjectNotes releases.
const char *kReleaseApiUrl =
    "https://api.github.com/repos/kestermckinney/ProjectNotes/releases/latest";

// GitHub rejects requests without a User-Agent header.
const char *kUserAgent = "ProjectNotes-Updater";
}

UpdateManager::UpdateManager(QWidget *parent)
    : QObject(parent)
    , m_parentWidget(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

QString UpdateManager::currentVersion()
{
    return QString("%1.%2.%3")
        .arg(APP_VERSION_MAJOR)
        .arg(APP_VERSION_MINOR)
        .arg(APP_VERSION_PATCH);
}

bool UpdateManager::isNewerVersion(const QString &candidate, const QString &current)
{
    const QStringList c = candidate.split('.');
    const QStringList r = current.split('.');

    const int parts = qMax(c.size(), r.size());
    for (int i = 0; i < parts; ++i)
    {
        const int cv = (i < c.size()) ? c.at(i).toInt() : 0;
        const int rv = (i < r.size()) ? r.at(i).toInt() : 0;

        if (cv != rv)
            return cv > rv;
    }

    return false; // equal
}

QString UpdateManager::selectPlatformAsset(const QJsonArray &assets)
{
    for (const QJsonValue &v : assets)
    {
        const QJsonObject asset = v.toObject();
        const QString name = asset.value("name").toString();

#if defined(Q_OS_WIN)
        if (name.endsWith("-Windows-x64-Setup.exe", Qt::CaseInsensitive))
            return asset.value("browser_download_url").toString();
#elif defined(Q_OS_MACOS)
        if (name.contains("macOS", Qt::CaseInsensitive) && name.endsWith(".dmg", Qt::CaseInsensitive))
            return asset.value("browser_download_url").toString();
#else
        Q_UNUSED(name);
#endif
    }

    return QString();
}

void UpdateManager::checkForUpdates(bool silent)
{
    m_silent = silent;

    if (m_checkReply)
        return; // a check is already in flight

    QNetworkRequest request{QUrl(QString::fromLatin1(kReleaseApiUrl))};
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    m_checkReply = m_network->get(request);
    connect(m_checkReply, &QNetworkReply::finished, this, &UpdateManager::onCheckFinished);
}

void UpdateManager::onCheckFinished()
{
    QNetworkReply *reply = m_checkReply;
    m_checkReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        const QString error = reply->errorString();
        QLog_Warning(CONSOLELOG, QString("Update check failed: %1").arg(error));
        emit checkFailed(error);
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject())
    {
        emit checkFailed(tr("Unexpected response from the update server."));
        return;
    }

    const QJsonObject release = doc.object();

    // Tags are published as "vX.Y.Z"; strip the leading "v".
    QString latest = release.value("tag_name").toString();
    if (latest.startsWith('v', Qt::CaseInsensitive))
        latest.remove(0, 1);

    if (latest.isEmpty())
    {
        emit checkFailed(tr("The update server did not report a version."));
        return;
    }

    if (!isNewerVersion(latest, currentVersion()))
    {
        QLog_Info(CONSOLELOG, QString("Update check: running %1, latest %2 — up to date.")
                                  .arg(currentVersion(), latest));
        emit upToDate();
        return;
    }

    const QString assetUrl = selectPlatformAsset(release.value("assets").toArray());
    if (assetUrl.isEmpty())
    {
        // A newer release exists but ships no installer for this platform.
        emit checkFailed(tr("Version %1 is available, but no installer was found for this platform. "
                            "Please download it manually.").arg(latest));
        return;
    }

    QLog_Info(CONSOLELOG, QString("Update available: %1 (current %2).").arg(latest, currentVersion()));
    emit updateAvailable(latest, release.value("body").toString(), QUrl(assetUrl));
}

void UpdateManager::downloadAndInstall(const QUrl &assetUrl)
{
    if (m_downloadReply)
        return; // already downloading

    const QString fileName = QFileInfo(assetUrl.path()).fileName();
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_downloadPath = QDir(tempDir).filePath(fileName);

    QNetworkRequest request{assetUrl};
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    m_progressDialog = new QProgressDialog(tr("Downloading update..."), tr("Cancel"), 0, 100, m_parentWidget);
    m_progressDialog->setWindowTitle(tr("Software Update"));
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setMinimumDuration(0);
    m_progressDialog->setAutoClose(false);
    m_progressDialog->setAutoReset(false);

    m_downloadReply = m_network->get(request);
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &UpdateManager::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished, this, &UpdateManager::onDownloadFinished);
    connect(m_progressDialog, &QProgressDialog::canceled, m_downloadReply, &QNetworkReply::abort);
}

void UpdateManager::onDownloadProgress(qint64 received, qint64 total)
{
    if (!m_progressDialog || total <= 0)
        return;

    m_progressDialog->setMaximum(static_cast<int>(total));
    m_progressDialog->setValue(static_cast<int>(received));
}

void UpdateManager::onDownloadFinished()
{
    QNetworkReply *reply = m_downloadReply;
    m_downloadReply = nullptr;

    const bool canceled = m_progressDialog && m_progressDialog->wasCanceled();

    if (m_progressDialog)
    {
        m_progressDialog->close();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }

    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError error = reply->error();
    const QString errorString = reply->errorString();
    reply->deleteLater();

    if (canceled)
        return; // user aborted — nothing to report

    if (error != QNetworkReply::NoError)
    {
        QLog_Error(ERRORLOG, QString("Update download failed: %1").arg(errorString));
        QMessageBox::warning(m_parentWidget, tr("Software Update"),
                             tr("The update could not be downloaded:\n%1").arg(errorString));
        return;
    }

    QFile out(m_downloadPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate) || out.write(payload) != payload.size())
    {
        QLog_Error(ERRORLOG, QString("Could not write update installer to %1").arg(m_downloadPath));
        QMessageBox::warning(m_parentWidget, tr("Software Update"),
                             tr("The update was downloaded but could not be saved to disk."));
        return;
    }
    out.close();

    launchInstaller();
}

void UpdateManager::launchInstaller()
{
#if defined(Q_OS_WIN)
    // Run the NSIS installer silently. It waits for this process (passed via
    // /waitpid) to exit before overwriting the locked executable, then relaunches
    // ProjectNotes (/relaunch). See packaging/windows/setupscript.nsi.
    const QStringList args{
        "/S",
        "/relaunch",
        QString("/waitpid=%1").arg(QCoreApplication::applicationPid())};

    if (!QProcess::startDetached(m_downloadPath, args))
    {
        QLog_Error(ERRORLOG, QString("Failed to launch update installer %1").arg(m_downloadPath));
        QMessageBox::warning(m_parentWidget, tr("Software Update"),
                             tr("The update was downloaded but the installer could not be started."));
        return;
    }

    QLog_Info(CONSOLELOG, "Update installer launched; shutting down for unattended install.");
    emit installerLaunched();
#elif defined(Q_OS_MACOS)
    // macOS installs are semi-attended: open the disk image and let the user
    // replace the app (the running app cannot overwrite itself).
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(m_downloadPath)))
    {
        QMessageBox::warning(m_parentWidget, tr("Software Update"),
                             tr("The update was downloaded but the disk image could not be opened.\n"
                                "It is located at:\n%1").arg(m_downloadPath));
        return;
    }

    QMessageBox::information(m_parentWidget, tr("Software Update"),
                            tr("Drag Project Notes into the Applications folder to finish updating, "
                               "then reopen the app."));
    emit installerLaunched();
#else
    // No unattended path on this platform; just reveal the download.
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_downloadPath).absolutePath()));
#endif
}
