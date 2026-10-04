// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include <QSqlQuery>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <memory>
#include "databaseobjects.h"
#include "ProjectNotesContacts/ContactMatcher.h"
#include "ProjectNotesContacts/ContactRepository.h"
#include "ProjectNotesContacts/ContactSyncSettingsModel.h"
#include "ProjectNotesContacts/VCard.h"
#include "ProjectNotesContacts/ThunderbirdBridge.h"
using namespace PN::Contacts;

class FakeContacts final : public ContactProvider {
public:
    Snapshot snapshot;
    int creates = 0;
    int updates = 0;
    QString createError;
    QString lastExportBook;
    bool deferred = false;
    Completion pending;
    void fetch(const QVariantMap &,const QJsonObject &,Completion done) override {
        if(deferred)pending=std::move(done);else done(snapshot);
    }
    void create(const QVariantMap &config,const Contact &contact,const QString &,Created done) override {
        ++creates;auto c=contact;c.id="exported";c.book=config.value("exportBookId").toString();lastExportBook=c.book;
        if(createError.isEmpty())snapshot.contacts<<c;done(c,createError);
    }
    void update(const QVariantMap &,const Contact &existing,const Contact &contact,Updated done) override {
        ++updates;auto c=contact;c.id=existing.id;c.book=existing.book;
        for(auto &current:snapshot.contacts)if(current.id==existing.id&&current.book==existing.book){current=c;break;}
        done(c,{});
    }
    void cancel() override {} // Deliberately delivers stale completions to test generation guards.
};
static Contact incoming(QString name="Jane Smith",QString email="jane@example.test",QString company="Acme Inc.") {
    Contact c;c.id="remote";c.book="contacts";c.name=name;c.company=company;
    if(!email.isEmpty())c.emails<<email;c.phones={"+1 212 555 1234"};
    c.fields={{"name",name},{"email",email},{"company",company},{"office_phone","+1 212 555 1234"},{"cell_phone",""},{"role","Manager"}};
    return c;
}
static Person existing(QString id="person",QString name="Jane Smith",QString email="jane@example.test") {
    Person p;p.id=id;p.name=name;p.clientId="client";p.company="Acme Corporation";
    p.fields={{"name",name},{"email",email},{"client_id","client"},{"office_phone","12125551234"},{"cell_phone",""},{"role",""}};return p;
}
class ContactsTest : public QObject {
    Q_OBJECT
private:
    QTemporaryDir directory;
    int count(const QString &table){QSqlQuery q(global_DBObjects.getDb());q.exec("SELECT count(*) FROM "+table+" WHERE deleted=0");q.next();return q.value(0).toInt();}
    void seed(){QSqlQuery q(global_DBObjects.getDb());QVERIFY(q.exec("INSERT INTO clients(id,client_name) VALUES('client','Acme Corporation')"));QVERIFY(q.exec("INSERT INTO people(id,name,email,office_phone,client_id) VALUES('person','Jane Smith','jane@example.test','12125551234','client')"));}
    void configure(ContactSyncSettingsModel &model,FakeContacts &provider){
        provider.snapshot.account="test-account";provider.snapshot.books={QVariantMap{{"id","contacts"},{"name","Contacts"},{"writable",true}}};
        provider.snapshot.defaultExportBookId="contacts";
        model.setProviderForTesting(&provider);model.setDatabase(global_DBObjects.getDatabaseFile());model.setOption("startup",false);model.setOption("enabled",true);
    }
private slots:
    void initTestCase(){
        QVERIFY(directory.isValid());QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory.path());
        QVERIFY(global_DBObjects.createDatabase(directory.filePath("contacts.db")));
        QVERIFY(global_DBObjects.openDatabase(directory.filePath("contacts.db"),"contacts-test",false));
    }
    void init(){QSqlQuery q(global_DBObjects.getDb());QVERIFY(q.exec("DELETE FROM people"));QVERIFY(q.exec("DELETE FROM clients"));}
    void cleanupTestCase(){global_DBObjects.closeDatabase();}
    void sharedIntegrationSelectsContactProvider_data(){
        QTest::addColumn<QString>("backend");QTest::addColumn<QString>("provider");
        QTest::newRow("default")<<QString("default-app")<<QString("none");
        QTest::newRow("graph")<<QString("graph")<<QString("office365");
        QTest::newRow("new-outlook")<<QString("outlook")<<QString("office365");
        QTest::newRow("classic-outlook")<<QString("outlook-classic")<<QString("outlook");
        QTest::newRow("thunderbird")<<QString("thunderbird")<<QString("thunderbird");
        QTest::newRow("mac")<<QString("mac-mail")<<QString("icloud");
    }
    void sharedIntegrationSelectsContactProvider(){
        QFETCH(QString,backend);QFETCH(QString,provider);QTemporaryDir state;
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());
        model.setIntegrationBackend(backend);model.setDatabase(global_DBObjects.getDatabaseFile());
        QCOMPARE(model.configuration().value("provider").toString(),provider);
        model.setOption("sourceBooks",QStringList{"old-book"});
        model.setIntegrationBackend(backend=="thunderbird"?"graph":"thunderbird");
        QTRY_COMPARE(model.configuration().value("provider").toString(),backend=="thunderbird"?QString("office365"):QString("thunderbird"));
        QTRY_VERIFY(model.configuration().value("sourceBooks").toStringList().isEmpty());
        model.setIntegrationBackend("default-app");QTRY_COMPARE(model.configuration().value("provider").toString(),QString("none"));
        model.syncNow();QTRY_VERIFY(!model.busy());QVERIFY(model.status().contains("Choose an application"));
        model.setDatabase(global_DBObjects.getDatabaseFile());QCOMPARE(model.configuration().value("provider").toString(),QString("none"));
    }
    void thunderbirdSetupValidatesBundledFiles(){
        QTemporaryDir root;const QString app=root.filePath("app");QDir().mkpath(app);
        auto result=installThunderbirdBridge(root.path(),app,root.filePath("manifests"));
        QVERIFY(result.error.contains("native host"));
        QFile helper(app+"/ProjectNotesThunderbirdContacts");
#ifdef Q_OS_WIN
        helper.setFileName(helper.fileName()+".exe");
#endif
        QVERIFY(helper.open(QIODevice::WriteOnly));helper.write("fixture");helper.close();
        QVERIFY(helper.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
        result=installThunderbirdBridge(root.path(),app,root.filePath("manifests"));
        QVERIFY(result.error.contains("add-on"));
        QVERIFY(!QFileInfo::exists(root.filePath("manifests/org.projectnotes.contacts.json")));
    }
    void thunderbirdSetupRegistersDesktopAndFlatpak(){
#ifndef Q_OS_LINUX
        QSKIP("Flatpak is Linux-only");
#else
        QTemporaryDir root;const QString app=root.filePath("app with ' quotes");QDir().mkpath(app);
        QFile helper(app+"/ProjectNotesThunderbirdContacts");QVERIFY(helper.open(QIODevice::WriteOnly));helper.write("fixture");helper.close();
        QVERIFY(helper.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
        QFile addon(app+"/projectnotes-contacts.xpi");QVERIFY(addon.open(QIODevice::WriteOnly));addon.write("addon fixture");addon.close();
        for(const auto &id:QStringList{"org.mozilla.thunderbird","org.mozilla.Thunderbird"})QVERIFY(QDir().mkpath(root.filePath(".var/app/"+id)));
        const auto native=root.filePath("native");
        auto result=installThunderbirdBridge(root.path(),app,native);
        QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QCOMPARE(result.flatpakIds.size(),2);
        auto read=[](const QString &path){QFile file(path);if(!file.open(QIODevice::ReadOnly))return QByteArray{};return file.readAll();};
        QCOMPARE(QJsonDocument::fromJson(read(native+"/org.projectnotes.contacts.json")).object()["path"].toString(),helper.fileName());
        for(const auto &id:result.flatpakIds){
            const auto dir=root.filePath(".var/app/"+id+"/.mozilla/native-messaging-hosts");
            const auto wrapper=dir+"/projectnotes-contacts-host.sh";
            QCOMPARE(QJsonDocument::fromJson(read(dir+"/org.projectnotes.contacts.json")).object()["path"].toString(),wrapper);
            QVERIFY(QFileInfo(wrapper).isExecutable());
            const auto script=read(wrapper);QVERIFY(script.contains("flatpak-spawn --host"));QVERIFY(script.contains("'\\''"));QVERIFY(script.endsWith(" \"$@\"\n"));
            QCOMPARE(read(root.filePath(".var/app/"+id+"/data/projectnotes-contacts.xpi")),QByteArray("addon fixture"));
        }
        // Repeated setup replaces only our files and also supports a packaged
        // Project Notes helper whose /app path is not visible on the host.
        result=installThunderbirdBridge(root.path(),app,native,"com.projectnotespro.ProjectNotes");
        QVERIFY2(result.error.isEmpty(),qPrintable(result.error));
        const auto nativeScript=read(native+"/projectnotes-contacts-host.sh");
        QVERIFY(nativeScript.contains("flatpak run --command=ProjectNotesThunderbirdContacts 'com.projectnotespro.ProjectNotes'"));
        QVERIFY(read(root.filePath(".var/app/org.mozilla.thunderbird/.mozilla/native-messaging-hosts/projectnotes-contacts-host.sh")).contains("flatpak-spawn --host /usr/bin/flatpak run"));
#endif
    }
    void companyNormalization(){
        QCOMPARE(ContactMatcher::normalize("  ACME, Inc. ",true),QString("acme"));
        QCOMPARE(ContactMatcher::normalize("Acme Company",true),QString("acme"));
        QCOMPARE(ContactMatcher::normalize("Acme LLP",true),QString("acme"));
        auto m=ContactMatcher::company("Acme LLC",{{"client","Acme Corporation",false}});QCOMPARE(m.id,QString("client"));
        m=ContactMatcher::company("Acme Inc",{{"a","Acme LLC",false},{"b","Acme Ltd",false}});QVERIFY(m.review);QVERIFY(m.id.isEmpty());
    }
    void obviousCompanyTypoMatchesAutomatically(){
        auto m=ContactMatcher::company("Consolidated Grain and Barg",{{"client","Consolidated Grain and Barge",false}});
        QCOMPARE(m.id,QString("client"));QVERIFY(!m.review);
    }
    void companyConjunctionNormalization(){
        const QString base="Consolidated Grain & Barge";
        QCOMPARE(ContactMatcher::normalize(base,true),QString("consolidated grain and barge"));
        QCOMPARE(ContactMatcher::normalize(base),QString("consolidated grain barge"));
        auto m=ContactMatcher::company("  Consolidated Grain&Barge, Inc.  ",{{"client","CONSOLIDATED GRAIN AND BARGE LLC",false}});
        QCOMPARE(m.id,QString("client"));QVERIFY(!m.review);
        QCOMPARE(m.candidates.first().score,1.0);QVERIFY(m.candidates.first().strong);
    }
    void companyLocationNeedsReview_data(){
        QTest::addColumn<QString>("branch");
        QTest::addColumn<bool>("reverse");
        for(const auto &dash:QStringList{"-",QString(QChar(0x2013)),QString(QChar(0x2014))}){
            const auto branch="CONSOLIDATED GRAIN AND BARGE "+dash+" MOUNT VERNON";
            QTest::newRow(qPrintable(dash+" forward"))<<branch<<false;
            QTest::newRow(qPrintable(dash+" reverse"))<<branch<<true;
        }
        QTest::newRow("legal suffix")<<QString("CONSOLIDATED GRAIN AND BARGE LLC - MOUNT VERNON")<<false;
    }
    void companyLocationNeedsReview(){
        QFETCH(QString,branch);QFETCH(bool,reverse);
        const QString base="Consolidated Grain & Barge";
        auto m=ContactMatcher::company(reverse?branch:base,{{"client",reverse?base:branch,false}});
        QVERIFY(m.review);QVERIFY(m.id.isEmpty());QCOMPARE(m.candidates.size(),1);
        QCOMPARE(m.candidates.first().id,QString("client"));QVERIFY(!m.candidates.first().strong);
        QCOMPARE(m.candidates.first().score,.90);QVERIFY(m.candidates.first().reason.contains("branch/location"));
    }
    void companyLocationCandidatesPreserveIdentity(){
        const QString base="Consolidated Grain & Barge";
        const QString branch="CONSOLIDATED GRAIN AND BARGE - MOUNT VERNON";
        QList<Client> clients={{"a",branch,false},{"b","CONSOLIDATED GRAIN AND BARGE - NEW ORLEANS",false},{"deleted",branch,true}};
        auto m=ContactMatcher::company(base,clients);
        QVERIFY(m.review);QVERIFY(m.id.isEmpty());QCOMPARE(m.candidates.size(),2);
        for(const auto &candidate:m.candidates)QVERIFY(!candidate.strong);
        m=ContactMatcher::company(branch,clients);QCOMPARE(m.id,QString("a"));
        clients.append({"base","CONSOLIDATED GRAIN AND BARGE",false});
        QCOMPARE(ContactMatcher::company(base,clients).id,QString("base"));
        QVERIFY(ContactMatcher::normalize(branch,true)!=ContactMatcher::normalize(clients[1].name,true));
        QVERIFY(ContactMatcher::company(base,{{"deleted",branch,true}}).candidates.isEmpty());
        QVERIFY(ContactMatcher::company(base,{{"other","Consolidated Grain Services - Mount Vernon",false}}).candidates.isEmpty());
        QVERIFY(ContactMatcher::company(base,{{"other","CONSOLIDATED GRAIN AND BARGE MOUNT VERNON",false}}).candidates.isEmpty());
        QVERIFY(ContactMatcher::company("Acme",{{"other","Acme - Mount Vernon",false}}).candidates.isEmpty());
        QVERIFY(ContactMatcher::company("A B",{{"other","A B - Mount Vernon",false}}).candidates.isEmpty());
        m=ContactMatcher::company("Acme Tools",{{"other","Acme Tools - ",false}});
        QCOMPARE(m.candidates.size(),1);QCOMPARE(m.candidates.first().reason,QString("Company name similarity"));
        // Preserve ordinary fuzzy scores above the branch-candidate floor.
        m=ContactMatcher::company(base,{{"other",base+" - X",false}});
        QVERIFY(m.review);QCOMPARE(m.candidates.size(),1);
        QVERIFY(m.candidates.first().score>.90);QVERIFY(!m.candidates.first().strong);
    }
    void exactEmailWithTypo(){auto m=ContactMatcher::person(incoming("Jane Smth"),"client",{existing()});QCOMPARE(m.id,QString("person"));}
    void reorderedAndAccentNames(){
        auto c=incoming("Smith, Jane");QCOMPARE(ContactMatcher::person(c,"client",{existing()}).id,QString("person"));
        c=incoming("José Martin");auto p=existing("p","Jose Martin");QCOMPARE(ContactMatcher::person(c,"client",{p}).id,QString("p"));
    }
    void nameOnlyNeedsReview(){auto c=incoming();c.emails.clear();c.phones.clear();auto m=ContactMatcher::person(c,"client",{existing()});QVERIFY(m.review);QVERIFY(m.id.isEmpty());}
    void exactThunderbirdFieldsPreferExactPerson(){
        auto c=incoming(" Jane  Smith "," jane@example.test "," Acme Corporation ");
        c.fields={{"name",c.name},{"company",c.company},{"email"," jane@example.test "},
                  {"office_phone"," 212 555 1234 "},{"cell_phone"," "},{"role"," Manager "}};
        c.phones={" 212 555 1234 "};
        auto p=existing();p.fields["office_phone"]="212 555 1234";p.fields["role"]="Manager";
        auto similar=existing("similar");similar.name="Jane Smith";similar.fields["role"]="Director";
        auto m=ContactMatcher::person(c,"client",{similar,p});
        QCOMPARE(m.id,QString("person"));QVERIFY(!m.review);
    }
    void exactThunderbirdFieldsWithoutIdentifiers(){
        auto c=incoming();c.emails.clear();c.phones.clear();c.fields["email"]="";c.fields["office_phone"]="";
        auto p=existing();p.company="Acme Inc.";p.fields["email"]="";p.fields["office_phone"]="";p.fields["role"]="Manager";
        QCOMPARE(ContactMatcher::person(c,"client",{p}).id,QString("person"));
        auto duplicate=p;duplicate.id="duplicate";
        QVERIFY(ContactMatcher::person(c,"client",{p,duplicate}).review);
    }
    void fuzzyPhoneAndCompany(){auto c=incoming("Jacqueline Smithe","");auto m=ContactMatcher::person(c,"client",{existing("person","Jacqueline Smith")});QCOMPARE(m.id,QString("person"));}
    void sharedCompanyPhoneDoesNotMatchDifferentPerson(){
        auto m=ContactMatcher::person(incoming("John Smith","john@example.test"),"client",{existing()});
        QVERIFY(m.candidates.isEmpty());QVERIFY(m.id.isEmpty());QVERIFY(!m.review);
    }
    void sharedIdentifiersDoNotMatchUnrelatedNames(){
        auto m=ContactMatcher::person(incoming("Robert Jones","jane@example.test","Different Business"),"other",{existing()});
        QVERIFY(m.candidates.isEmpty());QVERIFY(m.id.isEmpty());QVERIFY(!m.review);
    }
    void sharedIdentifiersNeedReview(){auto p=existing("p2","Jane Smithe");auto m=ContactMatcher::person(incoming(),"client",{existing(),p});QVERIFY(m.review);}
    void contradictoryCompanyNeedsReview(){auto c=incoming();auto m=ContactMatcher::person(c,"other",{existing()});QVERIFY(m.review);c.company="Different Business";QVERIFY(ContactMatcher::person(c,"",{existing()}).review);}
    void uniqueNameDoesNotProveIdentity(){auto c=incoming();c.emails={"different@example.test"};auto m=ContactMatcher::person(c,"client",{existing()});QVERIFY(m.review);}
    void rejectedAndDeletedExcluded(){auto p=existing();QVERIFY(ContactMatcher::person(incoming(),"client",{p},{p.id}).candidates.isEmpty());p.deleted=true;QVERIFY(ContactMatcher::person(incoming(),"client",{p}).candidates.isEmpty());}
    void vcardRoundTrip(){auto c=incoming("Jane; Smith");c.company="Acme; Division";c.fields["role"]="Director, Research";auto parsed=parseVCard(QString::fromUtf8(writeVCard(c,"uid")),"remote","contacts");QCOMPARE(parsed.name,c.name);QCOMPARE(parsed.company,c.company);QCOMPARE(parsed.fields.value("role"),c.fields.value("role"));}
    void omittedVcardFieldsStayOmitted(){auto c=parseVCard("BEGIN:VCARD\r\nVERSION:3.0\r\nN:Smith;Jane;;;\r\nEND:VCARD\r\n","id","book");QCOMPARE(c.name,QString("Jane Smith"));QVERIFY(!c.fields.contains("email"));}
    void atomicWriteAndStaleSnapshot(){
        seed();ContactRepository repo(&global_DBObjects);QString error;auto p=existing();p.fields["role"]="";
        QVERIFY(repo.apply(incoming(),p.id,p.clientId,"",p.name,p.fields,error));
        QVERIFY(!repo.apply(incoming(),p.id,p.clientId,"",p.name,p.fields,error));QVERIFY(error.contains("changed"));
    }
    void rollbackNewCompanyOnNameCollision(){
        seed();ContactRepository repo(&global_DBObjects);QString error;auto c=incoming();c.company="New Company";
        QVERIFY(!repo.apply(c,"different","","new-client",c.name,{},error));QCOMPARE(count("clients"),1);QCOMPARE(count("people"),1);
    }
    void companyCandidateRecheckedUnderWriteLock(){
        seed();ContactRepository repo(&global_DBObjects);auto c=incoming("Different Person","different@example.test","Acme LLC");QString error;
        QVERIFY(!repo.apply(c,"different","","new-client",c.name,{},error));QCOMPARE(count("clients"),1);QCOMPARE(count("people"),1);
    }
    void companyLocationCandidateRecheckedUnderWriteLock(){
        QSqlQuery q(global_DBObjects.getDb());
        QVERIFY(q.exec("INSERT INTO clients(id,client_name) VALUES('client','CONSOLIDATED GRAIN AND BARGE - MOUNT VERNON')"));
        ContactRepository repo(&global_DBObjects);QString error;
        auto c=incoming("Different Person","different@example.test","Consolidated Grain & Barge");
        QVERIFY(!repo.apply(c,"different","","new-client",c.name,{},error));
        QVERIFY(error.contains("matching company"));QCOMPARE(count("clients"),1);QCOMPARE(count("people"),0);
    }
    void explicitEmptyClearsButOmissionPreserves(){
        seed();ContactRepository repo(&global_DBObjects);QString error;auto c=incoming();
        c.fields.remove("office_phone");c.fields["email"]="";
        QVERIFY(repo.apply(c,"person","client","",c.name,{},error,true,true));
        QSqlQuery q(global_DBObjects.getDb());QVERIFY(q.exec("SELECT email,office_phone FROM people WHERE id='person'"));QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(),QString());QCOMPARE(q.value(1).toString(),QString("12125551234"));
    }
    void sameRunCompanyVariantsReuseOneClient(){
        QTemporaryDir state;FakeContacts provider;auto a=incoming("Alice One","alice@example.test","Acme Inc");auto b=incoming("Brian Two","brian@example.test","Acme LLC");b.id="remote-two";
        b.phones={"+1 212 555 5678"};b.fields["office_phone"]="+1 212 555 5678";
        provider.snapshot.contacts={a,b};ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(!model.busy());
        QCOMPARE(count("clients"),1);QCOMPARE(count("people"),2);
    }
    void equivalentCompanyVariantsShareOneReview(){
        seed();QSqlQuery q(global_DBObjects.getDb());
        QVERIFY(q.exec("UPDATE clients SET client_name='CONSOLIDATED GRAIN AND BARGE - MOUNT VERNON'"));
        QTemporaryDir state;FakeContacts provider;
        auto a=incoming("Alice One","alice@example.test","Consolidated Grain & Barge LLP");
        auto b=incoming("Brian Two","brian@example.test","Consolidated Grain and Barge Company");b.id="remote-two";
        a.phones.clear();a.fields["office_phone"]="";b.phones.clear();b.fields["office_phone"]="";provider.snapshot.contacts={a,b};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(model.reviews().size(),1);
        const auto review=model.reviews().first().toMap();QCOMPARE(review.value("stage").toString(),QString("client"));
        model.resolve(review.value("key").toString(),"clientUseRight","","client","");
        QTRY_VERIFY(!model.busy());QTRY_VERIFY(model.reviews().isEmpty());QCOMPARE(count("clients"),1);QCOMPARE(count("people"),3);
    }
    void sharedCompanyPhoneImportsDifferentPerson(){
        seed();QTemporaryDir state;FakeContacts provider;
        provider.snapshot.contacts={incoming("John Smith","john@example.test")};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(!model.busy());
        QCOMPARE(count("clients"),1);QCOMPARE(count("people"),2);QVERIFY(model.reviews().isEmpty());
    }
    void sharedIdentifiersImportUnrelatedPerson(){
        seed();QTemporaryDir state;FakeContacts provider;
        provider.snapshot.contacts={incoming("Robert Jones","jane@example.test","Different Business")};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(!model.busy());
        QCOMPARE(count("clients"),2);QCOMPARE(count("people"),2);QVERIFY(model.reviews().isEmpty());
    }
    void companyLocationImportWaitsForReview(){
        QSqlQuery q(global_DBObjects.getDb());
        QVERIFY(q.exec("INSERT INTO clients(id,client_name) VALUES('client','CONSOLIDATED GRAIN AND BARGE - MOUNT VERNON')"));
        QTemporaryDir state;FakeContacts provider;
        provider.snapshot.contacts={incoming("Jane Smith","jane@example.test","Consolidated Grain & Barge")};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.syncNow();QTRY_VERIFY(!model.busy());
        QCOMPARE(count("clients"),1);QCOMPARE(count("people"),0);QCOMPARE(model.reviews().size(),1);
        const auto review=model.reviews().first().toMap();
        const auto clients=review.value("clients").toList();QCOMPARE(clients.size(),1);
        QCOMPARE(clients.first().toMap().value("id").toString(),QString("client"));
        QVERIFY(review.value("selectedClientId").toString().isEmpty());
        QCOMPARE(review.value("stage").toString(),QString("client"));
        model.resolve(review.value("key").toString(),"clientUseRight","","client","");
        QTRY_COMPARE(model.reviews().size(),0);QCOMPARE(count("clients"),1);QCOMPARE(count("people"),1);
        QVERIFY(q.exec("SELECT client_id FROM people"));QVERIFY(q.next());QCOMPARE(q.value(0).toString(),QString("client"));q.finish();
        model.syncNow();QTRY_VERIFY(!model.busy());QVERIFY(model.reviews().isEmpty());
        QCOMPARE(count("clients"),1);QCOMPARE(count("people"),1);
    }
    void clientsResolvedBeforeContacts_data(){
        QTest::addColumn<bool>("separate");
        QTest::newRow("same-client")<<false;
        QTest::newRow("separate-client")<<true;
    }
    void clientsResolvedBeforeContacts(){
        QFETCH(bool,separate);seed();QTemporaryDir state;FakeContacts provider;
        QSqlQuery rename(global_DBObjects.getDb());QVERIFY(rename.exec("UPDATE clients SET client_name='Consolidated Grain & Barge'"));
        auto c=incoming("Jane Smith","other@example.test","Consolidated Grain & Barge - Boston");
        auto b=incoming("Brian Two","brian@example.test",c.company);b.id="remote-two";
        b.phones.clear();b.fields["office_phone"]="";provider.snapshot.contacts={c,b};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.syncNow();QTRY_VERIFY(!model.busy());
        QCOMPARE(count("people"),1);QCOMPARE(count("clients"),1);QCOMPARE(model.reviews().size(),1);
        auto review=model.reviews().first().toMap();QCOMPARE(review.value("stage").toString(),QString("client"));
        QVERIFY(review.value("people").toList().isEmpty());
        model.resolve(review.value("key").toString(),"useLeft","person","","");
        QTest::qWait(30);QCOMPARE(count("people"),1);QCOMPARE(model.reviews().size(),1);
        model.resolve(review.value("key").toString(),separate?"clientUseBoth":"clientUseRight","","client","");
        QTRY_COMPARE(count("people"),2);QTRY_VERIFY(!model.busy());
        QCOMPARE(count("clients"),separate?2:1);QCOMPARE(model.reviews().size(),1);
        review=model.reviews().first().toMap();QVERIFY(review.value("stage").toString()!="client");
        auto chosen=review.value("selectedClientId").toString();QVERIFY(!chosen.isEmpty());
        QCOMPARE(chosen=="client",!separate);
        model.resolve(review.value("key").toString(),"useBoth","","","");
        QTRY_COMPARE(model.reviews().size(),0);QCOMPARE(count("people"),3);
        QSqlQuery q(global_DBObjects.getDb());QVERIFY(q.exec("SELECT client_id FROM people WHERE email='other@example.test'"));QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(),chosen);q.finish();
        model.setDatabase(global_DBObjects.getDatabaseFile());
        model.syncNow();QTRY_VERIFY(!model.busy());QVERIFY(model.reviews().isEmpty());
        QCOMPARE(count("people"),3);QCOMPARE(count("clients"),separate?2:1);
    }
    void clientSideChoices_data(){
        QTest::addColumn<QString>("action");QTest::addColumn<int>("clientCount");QTest::addColumn<QString>("existingName");
        QTest::newRow("left")<<QString("clientUseLeft")<<1<<QString("Consolidated Grain & Barge");
        QTest::newRow("right")<<QString("clientUseRight")<<1<<QString("CONSOLIDATED GRAIN AND BARGE - MOUNT VERNON");
        QTest::newRow("both")<<QString("clientUseBoth")<<2<<QString("CONSOLIDATED GRAIN AND BARGE - MOUNT VERNON");
    }
    void clientSideChoices(){
        QFETCH(QString,action);QFETCH(int,clientCount);QFETCH(QString,existingName);
        seed();QSqlQuery q(global_DBObjects.getDb());
        QVERIFY(q.exec("UPDATE clients SET client_name='CONSOLIDATED GRAIN AND BARGE - MOUNT VERNON'"));
        QTemporaryDir state;FakeContacts provider;auto c=incoming("Brian Two","brian@example.test","  Consolidated Grain & Barge  ");
        c.phones.clear();c.fields["office_phone"]="";provider.snapshot.contacts={c};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(model.reviews().size(),1);
        const auto review=model.reviews().first().toMap();QCOMPARE(review.value("stage").toString(),QString("client"));
        model.resolve(review.value("key").toString(),action,"","client","");
        QTRY_VERIFY(!model.busy());QTRY_VERIFY(model.reviews().isEmpty());QCOMPARE(count("clients"),clientCount);QCOMPARE(count("people"),2);
        QVERIFY(q.exec("SELECT client_name FROM clients WHERE id='client'"));QVERIFY(q.next());QCOMPARE(q.value(0).toString(),existingName);q.finish();
        if(action=="clientUseBoth"){
            QVERIFY(q.exec("SELECT client_name FROM clients WHERE id<>'client' AND deleted=0"));QVERIFY(q.next());
            QCOMPARE(q.value(0).toString(),QString("Consolidated Grain & Barge"));
        }
    }
    void allClientChoicesRequiredBeforeContactMatching(){
        seed();QSqlQuery q(global_DBObjects.getDb());
        QVERIFY(q.exec("UPDATE clients SET client_name='Consolidated Grain & Barge'"));
        QTemporaryDir state;FakeContacts provider;
        auto a=incoming("Jane Smith","jane@example.test","Consolidated Grain & Barge - Boston");
        auto b=incoming("Brian Two","brian@example.test","Consolidated Grain & Barge - Chicago");
        b.id="remote-two";b.phones.clear();b.fields["office_phone"]="";provider.snapshot.contacts={a,b};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(model.reviews().size(),2);
        model.resolve(model.reviews().first().toMap().value("key").toString(),"clientUseRight","","client","");
        QTRY_COMPARE(model.reviews().size(),1);QCOMPARE(count("people"),1);QVERIFY(!model.busy());
        model.setDatabase(global_DBObjects.getDatabaseFile());QCOMPARE(model.reviews().size(),1);
        model.resolve(model.reviews().first().toMap().value("key").toString(),"clientUseRight","","client","");
        QTRY_COMPARE(count("people"),2);QTRY_VERIFY(!model.busy());QVERIFY(model.reviews().isEmpty());
        QCOMPARE(count("clients"),1);
    }
    void ignoredAndRejectedDecisionsPersist(){
        seed();QTemporaryDir state;FakeContacts provider;auto c=incoming();c.emails.clear();c.phones.clear();provider.snapshot.contacts={c};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(!model.busy());
        auto key=model.reviews().first().toMap().value("key").toString();model.rejectCandidate(key,"person");model.syncNow();QTRY_VERIFY(!model.busy());
        QVERIFY(model.reviews().first().toMap().value("people").toList().isEmpty());
        model.resolve(key,"ignore","","","");model.syncNow();QTRY_VERIFY(!model.busy());QVERIFY(model.reviews().isEmpty());QCOMPARE(count("people"),1);
    }
    void repeatImportUsesIds(){
        QTemporaryDir state;FakeContacts provider;provider.snapshot.contacts={incoming()};ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.syncNow();QTRY_VERIFY(!model.busy());QVERIFY(!model.busy());QCOMPARE(count("people"),1);QCOMPARE(count("clients"),1);
        provider.snapshot.contacts[0].name="Jane Smithe";provider.snapshot.contacts[0].fields["name"]="Jane Smithe";model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(count("people"),1);QCOMPARE(count("clients"),1);
        QSqlQuery q(global_DBObjects.getDb());QVERIFY(q.exec("SELECT name FROM people"));QVERIFY(q.next());QCOMPARE(q.value(0).toString(),QString("Jane Smithe"));
    }
    void staleReviewCannotRecreateRemovedPerson(){
        seed();QTemporaryDir state;FakeContacts provider;auto c=incoming();c.emails.clear();c.phones.clear();provider.snapshot.contacts={c};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(!model.busy());
        auto key=model.reviews().first().toMap().value("key").toString();QSqlQuery q(global_DBObjects.getDb());QVERIFY(q.exec("DELETE FROM people WHERE id='person'"));
        model.resolve(key,"link","person","client","Jane Smith");QTRY_VERIFY(model.status().contains("no longer exists"));QCOMPARE(count("people"),0);QCOMPARE(model.reviews().size(),1);
    }
    void ambiguousImportWaitsForReview(){
        seed();QTemporaryDir state;FakeContacts provider;auto c=incoming();c.emails.clear();c.phones.clear();provider.snapshot.contacts={c};ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(!model.busy());
        QCOMPARE(count("people"),1);QCOMPARE(model.reviews().size(),1);
        auto key=model.reviews().first().toMap().value("key").toString();model.resolve(key,"link","person","client","Jane Smith");QTRY_COMPARE(model.reviews().size(),0);model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(count("people"),1);QCOMPARE(model.reviews().size(),0);
    }
    void reviewUniqueDisplayNamePersists(){
        seed();QTemporaryDir state;FakeContacts provider;auto c=incoming();c.emails={"other@example.test"};c.fields["email"]="other@example.test";provider.snapshot.contacts={c};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(model.reviews().size(),1);
        auto key=model.reviews().first().toMap().value("key").toString();model.resolve(key,"create","","client","Jane Smith (Acme)");QTRY_COMPARE(model.reviews().size(),0);QCOMPARE(count("people"),2);model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(count("people"),2);QCOMPARE(model.reviews().size(),0);
    }
    void sideChoicesUseContactDetails_data(){
        QTest::addColumn<QString>("action");
        QTest::newRow("left")<<QString("useLeft");
        QTest::newRow("right")<<QString("ignore");
        QTest::newRow("both")<<QString("useBoth");
    }
    void sideChoicesUseContactDetails(){
        QFETCH(QString,action);seed();QTemporaryDir state;FakeContacts provider;
        auto c=incoming("Jane Smith","other@example.test","New Company");
        c.fields["cell_phone"]="5551234567";provider.snapshot.contacts={c};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(model.reviews().size(),1);
        auto review=model.reviews().first().toMap();
        auto right=review.value("people").toList().first().toMap().value("contact").toMap();
        QCOMPARE(right.value("name").toString(),QString("Jane Smith"));
        QCOMPARE(right.value("company").toString(),QString("Acme Corporation"));
        QCOMPARE(right.value("fields").toMap().value("email").toString(),QString("jane@example.test"));
        QCOMPARE(right.value("fields").toMap().value("office_phone").toString(),QString("12125551234"));
        model.resolve(review.value("key").toString(),action,"person","","");
        QTRY_VERIFY2(model.reviews().isEmpty(), qPrintable(model.reviews().isEmpty()?model.status():model.reviews().first().toMap().value("reason").toString()));QCOMPARE(count("people"),action=="useBoth"?2:1);
        QSqlQuery q(global_DBObjects.getDb());
        QVERIFY(q.exec("SELECT p.name,p.email,p.office_phone,p.cell_phone,c.client_name FROM people p LEFT JOIN clients c ON p.client_id=c.id WHERE p.email='other@example.test'"));
        if(action=="ignore")QVERIFY(!q.next());
        else {QVERIFY(q.next());QCOMPARE(q.value(0).toString(),action=="useBoth"?QString("Jane Smith (New Company)"):c.name);QCOMPARE(q.value(2).toString(),c.fields.value("office_phone").toString());QCOMPARE(q.value(3).toString(),QString("5551234567"));QCOMPARE(q.value(4).toString(),c.company);}
        q.finish();
        if(action!="useLeft"){
            QVERIFY(q.exec("SELECT email,client_id FROM people WHERE id='person'"));QVERIFY(q.next());
            QCOMPARE(q.value(0).toString(),QString("jane@example.test"));QCOMPARE(q.value(1).toString(),QString("client"));q.finish();
        }
        provider.snapshot.contacts[0].fields["cell_phone"]="5557654321";
        model.syncNow();QTRY_VERIFY(!model.busy());QVERIFY(model.reviews().isEmpty());QCOMPARE(count("people"),action=="useBoth"?2:1);
        if(action!="ignore"){
            QVERIFY(q.exec("SELECT cell_phone FROM people WHERE email='other@example.test'"));QVERIFY(q.next());QCOMPARE(q.value(0).toString(),QString("5557654321"));
        }
    }
    void stateSurvivesRestart(){
        QTemporaryDir state;FakeContacts provider;provider.snapshot.contacts={incoming()};{
            ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(!model.busy());
        }ContactSyncSettingsModel restarted(&global_DBObjects,nullptr,"ContactTest",state.path());configure(restarted,provider);restarted.syncNow();QTRY_VERIFY(!restarted.busy());QCOMPARE(count("people"),1);
    }
    void linkedDeletionNeverRestores(){
        QTemporaryDir state;FakeContacts provider;provider.snapshot.contacts={incoming()};ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(!model.busy());
        QSqlQuery q(global_DBObjects.getDb());QVERIFY(q.exec("UPDATE people SET deleted=1"));model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(count("people"),0);
    }
    void automaticExportBookSelection(){
        QVariantList books={QVariantMap{{"id","z"},{"writable",true}},QVariantMap{{"id","a"},{"writable",true}},QVariantMap{{"id","readonly"},{"writable",false}}};
        QCOMPARE(exportBook(books),QString("a"));QCOMPARE(exportBook(books,"z"),QString("z"));QCOMPARE(exportBook(books,"readonly"),QString("a"));
        std::reverse(books.begin(),books.end());QCOMPARE(exportBook(books),QString("a"));QCOMPARE(exportBook(books,"z"),QString("z"));
        QCOMPARE(exportBook({QVariantMap{{"id","readonly"},{"writable",false}}}),QString());
        QCOMPARE(exportBook(books,"readonly",true),QString());
        QCOMPARE(exportBook(books,"missing-default",true),QString());
    }
    void synchronizationNeverExports(){
        seed();QTemporaryDir state;FakeContacts provider;ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.setOption("export",true);model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(provider.creates,0);
        QVERIFY(!model.configuration().contains("export"));
    }
    void legacyDestinationDoesNotOverrideAutomaticRouting(){
        seed();QTemporaryDir state;FakeContacts provider;
        {
            ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        }
        QDir directory(state.path()+"/contacts");auto paths=directory.entryList({"*.json"},QDir::Files);QCOMPARE(paths.size(),1);
        QFile file(directory.filePath(paths.first()));QVERIFY(file.open(QIODevice::ReadOnly));auto journal=QJsonDocument::fromJson(file.readAll()).object();file.close();
        auto config=journal["configuration"].toObject();config["destination"]="obsolete-folder";config["export"]=true;journal["configuration"]=config;
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));auto bytes=QJsonDocument(journal).toJson();QCOMPARE(file.write(bytes),bytes.size());file.close();
        ContactSyncSettingsModel restarted(&global_DBObjects,nullptr,"ContactTest",state.path());restarted.setProviderForTesting(&provider);restarted.setDatabase(global_DBObjects.getDatabaseFile());
        QVERIFY(!restarted.configuration().contains("export"));QVERIFY(!restarted.configuration().contains("destination"));restarted.syncNow();QTRY_VERIFY(!restarted.busy());
        QCOMPARE(provider.creates,0);
    }
    void fullExportRequiresWritableBookButNotSyncEnabled(){
        QTemporaryDir state;FakeContacts provider;provider.snapshot.contacts={incoming()};ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.setOption("enabled",false);provider.snapshot.books={QVariantMap{{"id","contacts"},{"writable",false}}};
        QSignalSpy result(&model,&ContactSyncSettingsModel::fullExportFinished);
        model.fullExport();QTRY_VERIFY(!model.busy());QCOMPARE(count("people"),0);QCOMPARE(provider.creates,0);QVERIFY(model.status().contains("no writable address book"));
        QTRY_COMPARE(result.size(),1);QVERIFY(!result.first().at(1).toBool());
    }
    void fullExportReviewsExistingContactsAndSupportsAllActions(){
        seed();QTemporaryDir state;FakeContacts provider;provider.snapshot.contacts={incoming()};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        QSignalSpy result(&model,&ContactSyncSettingsModel::fullExportFinished);
        model.setOption("enabled",false);model.fullExport();QTRY_COMPARE(model.exportReviews().size(),1);QVERIFY(model.busy());
        QCOMPARE(provider.creates,0);model.resolveExport("person","update","remote");QTRY_VERIFY(!model.busy());
        QCOMPARE(provider.updates,1);QCOMPARE(provider.creates,0);QTRY_COMPARE(result.size(),1);QVERIFY(result.first().at(1).toBool());
        QVERIFY(result.first().at(0).toString().contains("1 updated"));
        provider.snapshot.contacts[0].fields["role"]="Changed externally";
        model.fullExport();QTRY_COMPARE(model.exportReviews().size(),1);model.resolveExport("person","skip","");QTRY_VERIFY(!model.busy());
        QCOMPARE(provider.updates,1);QCOMPARE(provider.creates,0);QVERIFY(model.status().contains("1 skipped"));
        model.fullExport();QTRY_COMPARE(model.exportReviews().size(),1);model.resolveExport("person","create","");QTRY_VERIFY(!model.busy());
        QCOMPARE(provider.creates,1);QCOMPARE(provider.lastExportBook,QString("contacts"));QVERIFY(model.status().contains("1 created"));
    }
    void fullExportSkipsIdenticalContactsWithoutReview(){
        seed();QTemporaryDir state;FakeContacts provider;auto same=incoming("Jane Smith","jane@example.test","Acme Corporation");
        same.fields["company"]="Acme Corporation";same.fields["role"]="";provider.snapshot.contacts={same};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.fullExport();QTRY_VERIFY(!model.busy());QVERIFY(model.exportReviews().isEmpty());
        QCOMPARE(provider.creates,0);QCOMPARE(provider.updates,0);QVERIFY(model.status().contains("1 skipped"));

        // An automatic identical match becomes a durable export link. A later
        // external change must therefore be reviewed rather than silently skipped.
        provider.snapshot.contacts[0].fields["role"]="Changed externally";
        model.fullExport();QTRY_COMPARE(model.exportReviews().size(),1);
        const auto candidates=model.exportReviews().first().toMap().value("candidates").toList();QCOMPARE(candidates.size(),1);
        QCOMPARE(candidates.first().toMap().value("reason").toString(),QString("Previously exported contact"));
        model.resolveExport("person","skip","remote");QTRY_VERIFY(!model.busy());
    }
    void fullExportIgnoresSharedIdentifiersWithUnrelatedName(){
        seed();QTemporaryDir state;FakeContacts provider;
        provider.snapshot.contacts={incoming("Robert Jones","jane@example.test","Acme Corporation")};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        model.fullExport();QTRY_VERIFY(!model.busy());QVERIFY(model.exportReviews().isEmpty());
        QCOMPARE(provider.creates,1);QCOMPARE(provider.updates,0);
    }
    void disconnectedFullExportReportsFailure(){
        seed();QTemporaryDir state;FakeContacts provider;provider.snapshot.error="Thunderbird bridge is disconnected.";
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        QSignalSpy result(&model,&ContactSyncSettingsModel::fullExportFinished);
        model.fullExport();QTRY_COMPARE(result.size(),1);
        QVERIFY(!result.first().at(1).toBool());QVERIFY(result.first().at(0).toString().contains("disconnected"));
        QCOMPARE(provider.creates,0);QVERIFY(!model.busy());
    }
    void uncertainFullExportStopsWithoutAutomaticRetry(){
        seed();QTemporaryDir state;FakeContacts provider;provider.createError="Connection lost after create";ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.fullExport();QTRY_VERIFY(!model.busy());QCOMPARE(provider.creates,1);QVERIFY(model.status().contains("may be uncertain"));
    }
    void staleCompletionIsIgnored(){
        QTemporaryDir state;FakeContacts provider;provider.deferred=true;provider.snapshot.contacts={incoming()};ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QVERIFY(model.busy());QTRY_VERIFY(bool(provider.pending));model.setOption("provider","icloud");provider.pending(provider.snapshot);QCOMPARE(count("people"),0);QVERIFY(!model.busy());
    }
    void databaseWaitDoesNotBlockUiThread(){
        QTemporaryDir state;FakeContacts provider;provider.snapshot.contacts={incoming()};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);
        QCOMPARE(model.thread(),QThread::currentThread());
        int ticks=0;QTimer heartbeat;heartbeat.setInterval(5);connect(&heartbeat,&QTimer::timeout,this,[&]{++ticks;});heartbeat.start();
        bool released=false;db_rwlock.lockForWrite();
        QTimer::singleShot(100,&model,[&]{db_rwlock.unlock();released=true;});
        model.syncNow();QTRY_VERIFY(released);QTRY_VERIFY(!model.busy());
        QVERIFY(ticks>0);QCOMPARE(count("people"),1);
    }
    void databaseSwitchDiscardsPendingResults(){
        QTemporaryDir state;FakeContacts provider;provider.deferred=true;provider.snapshot.contacts={incoming()};
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(bool(provider.pending));
        auto completion=provider.pending;model.setDatabase({});completion(provider.snapshot);QTest::qWait(30);
        QVERIFY(!model.busy());QCOMPARE(count("people"),0);QVERIFY(model.reviews().isEmpty());
    }
    void destructionDuringPendingFetchIsSafe(){
        QTemporaryDir state;FakeContacts provider;provider.deferred=true;provider.snapshot.contacts={incoming()};
        auto model=std::make_unique<ContactSyncSettingsModel>(&global_DBObjects,nullptr,"ContactTest",state.path());configure(*model,provider);model->syncNow();QTRY_VERIFY(bool(provider.pending));
        auto completion=provider.pending;model.reset();completion(provider.snapshot);QTest::qWait(30);QCOMPARE(count("people"),0);
    }
    void incompleteSnapshotsNeverWrite(){
        QTemporaryDir state;FakeContacts provider;provider.snapshot.contacts={incoming()};provider.snapshot.complete=false;ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ContactTest",state.path());configure(model,provider);model.syncNow();QTRY_VERIFY(!model.busy());QCOMPARE(count("people"),0);QVERIFY(model.status().contains("incomplete"));
    }
};
QTEST_GUILESS_MAIN(ContactsTest)
#include "tst_contacts.moc"
