// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "updatemanager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>
#include <cstring>

class ReleaseReply final : public QNetworkReply
{
public:
    ReleaseReply(const QNetworkRequest &request, QByteArray body,
                 NetworkError error, QObject *parent)
        : QNetworkReply(parent), m_body(std::move(body))
    {
        setRequest(request);
        setUrl(request.url());
        open(QIODevice::ReadOnly);
        if (error != NoError)
            setError(error, QStringLiteral("Release server unavailable"));
        QTimer::singleShot(0, this, [this] {
            setFinished(true);
            emit finished();
        });
    }

    void abort() override {}
    qint64 bytesAvailable() const override
    {
        return m_body.size() + QNetworkReply::bytesAvailable();
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 size = qMin(maxSize, qint64(m_body.size()));
        if (!size)
            return -1;
        std::memcpy(data, m_body.constData(), size);
        m_body.remove(0, size);
        return size;
    }

private:
    QByteArray m_body;
};

// Answers the GitHub and GitLab release endpoints separately. GitHub answers
// 404 unless a test gives it a body, as it does until releases move there.
class ReleaseNetwork final : public QNetworkAccessManager
{
public:
    QByteArray githubBody;
    QNetworkReply::NetworkError githubError = QNetworkReply::ContentNotFoundError;
    QByteArray gitlabBody;
    QNetworkReply::NetworkError gitlabError = QNetworkReply::NoError;
    QList<QNetworkRequest> requests;

    QNetworkRequest request(const QString &host) const
    {
        for (const QNetworkRequest &r : requests)
            if (r.url().host() == host)
                return r;
        return {};
    }

protected:
    QNetworkReply *createRequest(Operation, const QNetworkRequest &request,
                                 QIODevice *) override
    {
        requests.append(request);
        const bool github = request.url().host() == QStringLiteral("api.github.com");
        return new ReleaseReply(request, github ? githubBody : gitlabBody,
                                github ? githubError : gitlabError, this);
    }
};

namespace {

QByteArray gitlabRelease(const QString &version)
{
    return QString::fromUtf8(R"({"tag_name":"v%1","description":"GitLab release notes","assets":{"sources":[{"format":"zip","url":"https://gitlab.com/source.zip"}],"links":[{"name":"ProjectNotes-Windows-x64-Setup.exe","direct_asset_url":"https://gitlab.com/downloads/ProjectNotes-Windows-x64-Setup.exe","url":"https://gitlab.com/packages/windows"},{"name":"ProjectNotes-macOS.zip","url":"https://gitlab.com/packages/ProjectNotes-macOS.zip"}]}})")
        .arg(version).toUtf8();
}

QByteArray githubRelease(const QString &version)
{
    return QString::fromUtf8(R"({"tag_name":"v%1","body":"GitHub release notes","draft":false,"prerelease":false,"assets":[{"name":"ProjectNotesRemoteHost-%1-Windows-x64-Setup.exe","browser_download_url":"https://github.com/kestermckinney/ProjectNotes/releases/download/v%1/ProjectNotesRemoteHost-%1-Windows-x64-Setup.exe"},{"name":"ProjectNotes-%1-Windows-x64-Setup.exe","browser_download_url":"https://github.com/kestermckinney/ProjectNotes/releases/download/v%1/ProjectNotes-%1-Windows-x64-Setup.exe"},{"name":"ProjectNotes-%1-macOS.zip","browser_download_url":"https://github.com/kestermckinney/ProjectNotes/releases/download/v%1/ProjectNotes-%1-macOS.zip"}]})")
        .arg(version).toUtf8();
}

} // namespace

class UpdateManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void installerLinks()
    {
        const QString windows = QStringLiteral("ProjectNotes-Windows-x64-Setup.exe");
        const QString mac = QStringLiteral("ProjectNotes-macOS.zip");
        const QString direct = QStringLiteral("https://gitlab.com/release/ProjectNotes-Windows-x64-Setup.exe");
        const QString backing = QStringLiteral("https://gitlab.com/packages/opaque");
        QJsonArray links{
            QJsonObject{{"name", "source.zip"}, {"url", backing}},
            QJsonObject{{"name", windows}, {"url", backing}, {"direct_asset_url", direct}},
            QJsonObject{{"name", "ProjectNotes-macOS.dmg"}, {"url", backing}},
            QJsonObject{{"name", mac}, {"url", backing}}
        };
        QCOMPARE(UpdateManager::selectInstallerLink(links, "ProjectNotes-", "-Windows-x64-Setup.exe"), direct);
        QCOMPARE(UpdateManager::selectInstallerLink(links, "macOS", ".zip"), backing);

        // Plain asset links are supported without a direct_asset_path.
        links = QJsonArray{QJsonObject{{"name", windows.toUpper()}, {"url", backing}}};
        QCOMPARE(UpdateManager::selectInstallerLink(links, "ProjectNotes-", "-Windows-x64-Setup.exe"), backing);

        // The published GitLab release uses descriptive names. Its Remote Host
        // link precedes the desktop installer and must not be selected.
        links = QJsonArray{
            QJsonObject{{"name", "Project Notes Remote Host 6.4.1 Windows x64 Installer"},
                        {"direct_asset_url", "https://gitlab.com/releases/ProjectNotesRemoteHost-6.4.1-Windows-x64-Setup.exe"}},
            QJsonObject{{"name", "Project Notes 6.4.1 Windows x64 Installer"},
                        {"direct_asset_url", "https://gitlab.com/releases/ProjectNotes-6.4.1-Windows-x64-Setup.exe"},
                        {"url", "https://gitlab.com/packages/opaque"}}
        };
        QCOMPARE(UpdateManager::selectInstallerLink(links, "ProjectNotes-", "-Windows-x64-Setup.exe"),
                 QStringLiteral("https://gitlab.com/releases/ProjectNotes-6.4.1-Windows-x64-Setup.exe"));

        links = QJsonArray{QJsonObject{
            {"name", "Project Notes 6.4.1 Windows x64 Installer"},
            {"url", "https://gitlab.com/packages/ProjectNotes-6%2E4%2E1-Windows-x64-Setup%2Eexe"}}};
        QCOMPARE(UpdateManager::selectInstallerLink(links, "ProjectNotes-", "-Windows-x64-Setup.exe"),
                 QStringLiteral("https://gitlab.com/packages/ProjectNotes-6%2E4%2E1-Windows-x64-Setup%2Eexe"));

        // Missing/unsafe URLs do not hide a subsequent valid installer link.
        links = QJsonArray{
            QJsonObject{{"name", windows}},
            QJsonObject{{"name", windows}, {"url", "file:///tmp/setup.exe"}},
            QJsonObject{{"name", windows}, {"url", "http://example.com/setup.exe"}},
            QJsonObject{{"name", windows}, {"url", "/relative/setup.exe"}},
            QJsonObject{{"name", windows}, {"url", backing}}
        };
        QCOMPARE(UpdateManager::selectInstallerLink(links, "ProjectNotes-", "-Windows-x64-Setup.exe"), backing);
        QVERIFY(UpdateManager::selectInstallerLink(links, "macOS", ".zip").isEmpty());
        QVERIFY(UpdateManager::selectInstallerLink({}, "ProjectNotes-", "-Windows-x64-Setup.exe").isEmpty());
    }

    void releaseResponses_data()
    {
        QTest::addColumn<QByteArray>("gitlabBody");
        QTest::addColumn<bool>("gitlabError");
        QTest::addColumn<QByteArray>("githubBody");
        QTest::addColumn<bool>("githubError");
        QTest::addColumn<QString>("result");
        QTest::addColumn<QString>("notes");

        const QByteArray none;
        const QByteArray current = QJsonDocument(QJsonObject{
            {"tag_name", "v" + UpdateManager::currentVersion()}}).toJson();

        // GitLab alone, as today: GitHub answers 404.
        QTest::newRow("current") << current << false << none << true << "current" << "";
        QTest::newRow("older") << QByteArray(R"({"tag_name":"v1.0.0"})") << false << none << true
                               << "current" << "";
        QTest::newRow("invalid-json") << QByteArray("not JSON") << false << none << true << "failed" << "";
        QTest::newRow("array-response") << QByteArray("[]") << false << none << true << "failed" << "";
        QTest::newRow("missing-version") << QByteArray("{}") << false << none << true << "failed" << "";
        QTest::newRow("prerelease-tag") << QByteArray(R"({"tag_name":"v999.0.0-rc1"})") << false << none << true
                                        << "failed" << "";
        QTest::newRow("upcoming-release") << QByteArray(R"({"tag_name":"v999.0.0","upcoming_release":true})")
                                          << false << none << true << "current" << "";
        QTest::newRow("missing-installer") << QByteArray(R"({"tag_name":"v999.0.0","assets":{"links":[]}})")
                                           << false << none << true << "failed" << "";
        QTest::newRow("server-error") << none << true << none << true << "failed" << "";
        QTest::newRow("gitlab-assets") << gitlabRelease("999.0.0") << false << none << true
                                       << "available-gitlab" << "GitLab release notes";

        // GitHub alone, after the move: GitLab is gone or unreachable.
        QTest::newRow("github-assets") << none << true << githubRelease("999.0.0") << false
                                       << "available-github" << "GitHub release notes";
        QTest::newRow("github-current") << none << true << current << false << "current" << "";
        QTest::newRow("github-prerelease")
            << none << true << QByteArray(R"({"tag_name":"v999.0.0","prerelease":true})") << false
            << "current" << "";

        // Both answer.
        QTest::newRow("same-version-prefers-github") << gitlabRelease("999.0.0") << false
                                                     << githubRelease("999.0.0") << false
                                                     << "available-github" << "GitHub release notes";
        QTest::newRow("newer-gitlab-beats-stale-github") << gitlabRelease("999.1.0") << false
                                                         << githubRelease("999.0.0") << false
                                                         << "available-gitlab" << "GitLab release notes";
        QTest::newRow("newer-github-beats-gitlab") << gitlabRelease("999.0.0") << false
                                                   << githubRelease("999.1.0") << false
                                                   << "available-github" << "GitHub release notes";
        QTest::newRow("same-version-github-without-installer")
            << gitlabRelease("999.0.0") << false
            << QByteArray(R"({"tag_name":"v999.0.0","body":"GitHub release notes","assets":[]})") << false
            << "available-gitlab" << "GitLab release notes";
        QTest::newRow("github-error-gitlab-current") << current << false << none << true << "current" << "";
    }

    void releaseResponses()
    {
        QFETCH(QByteArray, gitlabBody);
        QFETCH(bool, gitlabError);
        QFETCH(QByteArray, githubBody);
        QFETCH(bool, githubError);
        QFETCH(QString, result);
        QFETCH(QString, notes);
        UpdateManager updater;
        delete updater.m_network;
        auto *network = new ReleaseNetwork;
        network->setParent(&updater);
        network->gitlabBody = gitlabBody;
        network->gitlabError = gitlabError ? QNetworkReply::ServiceUnavailableError : QNetworkReply::NoError;
        network->githubBody = githubBody;
        network->githubError = githubError ? QNetworkReply::ContentNotFoundError : QNetworkReply::NoError;
        updater.m_network = network;
        QSignalSpy available(&updater, &UpdateManager::updateAvailable);
        QSignalSpy current(&updater, &UpdateManager::upToDate);
        QSignalSpy failed(&updater, &UpdateManager::checkFailed);
        QSignalSpy moveRemote(&updater, &UpdateManager::linuxRepositoryChangeNeeded);
        updater.checkForUpdates(true);
        updater.checkForUpdates(true); // Coalesce concurrent checks.
        QTRY_COMPARE(available.size() + current.size() + failed.size() + moveRemote.size(), 1);
        QTest::qWait(10);
        // one answer for both hosts
        QCOMPARE(available.size() + current.size() + failed.size() + moveRemote.size(), 1);

        QCOMPARE(network->requests.size(), 2);
        const QNetworkRequest gitlab = network->request(QStringLiteral("gitlab.com"));
        QCOMPARE(gitlab.url().toString(),
                 QStringLiteral("https://gitlab.com/api/v4/projects/kestermckinney%2FProjectNotes/releases/permalink/latest"));
        QCOMPARE(gitlab.rawHeader("Accept"), QByteArray("application/json"));
        QVERIFY(!gitlab.hasRawHeader("X-GitHub-Api-Version"));
        const QNetworkRequest github = network->request(QStringLiteral("api.github.com"));
        QCOMPARE(github.url().toString(),
                 QStringLiteral("https://api.github.com/repos/kestermckinney/ProjectNotes/releases/latest"));
        QCOMPARE(github.rawHeader("Accept"), QByteArray("application/vnd.github+json"));
        QCOMPARE(github.rawHeader("X-GitHub-Api-Version"), QByteArray("2022-11-28"));
        for (const QNetworkRequest &r : {gitlab, github})
        {
            QVERIFY(!r.hasRawHeader("PRIVATE-TOKEN"));
            QVERIFY(!r.hasRawHeader("Authorization"));
        }

#if defined(Q_OS_LINUX)
        // Flatpak owns updates on Linux, so a newer release is reported, not
        // offered. One found on GitHub asks the user to move the remote there.
        if (result == "available-github")
        {
            QCOMPARE(moveRemote.size(), 1);
            QVERIFY(moveRemote.at(0).at(0).toString().startsWith(QStringLiteral("999.")));
            QCOMPARE(moveRemote.at(0).at(1).toUrl(), QUrl(UpdateManager::linuxInstallInstructionsUrl()));
            QCOMPARE(moveRemote.at(0).at(1).toUrl().host(), QStringLiteral("www.projectnotespro.com"));
            return;
        }
        if (result.startsWith("available"))
        {
            QCOMPARE(failed.size(), 1);
            QVERIFY(failed.at(0).at(0).toString().contains("flatpak update"));
            return;
        }
#endif
        QCOMPARE(moveRemote.size(), 0); // Linux only
        if (result.startsWith("available"))
        {
            QCOMPARE(available.size(), 1);
            QCOMPARE(available.at(0).at(1).toString(), notes);
            const QString host = result == "available-github" ? "github.com" : "gitlab.com";
            const QUrl url = available.at(0).at(2).toUrl();
            QCOMPARE(url.host(), host);
#if defined(Q_OS_WIN)
            QVERIFY(url.path().endsWith("-Windows-x64-Setup.exe"));
#elif defined(Q_OS_MACOS)
            QVERIFY(url.path().endsWith("macOS.zip"));
#endif
        }
        else if (result == "current")
            QCOMPARE(current.size(), 1);
        else
        {
            QCOMPARE(failed.size(), 1);
            QVERIFY(!failed.at(0).at(0).toString().isEmpty());
        }
    }

    void chooseRelease()
    {
        using Candidate = UpdateManager::ReleaseCandidate;
        Candidate github{UpdateManager::ReleaseSource::GitHub};
        Candidate gitlab{UpdateManager::ReleaseSource::GitLab};
        QVERIFY(!UpdateManager::chooseRelease(github, gitlab));
        gitlab.version = "6.5.2";
        QCOMPARE(UpdateManager::chooseRelease(github, gitlab), &gitlab);
        github.version = "6.5.2";
        QCOMPARE(UpdateManager::chooseRelease(github, gitlab), &github);
        github.version = "6.4.3"; // an old release left behind on GitHub
        QCOMPARE(UpdateManager::chooseRelease(github, gitlab), &gitlab);
        github.version = "6.10.0";
        QCOMPARE(UpdateManager::chooseRelease(github, gitlab), &github);
    }

};

QTEST_MAIN(UpdateManagerTest)
#include "tst_updatemanager.moc"
