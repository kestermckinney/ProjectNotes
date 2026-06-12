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
// platform installer, and launches it. On Windows the installer runs silently
// and relaunches the app; on macOS the downloaded .dmg is opened for the user.
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

    // "5.2.0" assembled from version.h.
    static QString currentVersion();

    // True when candidate is a strictly higher semantic version than current.
    static bool isNewerVersion(const QString &candidate, const QString &current);

signals:
    void updateAvailable(const QString &version, const QString &releaseNotes, const QUrl &assetUrl);
    void upToDate();
    void checkFailed(const QString &error);
    void installerLaunched();

private slots:
    void onCheckFinished();
    void onDownloadFinished();
    void onDownloadProgress(qint64 received, qint64 total);

private:
    // Returns the browser_download_url of the asset matching the running platform,
    // or an empty string when the release carries no suitable installer.
    static QString selectPlatformAsset(const class QJsonArray &assets);

    void launchInstaller();

    bool m_silent = false;
    QWidget *m_parentWidget = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_checkReply = nullptr;
    QNetworkReply *m_downloadReply = nullptr;
    QProgressDialog *m_progressDialog = nullptr;
    QString m_downloadPath;
};

#endif // UPDATEMANAGER_H
