// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QString>
#include <QUrl>

class QWidget;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;

// Checks GitHub Releases for a newer ProjectNotes build, downloads the
// platform installer, and launches it. On both Windows and macOS the install is
// unattended and the app relaunches itself: Windows runs the NSIS installer
// silently (it waits on /waitpid, then /relaunch), while macOS launches a
// detached helper that waits for the app to quit, swaps the installed .app for
// the freshly downloaded bundle, and reopens it.
//
// Networking is asynchronous: callers invoke checkForUpdates()/downloadAndInstall()
// and react to the signals below. The "silent" flag passed to checkForUpdates is
// remembered so automatic (startup/daily) checks stay quiet when there is nothing
// to do, while a manual Help-menu check always reports its result.
class UpdateManager : public QObject
{
    Q_OBJECT

public:
    explicit UpdateManager(QWidget *parent = nullptr);

    // Query the GitHub "latest release" endpoint. When silent is true,
    // upToDate()/checkFailed() are emitted but callers are expected to ignore them.
    void checkForUpdates(bool silent);

    // Download the given installer asset (shown with a progress dialog) and,
    // on success, launch it and emit installerLaunched() so the app can shut down.
    void downloadAndInstall(const QUrl &assetUrl);

    // Semantic version assembled from version.h.
    static QString currentVersion();

    // True when candidate is a strictly higher semantic version than current.
    static bool isNewerVersion(const QString &candidate, const QString &current);

    // When false, the download shows NO native QProgressDialog and errors are
    // reported via downloadProgress()/updateError() signals instead of native
    // QMessageBoxes — so a host (the QML app) can render its own themed UI.
    // Defaults true, so the Widgets app is unaffected.
    void setUseNativeUi(bool use) { m_useNativeUi = use; }

    // Abort an in-flight download (themed Cancel button).
    void cancelDownload();

signals:
    void updateAvailable(const QString &version, const QString &releaseNotes, const QUrl &assetUrl);
    void upToDate();
    void checkFailed(const QString &error);
    void installerLaunched();
    // Emitted during a download so a themed host can show progress (total <= 0
    // means the size is unknown → show an indeterminate bar).
    void downloadProgress(qint64 received, qint64 total);
    // A download / install error, emitted only when setUseNativeUi(false).
    void updateError(const QString &message);

private slots:
    void onCheckFinished();
    void onDownloadFinished();
    void onDownloadProgress(qint64 received, qint64 total);

private:
    // Returns the browser_download_url of the asset matching the running platform,
    // or an empty string when the release carries no suitable installer.
    static QString selectPlatformAsset(const class QJsonArray &assets);

    void launchInstaller();

    // Report a download/install failure: native QMessageBox when m_useNativeUi,
    // else the updateError() signal for a themed host to display.
    void reportUpdateError(const QString &title, const QString &message);

#if defined(Q_OS_MACOS)
    // Writes the detached swap/relaunch helper to a temp file (see launchInstaller).
    // Returns the script path, or an empty string on failure.
    QString writeMacRelaunchScript(const QString &zipPath, const QString &targetApp);
#endif

    bool m_silent = false;
    bool m_useNativeUi = true;
    bool m_downloadCanceled = false;
    QWidget *m_parentWidget = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_checkReply = nullptr;
    QNetworkReply *m_downloadReply = nullptr;
    QProgressDialog *m_progressDialog = nullptr;
    QString m_downloadPath;
};

#endif // UPDATEMANAGER_H
