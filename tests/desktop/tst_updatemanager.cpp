#include <QtTest>
#include <QJsonObject>
#include <QJsonArray>
#include "updatemanager.h"

class UpdateManagerTest : public QObject
{
    Q_OBJECT
private slots:
    void comparesVersions()
    {
        QVERIFY(UpdateManager::isNewerVersion("6.4.0", "6.3.0"));
        QVERIFY(!UpdateManager::isNewerVersion("6.4.0", "6.4.0"));
        QVERIFY(!UpdateManager::isNewerVersion("3.4.0", "6.3.0"));
    }
    void ignoresUpcomingRelease()
    {
        UpdateManager updater;
        QSignalSpy available(&updater, &UpdateManager::updateAvailable);
        QSignalSpy current(&updater, &UpdateManager::upToDate);
        updater.processRelease({{"tag_name", "v99.0.0"}, {"upcoming_release", true}});
        QCOMPARE(available.count(), 0);
        QCOMPARE(current.count(), 1);
    }
    void rejectsMalformedVersion()
    {
        UpdateManager updater;
        QSignalSpy failed(&updater, &UpdateManager::checkFailed);
        updater.processRelease({{"tag_name", "v99.0.0-rc1"}});
        updater.processRelease({{"tag_name", "garbage"}});
        QCOMPARE(failed.count(), 2);
    }
    void currentRelease()
    {
        UpdateManager updater;
        QSignalSpy current(&updater, &UpdateManager::upToDate);
        updater.processRelease({{"tag_name", "v" + UpdateManager::currentVersion()}});
        QCOMPARE(current.count(), 1);
    }
    void gitlabReleaseLinks()
    {
        UpdateManager updater;
        QSignalSpy available(&updater, &UpdateManager::updateAvailable);
        QSignalSpy failed(&updater, &UpdateManager::checkFailed);
        const QJsonArray links{
            QJsonObject{{"name", "ProjectNotes-99.0.0-Windows-x64-Setup.exe"},
                        {"url", "https://gitlab.com/packages/windows.exe"},
                        {"direct_asset_url", "https://gitlab.com/releases/windows.exe"}},
            QJsonObject{{"name", "ProjectNotes-99.0.0-macOS.zip"},
                        {"url", "https://gitlab.com/packages/macOS.zip"}}};
        updater.processRelease({{"tag_name", "v99.0.0"}, {"description", "GitLab release notes"},
                                {"assets", QJsonObject{{"links", links}}}});
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
        QCOMPARE(available.count(), 1);
        QCOMPARE(available.at(0).at(1).toString(), QString("GitLab release notes"));
#if defined(Q_OS_WIN)
        QCOMPARE(available.at(0).at(2).toUrl(), QUrl("https://gitlab.com/releases/windows.exe"));
#else
        QCOMPARE(available.at(0).at(2).toUrl(), QUrl("https://gitlab.com/packages/macOS.zip"));
#endif
#else
        QCOMPARE(available.count(), 0);
        QCOMPARE(failed.count(), 1);
        QVERIFY(failed.at(0).at(0).toString().contains("flatpak update"));
#endif
    }
};
QTEST_MAIN(UpdateManagerTest)
#include "tst_updatemanager.moc"
