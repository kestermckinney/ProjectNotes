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

class ReleaseNetwork final : public QNetworkAccessManager
{
public:
    QByteArray body;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QNetworkRequest lastRequest;
    int requestCount = 0;

protected:
    QNetworkReply *createRequest(Operation, const QNetworkRequest &request,
                                 QIODevice *) override
    {
        lastRequest = request;
        ++requestCount;
        return new ReleaseReply(request, body, error, this);
    }
};

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
        QCOMPARE(UpdateManager::selectInstallerLink(links, {}, "-Windows-x64-Setup.exe"), direct);
        QCOMPARE(UpdateManager::selectInstallerLink(links, "macOS", ".zip"), backing);

        // Plain asset links are supported without a direct_asset_path.
        links = QJsonArray{QJsonObject{{"name", windows.toUpper()}, {"url", backing}}};
        QCOMPARE(UpdateManager::selectInstallerLink(links, {}, "-Windows-x64-Setup.exe"), backing);

        // Missing/unsafe URLs do not hide a subsequent valid installer link.
        links = QJsonArray{
            QJsonObject{{"name", windows}},
            QJsonObject{{"name", windows}, {"url", "file:///tmp/setup.exe"}},
            QJsonObject{{"name", windows}, {"url", "http://example.com/setup.exe"}},
            QJsonObject{{"name", windows}, {"url", "/relative/setup.exe"}},
            QJsonObject{{"name", windows}, {"url", backing}}
        };
        QCOMPARE(UpdateManager::selectInstallerLink(links, {}, "-Windows-x64-Setup.exe"), backing);
        QVERIFY(UpdateManager::selectInstallerLink(links, "macOS", ".zip").isEmpty());
        QVERIFY(UpdateManager::selectInstallerLink({}, {}, "-Windows-x64-Setup.exe").isEmpty());
    }

    void releaseResponses_data()
    {
        QTest::addColumn<QByteArray>("body");
        QTest::addColumn<QString>("result");
        QTest::addColumn<bool>("networkError");
        QTest::newRow("current") << QJsonDocument(QJsonObject{
            {"tag_name", "v" + UpdateManager::currentVersion()}}).toJson()
            << QStringLiteral("current") << false;
        QTest::newRow("older") << QByteArray(R"({"tag_name":"v1.0.0"})")
            << QStringLiteral("current") << false;
        QTest::newRow("invalid-json") << QByteArray("not JSON") << QStringLiteral("failed") << false;
        QTest::newRow("array-response") << QByteArray("[]") << QStringLiteral("failed") << false;
        QTest::newRow("missing-version") << QByteArray("{}") << QStringLiteral("failed") << false;
        QTest::newRow("missing-installer") << QByteArray(R"({"tag_name":"v999.0.0","assets":{"links":[]}})")
            << QStringLiteral("failed") << false;
        QTest::newRow("server-error") << QByteArray() << QStringLiteral("failed") << true;
        const QByteArray release = R"({"tag_name":"v999.0.0","description":"GitLab release notes","assets":{"sources":[{"format":"zip","url":"https://gitlab.com/source.zip"}],"links":[{"name":"ProjectNotes-Windows-x64-Setup.exe","direct_asset_url":"https://gitlab.com/downloads/ProjectNotes-Windows-x64-Setup.exe","url":"https://gitlab.com/packages/windows"},{"name":"ProjectNotes-macOS.zip","url":"https://gitlab.com/packages/ProjectNotes-macOS.zip"}]}})";
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
        QTest::newRow("gitlab-assets") << release << QStringLiteral("available") << false;
#else
        QTest::newRow("linux-has-no-native-installer") << release << QStringLiteral("failed") << false;
#endif
    }

    void releaseResponses()
    {
        QFETCH(QByteArray, body);
        QFETCH(QString, result);
        QFETCH(bool, networkError);
        UpdateManager updater;
        delete updater.m_network;
        auto *network = new ReleaseNetwork;
        network->setParent(&updater);
        network->body = body;
        if (networkError)
            network->error = QNetworkReply::ServiceUnavailableError;
        updater.m_network = network;
        QSignalSpy available(&updater, &UpdateManager::updateAvailable);
        QSignalSpy current(&updater, &UpdateManager::upToDate);
        QSignalSpy failed(&updater, &UpdateManager::checkFailed);
        updater.checkForUpdates(true);
        updater.checkForUpdates(true); // Coalesce concurrent checks.
        QTRY_COMPARE(available.size() + current.size() + failed.size(), 1);
        QCOMPARE(network->requestCount, 1);
        QCOMPARE(network->lastRequest.url().toString(),
                 QStringLiteral("https://gitlab.com/api/v4/projects/kestermckinney%2FProjectNotes/releases/permalink/latest"));
        QCOMPARE(network->lastRequest.rawHeader("Accept"), QByteArray("application/json"));
        QVERIFY(!network->lastRequest.hasRawHeader("X-GitHub-Api-Version"));
        QVERIFY(!network->lastRequest.hasRawHeader("PRIVATE-TOKEN"));
        QVERIFY(!network->lastRequest.hasRawHeader("Authorization"));
        if (result == "available")
        {
            QCOMPARE(available.size(), 1);
            QCOMPARE(available.at(0).at(0).toString(), QStringLiteral("999.0.0"));
            QCOMPARE(available.at(0).at(1).toString(), QStringLiteral("GitLab release notes"));
#if defined(Q_OS_WIN)
            QCOMPARE(available.at(0).at(2).toUrl(), QUrl("https://gitlab.com/downloads/ProjectNotes-Windows-x64-Setup.exe"));
#elif defined(Q_OS_MACOS)
            QCOMPARE(available.at(0).at(2).toUrl(), QUrl("https://gitlab.com/packages/ProjectNotes-macOS.zip"));
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
};

QTEST_MAIN(UpdateManagerTest)
#include "tst_updatemanager.moc"
