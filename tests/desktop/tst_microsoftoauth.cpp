// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectNotesIntegrations/MicrosoftOAuthManager.h"
#include "ProjectNotesIntegrations/Office365Service.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest/QtTest>

using namespace PN::Comm;

class FakeHttpTransport final : public HttpTransport {
public:
    QList<HttpRequest> requests;
    QByteArray nextBody;
    ServiceError nextError;

    void send(HttpRequest request, Completion completion) override
    {
        requests.append(request);
        completion({request.operationId, 200, {}, nextBody, nextError});
    }
    void cancel(const QUuid &) override {}
};

class MicrosoftOAuthManagerTest final : public QObject {
    Q_OBJECT
private slots:
    void deviceCodeFlowUsesInjectedTransport();
    void qualifiesDeveloperProfileSecretsOnly();
    void requiresExplicitFeatureScopes();
    void emailDraftConsentPreservesFileFinderSession();
    void facadeAddsAuthorizationWithoutExposingToken();
    void refreshFailureAndSignOutClearCredentialState();
    void sharedServiceOwnsIdentitySettings();
};

void MicrosoftOAuthManagerTest::deviceCodeFlowUsesInjectedTransport()
{
    FakeHttpTransport transport;
    transport.nextBody = QJsonDocument(QJsonObject{
        {"device_code", "synthetic-device"}, {"user_code", "ABCD-EFGH"},
        {"verification_uri", "https://login.example.test/device"}, {"expires_in", 900}, {"interval", 5}
    }).toJson(QJsonDocument::Compact);
    MicrosoftOAuthManager manager;
    manager.setHttpTransport(&transport);
    manager.configure("tenant", "client");
    manager.startSignIn();
    manager.startSignIn();

    QCOMPARE(transport.requests.size(), 1);
    QCOMPARE(transport.requests.constFirst().method, QByteArray("POST"));
    QVERIFY(transport.requests.constFirst().url.path().endsWith("/devicecode"));
    QVERIFY(transport.requests.constFirst().body.contains("client_id=client"));
    QCOMPARE(manager.userCode(), QStringLiteral("ABCD-EFGH"));
    QCOMPARE(manager.verificationUrl(), QUrl(QStringLiteral("https://login.example.test/device")));
    QVERIFY(manager.authenticationInProgress());
}

void MicrosoftOAuthManagerTest::facadeAddsAuthorizationWithoutExposingToken()
{
    FakeHttpTransport transport;
    transport.nextBody = QJsonDocument(QJsonObject{
        {"access_token", "synthetic-access"}, {"refresh_token", "synthetic-refresh"}, {"expires_in", 3600},
        {"scope", "offline_access User.Read Mail.ReadWrite"}
    }).toJson(QJsonDocument::Compact);
    MicrosoftOAuthManager manager;
    manager.setHttpTransport(&transport);
    manager.setSecretStore([](const QString &, QString *) { return QStringLiteral("saved-refresh"); }, {}, {});
    QHash<QString, QStringList> persistedScopes;
    manager.setGrantedScopesStore(
        [&persistedScopes](const QString &account) { return persistedScopes.value(account); },
        [&persistedScopes](const QString &account, const QStringList &scopes) { persistedScopes.insert(account, scopes); },
        [&persistedScopes](const QString &account) { persistedScopes.remove(account); });
    persistedScopes.insert(QStringLiteral("tenant/client"),
                           {QStringLiteral("offline_access"), QStringLiteral("User.Read"),
                            QStringLiteral("Mail.ReadWrite")});
    manager.configure("tenant", "client");
    Office365Service service;
    service.setOAuthManager(&manager);
    service.setHttpTransport(&transport);
    manager.restoreSession();
    QVERIFY(service.account().authenticated);
    QVERIFY(transport.requests.constFirst().body.contains("Mail.ReadWrite"));
    QCOMPARE(service.account().generation, manager.sessionGeneration());
    QCOMPARE(service.account().grantedScopes,
             QStringList({QStringLiteral("offline_access"), QStringLiteral("User.Read"),
                          QStringLiteral("Mail.ReadWrite")}));
    QCOMPARE(persistedScopes.value(QStringLiteral("tenant/client")), service.account().grantedScopes);
    MicrosoftOAuthManager restored;
    restored.setGrantedScopesStore(
        [&persistedScopes](const QString &account) { return persistedScopes.value(account); }, {}, {});
    restored.configure("tenant", "client");
    QCOMPARE(restored.grantedScopeList(), service.account().grantedScopes);

    HttpRequest request;
    request.operationId = QUuid::createUuid();
    request.method = "GET";
    request.url = QUrl(QStringLiteral("https://graph.microsoft.com/v1.0/me"));
    request.headers.insert("Authorization", "attacker supplied");
    bool completed = false;
    service.sendGraphRequest(request, [&completed](HttpResponse) { completed = true; });
    QVERIFY(completed);
    QCOMPARE(transport.requests.constLast().headers.value("Authorization"), QByteArray("Bearer synthetic-access"));

    transport.nextBody = QJsonDocument(QJsonObject{
        {"displayName", "Ada Example"}, {"mail", "ada@example.test"}
    }).toJson(QJsonDocument::Compact);
    service.refreshIdentity();
    QCOMPARE(service.account().label, QStringLiteral("Ada Example <ada@example.test>"));

    transport.nextError = {QStringLiteral("synthetic-network-error"), QStringLiteral("Synthetic failure")};
    service.refreshIdentity();
    QCOMPARE(service.account().label, QStringLiteral("Ada Example <ada@example.test>"));

    Office365Service unsignedService;
    bool rejected = false;
    unsignedService.sendGraphRequest(request, [&rejected](HttpResponse response) {
        rejected = response.error.code == QStringLiteral("office365-not-authenticated");
    });
    QVERIFY(rejected);

    manager.signOut();
    QVERIFY(!service.account().authenticated);
    QVERIFY(service.account().grantedScopes.isEmpty());
    QVERIFY(!persistedScopes.contains(QStringLiteral("tenant/client")));
}

void MicrosoftOAuthManagerTest::sharedServiceOwnsIdentitySettings()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       temporary.filePath(QStringLiteral("settings")));
    const QString organization = QStringLiteral("ProjectNotes-Test-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSettings canonical(organization, QStringLiteral("AppSettings"));
    canonical.setFallbacksEnabled(false);
    canonical.clear();

    Office365Service service;
    service.configureIdentity(organization, QStringLiteral("organizations"), {});
    service.setIdentity(QStringLiteral("tenant-a"), QStringLiteral("client-a"));

    QCOMPARE(service.tenantId(), QStringLiteral("tenant-a"));
    QCOMPARE(service.clientId(), QStringLiteral("client-a"));
    QCOMPARE(canonical.value(QStringLiteral("FileFinder/tenantId")).toString(),
             QStringLiteral("tenant-a"));
    QCOMPARE(canonical.value(QStringLiteral("FileFinder/clientId")).toString(),
             QStringLiteral("client-a"));

    canonical.clear();
}

void MicrosoftOAuthManagerTest::requiresExplicitFeatureScopes()
{
    FakeHttpTransport transport;
    transport.nextBody = QJsonDocument(QJsonObject{
        {"device_code", "synthetic-device"}, {"user_code", "ABCD-EFGH"},
        {"verification_uri", "https://login.example.test/device"}
    }).toJson(QJsonDocument::Compact);
    MicrosoftOAuthManager manager;
    manager.setHttpTransport(&transport);
    bool cleared = false;
    QObject::connect(&manager, &MicrosoftOAuthManager::accessTokenChanged,
                     [&cleared](const QString &token) { cleared = token.isEmpty(); });
    manager.setRequestedScopes({"offline_access", "User.Read", "Mail.ReadWrite"});
    const quint64 generation = manager.sessionGeneration();
    manager.configure("tenant", "client");
    manager.startSignIn();
    QCOMPARE(manager.sessionGeneration(), generation + 1);
    QVERIFY(cleared);
    const QByteArray body = transport.requests.constFirst().body;
    QVERIFY(body.contains("Mail.ReadWrite"));
    QVERIFY(!body.contains("Mail.Send"));
    QVERIFY(!body.contains("Calendars"));
}

void MicrosoftOAuthManagerTest::emailDraftConsentPreservesFileFinderSession()
{
    FakeHttpTransport transport;
    transport.nextBody = QJsonDocument(QJsonObject{
        {"access_token", "file-finder-access"}, {"refresh_token", "file-finder-refresh"},
        {"expires_in", 3600},
        {"scope", "offline_access https://graph.microsoft.com/Team.ReadBasic.All "
                  "https://graph.microsoft.com/Channel.ReadBasic.All "
                  "https://graph.microsoft.com/Files.Read.All"}
    }).toJson(QJsonDocument::Compact);
    MicrosoftOAuthManager manager;
    manager.setSecretStore([](const QString &, QString *) { return QStringLiteral("saved-refresh"); }, {}, {});
    manager.configure("tenant", "client");
    Office365Service service;
    service.setHttpTransport(&transport);
    service.setOAuthManager(&manager);
    service.restoreSession();

    QVERIFY(service.account().authenticated);
    QVERIFY(service.account().grantedScopes.contains(
        QStringLiteral("https://graph.microsoft.com/Files.Read.All")));

    service.requestEmailDraftConsent();
    QVERIFY(service.account().authenticated);
    QVERIFY(service.account().grantedScopes.contains(
        QStringLiteral("https://graph.microsoft.com/Files.Read.All")));

    transport.nextBody = QJsonDocument(QJsonObject{
        {"device_code", "draft-device"}, {"user_code", "DRAFT-CODE"},
        {"verification_uri", "https://login.example.test/device"}, {"interval", 1}
    }).toJson(QJsonDocument::Compact);
    service.startSignIn();
    const QByteArray consentBody = transport.requests.constLast().body;
    QVERIFY(consentBody.contains("User.Read"));
    QVERIFY(consentBody.contains("Mail.ReadWrite"));
    QVERIFY(!consentBody.contains("Mail.Send"));
    QVERIFY(!consentBody.contains("Calendars"));
    QVERIFY(!consentBody.contains("Files.Read"));
    QVERIFY(!consentBody.contains("Team.Read"));

    transport.nextBody = QJsonDocument(QJsonObject{
        {"error", "authorization_declined"}, {"error_description", "Synthetic denial"}
    }).toJson(QJsonDocument::Compact);
    QTRY_VERIFY(!manager.authenticationInProgress());
    QVERIFY(service.account().authenticated);
    QVERIFY(service.account().grantedScopes.contains(
        QStringLiteral("https://graph.microsoft.com/Files.Read.All")));
}

void MicrosoftOAuthManagerTest::refreshFailureAndSignOutClearCredentialState()
{
    FakeHttpTransport transport;
    transport.nextBody = QJsonDocument(QJsonObject{
        {"access_token", "synthetic-access"}, {"refresh_token", "rotated-refresh"}, {"expires_in", 3600}
    }).toJson(QJsonDocument::Compact);
    QString removedAccount;
    MicrosoftOAuthManager manager;
    manager.setHttpTransport(&transport);
    manager.setSecretStore([](const QString &, QString *) { return QStringLiteral("saved-refresh"); }, {},
                           [&removedAccount](const QString &account, QString *) {
                               removedAccount = account;
                               return true;
                           });
    manager.configure("tenant", "client");
    manager.restoreSession();
    QVERIFY(manager.authenticated());
    const quint64 authenticatedGeneration = manager.sessionGeneration();
    manager.signOut();
    QVERIFY(!manager.authenticated());
    QVERIFY(!manager.authenticationInProgress());
    QCOMPARE(manager.status(), QStringLiteral("Signed out"));
    QCOMPARE(removedAccount, QStringLiteral("tenant/client"));
    QCOMPARE(manager.sessionGeneration(), authenticatedGeneration + 1);

    transport.nextBody = QJsonDocument(QJsonObject{{"error", "invalid_grant"}})
                             .toJson(QJsonDocument::Compact);
    removedAccount.clear();
    manager.restoreSession();
    QVERIFY(!manager.authenticated());
    QCOMPARE(manager.status(), QStringLiteral("Microsoft sign-in must be renewed. Sign in again."));
    QCOMPARE(removedAccount, QStringLiteral("tenant/client"));
}

void MicrosoftOAuthManagerTest::qualifiesDeveloperProfileSecretsOnly()
{
    QString account;
    MicrosoftOAuthManager manager;
    manager.setSecretStore([&account](const QString &key, QString *) { account = key; return QString(); }, {}, {});
    manager.configure("tenant", "client");
    QCOMPARE(account, QStringLiteral("tenant/client"));

    MicrosoftOAuthManager developer;
    developer.setSecretNamespace("ProjectNotes-dev");
    developer.setSecretStore([&account](const QString &key, QString *) { account = key; return QString(); }, {}, {});
    developer.configure("tenant", "client");
    QCOMPARE(account, QStringLiteral("ProjectNotes-dev/tenant/client"));
}

QTEST_GUILESS_MAIN(MicrosoftOAuthManagerTest)
#include "tst_microsoftoauth.moc"
