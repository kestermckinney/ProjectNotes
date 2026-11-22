// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "updatemanager.h"
#include "version.h"
#include "appidentity.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
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
// Where ProjectNotes releases are published. The repository is moving from
// GitLab to GitHub; both are checked (see UpdateManager in updatemanager.h).
const char *kGitHubReleaseApiUrl =
    "https://api.github.com/repos/kestermckinney/ProjectNotes/releases/latest";
const char *kGitLabReleaseApiUrl =
    "https://gitlab.com/api/v4/projects/kestermckinney%2FProjectNotes/releases/permalink/latest";

// Tags are published as "vX.Y.Z". Anything else (a prerelease suffix, an
// empty tag) is not offered.
QString stableVersionFromTag(QString tag)
{
    if (tag.startsWith('v', Qt::CaseInsensitive))
        tag.remove(0, 1);
    static const QRegularExpression stableVersion(QStringLiteral("^[0-9]+\\.[0-9]+\\.[0-9]+$"));
    return stableVersion.match(tag).hasMatch() ? tag : QString();
}

// Identify the application to the release service.
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

QString UpdateManager::selectInstallerLink(const QJsonArray &assets,
                                         const QString &nameFragment,
                                         const QString &nameSuffix)
{
    for (const QJsonValue &v : assets)
    {
        const QJsonObject asset = v.toObject();
        const QString name = asset.value("name").toString();
        // Release link names may be descriptive labels rather than filenames.
        // GitLab's direct asset URL still carries the actual installer name.
        QString url = asset.value("direct_asset_url").toString();
        if (url.isEmpty())
            url = asset.value("url").toString();
        const QUrl downloadUrl(url);
        if (!downloadUrl.isValid() || downloadUrl.scheme() != "https"
            || downloadUrl.host().isEmpty())
            continue;

        const QString fileName = downloadUrl.fileName(QUrl::FullyDecoded);
        const auto matches = [&](const QString &candidate) {
            return candidate.contains(nameFragment, Qt::CaseInsensitive)
                   && candidate.endsWith(nameSuffix, Qt::CaseInsensitive);
        };
        if (matches(name) || matches(fileName))
            return url;
    }

    return QString();
}

QString UpdateManager::selectPlatformAsset(const QJsonArray &assets)
{
#if defined(Q_OS_WIN)
    return selectInstallerLink(assets, QStringLiteral("ProjectNotes-"),
                               QStringLiteral("-Windows-x64-Setup.exe"));
#elif defined(Q_OS_MACOS)
    // Only the self-contained .app zip supports unattended installation.
    return selectInstallerLink(assets, QStringLiteral("macOS"), QStringLiteral(".zip"));
#else
    // Linux updates are delivered as Flatpak bundles or through its remote.
    Q_UNUSED(assets);
    return QString();
#endif
}

void UpdateManager::checkForUpdates(bool silent)
{
    m_silent = silent;

    if (m_githubReply || m_gitlabReply)
        return; // a check is already in flight

    m_githubRelease = ReleaseCandidate{ReleaseSource::GitHub};
    m_gitlabRelease = ReleaseCandidate{ReleaseSource::GitLab};
    m_githubReply = startReleaseRequest(kGitHubReleaseApiUrl, true);
    m_gitlabReply = startReleaseRequest(kGitLabReleaseApiUrl, false);
}

QNetworkReply *UpdateManager::startReleaseRequest(const char *url, bool github)
{
    QNetworkRequest request{QUrl(QString::fromLatin1(url))};
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    if (github)
    {
        request.setRawHeader("Accept", "application/vnd.github+json");
        request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    }
    else
    {
        request.setRawHeader("Accept", "application/json");
    }
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_network->get(request);
    const ReleaseSource source = github ? ReleaseSource::GitHub : ReleaseSource::GitLab;
    connect(reply, &QNetworkReply::finished, this, [this, reply, source] { onReleaseReplyFinished(reply, source); });
    return reply;
}

void UpdateManager::onReleaseReplyFinished(QNetworkReply *reply, ReleaseSource source)
{
    reply->deleteLater();
    const bool github = source == ReleaseSource::GitHub;
    ReleaseCandidate result;
    if (reply->error() != QNetworkReply::NoError)
    {
        result.source = source;
        result.error = reply->errorString();
    }
    else
    {
        const QByteArray body = reply->readAll();
        result = github ? parseGitHubRelease(body) : parseGitLabRelease(body);
    }
    if (!result.error.isEmpty())
        QLog_Info(CONSOLELOG, QString("Update check: %1 gave no release: %2")
                                  .arg(github ? "GitHub" : "GitLab", result.error));

    if (github)
    {
        m_githubReply = nullptr;
        m_githubRelease = result;
    }
    else
    {
        m_gitlabReply = nullptr;
        m_gitlabRelease = result;
    }

    if (!m_githubReply && !m_gitlabReply)
        finishCheck();
}

UpdateManager::ReleaseCandidate UpdateManager::parseGitHubRelease(const QByteArray &body)
{
    ReleaseCandidate result{ReleaseSource::GitHub};
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject())
    {
        result.error = tr("Unexpected response from the update server.");
        return result;
    }
    const QJsonObject release = doc.object();
    result.answered = true;
    // releases/latest never returns these, but a record that is one is not an update.
    if (release.value("draft").toBool() || release.value("prerelease").toBool())
    {
        result.skipped = true;
        return result;
    }
    result.version = stableVersionFromTag(release.value("tag_name").toString());
    if (result.version.isEmpty())
    {
        result.error = tr("The update server did not report a version.");
        return result;
    }
    result.notes = release.value("body").toString();
    for (const QJsonValue &v : release.value("assets").toArray())
    {
        const QJsonObject asset = v.toObject();
        result.links.append(QJsonObject{{"name", asset.value("name").toString()},
                                        {"url", asset.value("browser_download_url").toString()}});
    }
    return result;
}

UpdateManager::ReleaseCandidate UpdateManager::parseGitLabRelease(const QByteArray &body)
{
    ReleaseCandidate result{ReleaseSource::GitLab};
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject())
    {
        result.error = tr("Unexpected response from the update server.");
        return result;
    }
    const QJsonObject release = doc.object();
    result.answered = true;
    // Upcoming releases are preparation records, not installable updates.
    if (release.value("upcoming_release").toBool())
    {
        result.skipped = true;
        return result;
    }
    result.version = stableVersionFromTag(release.value("tag_name").toString());
    if (result.version.isEmpty())
    {
        result.error = tr("The update server did not report a version.");
        return result;
    }
    result.notes = release.value("description").toString();
    result.links = release.value("assets").toObject().value("links").toArray();
    return result;
}

const UpdateManager::ReleaseCandidate *UpdateManager::chooseRelease(const ReleaseCandidate &github,
                                                                    const ReleaseCandidate &gitlab)
{
    const bool hasGitHub = !github.version.isEmpty();
    const bool hasGitLab = !gitlab.version.isEmpty();
    if (!hasGitHub || !hasGitLab)
        return hasGitHub ? &github : (hasGitLab ? &gitlab : nullptr);

    if (isNewerVersion(gitlab.version, github.version))
        return &gitlab;
    if (isNewerVersion(github.version, gitlab.version))
        return &github;
    // Same version on both: the one this platform can install, then GitHub.
    if (selectPlatformAsset(github.links).isEmpty() && !selectPlatformAsset(gitlab.links).isEmpty())
        return &gitlab;
    return &github;
}

void UpdateManager::finishCheck()
{
    if (const ReleaseCandidate *release = chooseRelease(m_githubRelease, m_gitlabRelease))
    {
        processRelease(*release);
        return;
    }

    // Neither host reported a version. A host that answered with only an
    // upcoming or draft record means there is nothing to install yet.
    if (m_githubRelease.skipped || m_gitlabRelease.skipped)
    {
        emit upToDate();
        return;
    }

    // Both failed. Until releases are published on GitHub, its usual reason is
    // "not found", so GitLab's reason is the more useful one to show.
    const QString error = !m_gitlabRelease.error.isEmpty() ? m_gitlabRelease.error : m_githubRelease.error;
    QLog_Warning(CONSOLELOG, QString("Update check failed: %1").arg(error));
    emit checkFailed(error.isEmpty() ? tr("Unexpected response from the update server.") : error);
}

void UpdateManager::processRelease(const ReleaseCandidate &release)
{
    const QString &latest = release.version;
    const QString host = release.source == ReleaseSource::GitHub ? QStringLiteral("GitHub") : QStringLiteral("GitLab");

    if (!isNewerVersion(latest, currentVersion()))
    {
        QLog_Info(CONSOLELOG, QString("Update check: running %1, latest %2 (%3) — up to date.")
                                  .arg(currentVersion(), latest, host));
        emit upToDate();
        return;
    }

#if defined(Q_OS_LINUX)
    // The host's Flatpak software manager owns sandboxed app updates.
    emit checkFailed(tr("Version %1 is available. Update Project Notes through your software "
                        "manager or run flatpak update com.projectnotespro.ProjectNotes.").arg(latest));
    return;
#endif

    const QString assetUrl = selectPlatformAsset(release.links);
    if (assetUrl.isEmpty())
    {
        // A newer release exists but ships no installer for this platform.
        emit checkFailed(tr("Version %1 is available, but no installer was found for this platform. "
                            "Please download it manually.").arg(latest));
        return;
    }

    QLog_Info(CONSOLELOG, QString("Update available: %1 from %2 (current %3).").arg(latest, host, currentVersion()));
    emit updateAvailable(latest, release.notes, QUrl(assetUrl));
}

void UpdateManager::downloadAndInstall(const QUrl &assetUrl)
{
    if (m_downloadReply)
        return; // already downloading

    m_downloadCanceled = false;

    const QString fileName = QFileInfo(assetUrl.path()).fileName();
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_downloadPath = QDir(tempDir).filePath(fileName);

    QNetworkRequest request{assetUrl};
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    // Native progress dialog only in native-UI mode; a themed host tracks the
    // downloadProgress() signal instead.
    if (m_useNativeUi)
    {
        m_progressDialog = new QProgressDialog(tr("Downloading update..."), tr("Cancel"), 0, 100, m_parentWidget);
        m_progressDialog->setWindowTitle(tr("Software Update"));
        m_progressDialog->setWindowModality(Qt::WindowModal);
        m_progressDialog->setMinimumDuration(0);
        m_progressDialog->setAutoClose(false);
        m_progressDialog->setAutoReset(false);
    }

    m_downloadReply = m_network->get(request);
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &UpdateManager::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished, this, &UpdateManager::onDownloadFinished);
    if (m_progressDialog)
        connect(m_progressDialog, &QProgressDialog::canceled, m_downloadReply, &QNetworkReply::abort);
}

void UpdateManager::cancelDownload()
{
    m_downloadCanceled = true;
    if (m_downloadReply)
        m_downloadReply->abort();
}

void UpdateManager::reportUpdateError(const QString &title, const QString &message)
{
    if (m_useNativeUi)
        QMessageBox::warning(m_parentWidget, title, message);
    else
        emit updateError(message);
}

void UpdateManager::onDownloadProgress(qint64 received, qint64 total)
{
    // Themed hosts render their own bar from this signal.
    emit downloadProgress(received, total);

    if (!m_progressDialog || total <= 0)
        return;

    m_progressDialog->setMaximum(static_cast<int>(total));
    m_progressDialog->setValue(static_cast<int>(received));
}

void UpdateManager::onDownloadFinished()
{
    QNetworkReply *reply = m_downloadReply;
    m_downloadReply = nullptr;

    const bool canceled = (m_progressDialog && m_progressDialog->wasCanceled())
                          || m_downloadCanceled;

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
        reportUpdateError(tr("Software Update"),
                          tr("The update could not be downloaded:\n%1").arg(errorString));
        return;
    }

    QFile out(m_downloadPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate) || out.write(payload) != payload.size())
    {
        QLog_Error(ERRORLOG, QString("Could not write update installer to %1").arg(m_downloadPath));
        reportUpdateError(tr("Software Update"),
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
        reportUpdateError(tr("Software Update"),
                          tr("The update was downloaded but the installer could not be started."));
        return;
    }

    QLog_Info(CONSOLELOG, "Update installer launched; shutting down for unattended install.");
    emit installerLaunched();
#elif defined(Q_OS_MACOS)
    // Unattended swap + relaunch (mirrors the Windows /waitpid + /relaunch flow).
    // The running app cannot overwrite its own bundle, so a detached helper waits
    // for this process to exit, replaces the installed .app with the freshly
    // downloaded one, and relaunches it.
    //
    // applicationDirPath() is ".../Project Notes.app/Contents/MacOS"; walk up three
    // levels to the .app. When the app is not running from a bundle (a dev build),
    // there is nothing to swap — just reveal the download and bail.
    const QDir macosDir(QCoreApplication::applicationDirPath());
    QDir appDir(macosDir);
    const bool inBundle = appDir.cdUp()        // Contents
                          && appDir.cdUp()      // Project Notes.app
                          && appDir.path().endsWith(".app");
    if (!inBundle)
    {
        QLog_Warning(CONSOLELOG, "Not running from a .app bundle; revealing download instead of swapping.");
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_downloadPath).absolutePath()));
        return;
    }

    const QString targetApp = appDir.path();
    const QString scriptPath = writeMacRelaunchScript(m_downloadPath, targetApp);
    if (scriptPath.isEmpty())
    {
        reportUpdateError(tr("Software Update"),
                          tr("The update was downloaded but the installer could not be prepared."));
        return;
    }

    if (!QProcess::startDetached("/bin/bash", {scriptPath}))
    {
        QLog_Error(ERRORLOG, QString("Failed to launch update helper %1").arg(scriptPath));
        reportUpdateError(tr("Software Update"),
                          tr("The update was downloaded but the installer could not be started."));
        return;
    }

    QLog_Info(CONSOLELOG, "Update helper launched; shutting down for unattended install.");
    emit installerLaunched();
#else
    // No unattended path on this platform; just reveal the download.
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_downloadPath).absolutePath()));
#endif
}

#if defined(Q_OS_MACOS)
QString UpdateManager::writeMacRelaunchScript(const QString &zipPath, const QString &targetApp)
{
    // The helper runs detached after the app quits. It waits for our PID to exit,
    // expands the downloaded .app, replaces the installed bundle (escalating to an
    // authenticated copy only if an in-place swap is denied — e.g. a pkg-installed
    // bundle owned by root in /Applications), and relaunches.
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString scriptPath = QDir(tempDir).filePath(PN::Identity::tempPrefix() + QStringLiteral("update.sh"));

    const QString script = QString(R"SH(#!/bin/bash
PID=%1
ZIP="%2"
TARGET="%3"
STAGE="$(mktemp -d -t %4update)"

# Wait for the running app to exit before touching its bundle.
while kill -0 "$PID" 2>/dev/null; do sleep 0.2; done

if ! /usr/bin/ditto -x -k "$ZIP" "$STAGE"; then
    /usr/bin/open "$STAGE"
    exit 1
fi

NEW_APP="$(/usr/bin/find "$STAGE" -maxdepth 2 -name '*.app' -type d | head -n 1)"
if [ -z "$NEW_APP" ]; then
    exit 1
fi

/usr/bin/xattr -dr com.apple.quarantine "$NEW_APP" 2>/dev/null || true

# Inner swap script keeps path quoting simple when re-run under osascript.
SWAP="$(mktemp -t %4swap)"
cat > "$SWAP" <<'SWAPEOF'
#!/bin/bash
set -e
/bin/rm -rf "$1"
/usr/bin/ditto "$2" "$1"
SWAPEOF

if ! /bin/bash "$SWAP" "$TARGET" "$NEW_APP" 2>/dev/null; then
    /usr/bin/osascript -e "do shell script \"/bin/bash '$SWAP' '$TARGET' '$NEW_APP'\" with administrator privileges" || exit 1
fi

/usr/bin/open "$TARGET"

/bin/rm -rf "$STAGE" "$SWAP" "$ZIP"
/bin/rm -f "$0"
)SH")
        .arg(QCoreApplication::applicationPid())
        .arg(zipPath, targetApp)
        .arg(PN::Identity::tempPrefix());

    QFile out(scriptPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
        out.write(script.toUtf8()) != script.toUtf8().size())
    {
        QLog_Error(ERRORLOG, QString("Could not write update helper to %1").arg(scriptPath));
        return QString();
    }
    out.close();

    return scriptPath;
}
#endif
