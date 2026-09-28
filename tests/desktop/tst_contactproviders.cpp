// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QJsonDocument>
#include <QThread>
#include "ProjectNotesContacts/ContactProvider.h"
#include "ProjectNotesIntegrations/Office365Service.h"
#include "ProjectNotesIntegrations/MicrosoftOAuthManager.h"
using namespace PN::Contacts;
using namespace PN::Comm;
class RoutingTransport final : public HttpTransport {
public:
    QList<HttpRequest> requests;
    std::function<HttpResponse(const HttpRequest &)> route;
    void send(HttpRequest r,Completion done) override {requests<<r;auto response=route(r);response.operationId=r.operationId;done(response);}
    void cancel(const QUuid &) override {}
};
static HttpResponse json(QJsonObject o,int code=200){HttpResponse r;r.statusCode=code;r.body=QJsonDocument(o).toJson();return r;}
static HttpResponse davXml(QByteArray body,int code=207){HttpResponse r;r.statusCode=code;r.body=body;return r;}
class ContactProvidersTest : public QObject {
    Q_OBJECT
private slots:
    void graphRequestsCrossThreadWithoutMovingSharedAccount(){
        RoutingTransport transport;bool correctTransportThread=true;
        transport.route=[&](const HttpRequest&r){
            correctTransportThread&=QThread::currentThread()==thread();
            if(r.url.path().endsWith("/token"))return json({{"access_token","synthetic"},{"refresh_token","synthetic"},{"expires_in",3600},{"scope","offline_access User.Read Contacts.ReadWrite"}});
            if(r.url.path()=="/v1.0/me")return json({{"displayName","Test Account"},{"mail","test@example.test"}});
            if(r.method=="POST")return json({{"id","created"}},201);
            return json({{"value",QJsonArray{}}});
        };
        MicrosoftOAuthManager oauth;Office365Service office;office.setOAuthManager(&oauth);office.setHttpTransport(&transport);
        oauth.setSecretStore([](const QString&,QString*){return QString("refresh");},{},{});oauth.configure("tenant","client");oauth.restoreSession();
        auto account=office.account();QThread workerThread;
        auto *provider=new NativeContactProvider("office365",&office);provider->moveToThread(&workerThread);
        connect(&workerThread,&QThread::finished,provider,&QObject::deleteLater);workerThread.start();
        struct Stop {QThread &thread;~Stop(){thread.quit();thread.wait();}} stop{workerThread};
        bool finished=false,correctCallbackThread=false;Snapshot result;
        QMetaObject::invokeMethod(provider,[&,account]{
            provider->setOfficeAccount(account);provider->fetch({}, {},[&](Snapshot snapshot){
                bool onWorker=QThread::currentThread()==&workerThread;
                QMetaObject::invokeMethod(this,[&,snapshot,onWorker]{result=snapshot;correctCallbackThread=onWorker;finished=true;},Qt::QueuedConnection);
            });
        },Qt::QueuedConnection);
        QTRY_VERIFY(finished);QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QVERIFY(correctTransportThread);QVERIFY(correctCallbackThread);
        finished=false;QString error;Contact created;
        QMetaObject::invokeMethod(provider,[&]{Contact c;c.name="Jane Smith";provider->create({{"exportBookId","default"}},c,"intent",[&](Contact contact,QString message){
            bool onWorker=QThread::currentThread()==&workerThread;
            QMetaObject::invokeMethod(this,[&,contact,message,onWorker]{created=contact;error=message;correctCallbackThread=onWorker;finished=true;},Qt::QueuedConnection);
        });},Qt::QueuedConnection);
        QTRY_VERIFY(finished);QVERIFY2(error.isEmpty(),qPrintable(error));QCOMPARE(created.id,QString("created"));QVERIFY(correctCallbackThread);QVERIFY(correctTransportThread);
        QMetaObject::invokeMethod(provider,[provider]{provider->cancel();},Qt::BlockingQueuedConnection);
    }
    void graphPaginationDeltaAndDeletion(){
        RoutingTransport transport;int phase=0;
        transport.route=[&](const HttpRequest&r){
            if(r.url.path().endsWith("/token"))return json({{"access_token","synthetic"},{"refresh_token","synthetic"},{"expires_in",3600},{"scope","offline_access User.Read Contacts.ReadWrite"}});
            if(r.url.path()=="/v1.0/me")return json({{"displayName","Test Account"},{"mail","test@example.test"}});
            if(r.url.path()=="/v1.0/me/contactFolders")return json({{"value",QJsonArray{}}});
            if(phase==0&&r.url.query().isEmpty())return json({{"value",QJsonArray{QJsonObject{{"id","a"},{"displayName","Alice Smith"},{"emailAddresses",QJsonArray{QJsonObject{{"address","alice@example.test"}}}}}}},{"@odata.nextLink","https://graph.microsoft.com/v1.0/me/contacts/delta?page=2"}});
            if(phase==0)return json({{"value",QJsonArray{QJsonObject{{"id","b"},{"displayName","Bob Jones"}}}},{"@odata.deltaLink","https://graph.microsoft.com/v1.0/me/contacts/delta?token=1"}});
            return json({{"value",QJsonArray{QJsonObject{{"id","a"},{"displayName","Alice Smyth"}},QJsonObject{{"id","b"},{"@removed",QJsonObject{{"reason","deleted"}}}}}},{"@odata.deltaLink","https://graph.microsoft.com/v1.0/me/contacts/delta?token=2"}});
        };
        MicrosoftOAuthManager oauth;Office365Service office;office.setOAuthManager(&oauth);office.setHttpTransport(&transport);
        oauth.setSecretStore([](const QString&,QString*){return QString("synthetic-refresh");},{},{});oauth.configure("tenant","client");oauth.restoreSession();
        QVERIFY(office.authenticated());QVERIFY(!office.account().label.isEmpty());
        NativeContactProvider provider("office365",&office);Snapshot first;provider.fetch({}, {},[&](Snapshot s){first=s;});
        QVERIFY2(first.error.isEmpty(),qPrintable(first.error));QCOMPARE(first.contacts.size(),2);
        QCOMPARE(first.defaultExportBookId,QString("default"));
        phase=1;QJsonObject state{{"providerStates",QJsonObject{{"office365/"+first.account,first.cursors}}}};Snapshot second;
        provider.fetch({},state,[&](Snapshot s){second=s;});QVERIFY2(second.error.isEmpty(),qPrintable(second.error));QCOMPARE(second.contacts.size(),1);
        QCOMPARE(second.contacts.first().name,QString("Alice Smyth"));QCOMPARE(second.contacts.first().fields.value("email").toString(),QString("alice@example.test"));
        QVERIFY(transport.requests.last().url.query().contains("token=1"));
    }
    void carddavDiscoveryChangesAndExpiredToken(){
        RoutingTransport transport;int phase=0;bool expired=false;
        transport.route=[&](const HttpRequest&r){
            if(r.url.path()=="/")return davXml("<d:multistatus xmlns:d='DAV:'><d:response><d:propstat><d:prop><d:current-user-principal><d:href>/principal/</d:href></d:current-user-principal></d:prop></d:propstat></d:response></d:multistatus>");
            if(r.url.path()=="/principal/")return davXml("<d:multistatus xmlns:d='DAV:' xmlns:c='urn:ietf:params:xml:ns:carddav'><d:response><d:propstat><d:prop><c:addressbook-home-set><d:href>/books/</d:href></c:addressbook-home-set></d:prop></d:propstat></d:response></d:multistatus>");
            if(r.url.path()=="/books/")return davXml("<d:multistatus xmlns:d='DAV:' xmlns:c='urn:ietf:params:xml:ns:carddav'><d:response><d:href>/books/contacts/</d:href><d:propstat><d:prop><d:resourcetype><c:addressbook/></d:resourcetype><d:displayname>Contacts</d:displayname><d:sync-token>token1</d:sync-token></d:prop></d:propstat></d:response></d:multistatus>");
            if(phase==1&&!expired&&r.body.contains("sync-collection")){expired=true;return davXml("",409);}
            if(phase==2)return davXml("<d:multistatus xmlns:d='DAV:'><d:response><d:href>/books/contacts/a.vcf</d:href><d:status>HTTP/1.1 404 Not Found</d:status></d:response><d:sync-token>token3</d:sync-token></d:multistatus>");
            return davXml("<d:multistatus xmlns:d='DAV:' xmlns:c='urn:ietf:params:xml:ns:carddav'><d:response><d:href>/books/contacts/a.vcf</d:href><d:propstat><d:prop><c:address-data>BEGIN:VCARD\r\nVERSION:3.0\r\nFN:Alice Smith\r\nEMAIL:alice@example.test\r\nEND:VCARD\r\n</c:address-data></d:prop></d:propstat></d:response><d:sync-token>token2</d:sync-token></d:multistatus>");
        };
        NativeContactProvider provider("icloud",nullptr);provider.setCardDAVTransportForTesting(&transport,[]{return QString("synthetic-password");});
        Snapshot first;QVariantMap config{{"account","test@example.test"}};provider.fetch(config,{},[&](Snapshot s){first=s;});
        QVERIFY2(first.error.isEmpty(),qPrintable(first.error));QCOMPARE(first.contacts.size(),1);QCOMPARE(first.contacts.first().name,QString("Alice Smith"));
        phase=1;Snapshot second;provider.fetch(config,{{"providerStates",QJsonObject{{"icloud/"+first.account,first.cursors}}}},[&](Snapshot s){second=s;});
        QVERIFY2(second.error.isEmpty(),qPrintable(second.error));QVERIFY(expired);QCOMPARE(second.contacts.size(),1);
        phase=2;Snapshot third;provider.fetch(config,{{"providerStates",QJsonObject{{"icloud/"+second.account,second.cursors}}}},[&](Snapshot s){third=s;});
        QVERIFY2(third.error.isEmpty(),qPrintable(third.error));QCOMPARE(third.contacts.size(),0);
        QVERIFY(transport.requests.last().manualRedirects);
    }
    void carddavRejectsUnsafeRedirectAndAuthentication(){
        RoutingTransport transport;transport.route=[](const HttpRequest&){auto r=davXml("",302);r.headers["Location"]="https://evil.example.test/";return r;};
        NativeContactProvider provider("icloud",nullptr);provider.setCardDAVTransportForTesting(&transport,[]{return QString("synthetic-password");});
        Snapshot result;provider.fetch({{"account","test@example.test"}}, {},[&](Snapshot s){result=s;});QVERIFY(!result.error.isEmpty());QCOMPARE(transport.requests.size(),1);
        transport.route=[](const HttpRequest&){return davXml("",401);};provider.fetch({{"account","test@example.test"}}, {},[&](Snapshot s){result=s;});QVERIFY(result.authenticationError);
    }
    void carddavCreateUsesConditionalWrite(){
        RoutingTransport transport;transport.route=[](const HttpRequest&){return davXml("",201);};NativeContactProvider provider("icloud",nullptr);provider.setCardDAVTransportForTesting(&transport,[]{return QString("synthetic-password");});
        Contact c;c.name="Alice Smith";QString error;Contact result;provider.create({{"account","test@example.test"},{"exportBookId","https://contacts.icloud.com/books/"}},c,"intent",[&](Contact v,QString e){result=v;error=e;});
        QVERIFY(error.isEmpty());QCOMPARE(result.id,QString("https://contacts.icloud.com/books/intent.vcf"));QCOMPARE(transport.requests.first().headers.value("If-None-Match"),QByteArray("*"));
    }
};
QTEST_GUILESS_MAIN(ContactProvidersTest)
#include "tst_contactproviders.moc"
