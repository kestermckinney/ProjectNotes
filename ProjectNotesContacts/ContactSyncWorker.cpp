// SPDX-License-Identifier: GPL-3.0-only
#include "ContactSyncWorker.h"
#include "ContactMatcher.h"
#include "credentialstore.h"
#include "ProjectNotesIntegrations/Office365Service.h"
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QSettings>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDateTime>
#include <QSet>
#include <QElapsedTimer>
namespace PN::Contacts {
static QString hash(const QString&s){return QString::fromLatin1(QCryptographicHash::hash(s.toUtf8(),QCryptographicHash::Sha256).toHex());}
static QString clientKey(const QString &scope,const QString &name){return "client/"+hash(scope+QChar(0)+name.trimmed().toCaseFolded());}
Match ContactSyncWorker::companyMatch(const QString &name,const QList<Client> &clients) const {
    const auto decision=m_journal["clientDecisions"].toObject()[clientKey(m_scope,name)].toObject();
    const auto id=decision["clientId"].toString();
    for(const auto &client:clients)if(!client.deleted&&client.id==id){Match match;match.id=id;return match;}
    return ContactMatcher::company(name,clients);
}
bool ContactSyncWorker::prepareClientReviews(){
    auto entries=m_journal["reviews"].toObject();
    for(auto it=entries.begin();it!=entries.end();) {
        if(it.value().toObject()["scope"].toString()==m_scope&&it.value().toObject()["stage"].toString()=="client")it=entries.erase(it);
        else ++it;
    }
    const auto books=m_config.value("sourceBooks").toStringList();int pending=0;
    for(const auto &contact:m_snapshot.contacts){
        if((!books.isEmpty()&&!books.contains(contact.book))||contact.company.trimmed().isEmpty()||
           m_journal["ignored"].toObject()[sourceKey(m_scope,contact)].toBool())continue;
        const auto match=companyMatch(contact.company,m_clients);if(!match.review)continue;
        const auto key=clientKey(m_scope,contact.company);if(entries.contains(key))continue;
        QJsonArray candidates;for(const auto &candidate:match.candidates)candidates<<QJsonObject{
            {"id",candidate.id},{"label",candidate.label},{"reason",candidate.reason},{"score",candidate.score}};
        entries[key]=QJsonObject{{"stage","client"},{"company",contact.company},{"clients",candidates},
            {"scope",m_scope},{"reason","Possible duplicate client; choose an existing client or keep this company separate."}};
        ++pending;
    }
    m_journal["reviews"]=entries;
    if(!persist())return false;
    if(pending){m_status=QString("Resolve %1 client matches before reviewing contact duplicates.").arg(pending);return false;}
    return true;
}
ContactSyncWorker::ContactSyncWorker(PN::Comm::Office365Service*office,QString org,QString root,QObject*parent)
    :QObject(parent),m_office(office),m_organization(std::move(org)),m_root(std::move(root)) {
    m_timer.setParent(this);
    connect(&m_timer,&QTimer::timeout,this,[this]{if(!m_authFailed)syncNow();});
    m_repository.onChanged=[this]{emit recordsChanged(m_databasePath);};
}
void ContactSyncWorker::setOfficeAccount(PN::Comm::Office365Account account,bool inProgress) {
    m_account=std::move(account);
    if(auto *provider=dynamic_cast<NativeContactProvider*>(m_provider))provider->setOfficeAccount(m_account);
    if(m_busy&&m_config.value("provider")=="office365"&&m_account.generation!=m_officeGeneration){cancel();m_status="Office 365 account changed; synchronization canceled.";schedule();emit changed();}
    if(m_authFailed&&m_config.value("provider")=="office365"&&m_account.authenticated&&!inProgress){m_authFailed=false;schedule();}
    emit changed();
}
ContactSyncWorker::~ContactSyncWorker(){cancel();}
QVariantList ContactSyncWorker::reviews() const {
    QVariantList out;auto entries=m_journal["reviews"].toObject();
    bool clientStage=false;
    for(auto it=entries.begin();it!=entries.end();++it)if(it.value().toObject()["stage"].toString()=="client"&&it.value().toObject()["scope"]==m_journal["activeScope"])clientStage=true;
    QList<Person> people;QList<Client> clients;QString error;
    if(!entries.isEmpty()&&m_repository.isOpen())m_repository.read(people,clients,error);
    for(auto it=entries.begin();it!=entries.end();++it){
        auto entry=it.value().toObject().toVariantMap();entry["key"]=it.key();
        if(clientStage&&(entry.value("stage")!="client"||entry.value("scope").toString()!=m_journal["activeScope"].toString()))continue;
        QVariantList candidates;
        for(const auto &value:entry.value("people").toList()){
            auto candidate=value.toMap();
            for(const auto &person:people)if(person.id==candidate.value("id").toString()&&!person.deleted){
                candidate["contact"]=QVariantMap{{"name",person.name},{"company",person.company},{"fields",person.fields}};
                candidates<<candidate;break;
            }
        }
        entry["people"]=candidates;out<<entry;
    }
    return out;
}
QString ContactSyncWorker::scope(const QString &account)const{return m_config.value("provider").toString()+"/"+account;}
void ContactSyncWorker::cancel(){++m_generation;m_timer.stop();if(m_provider)m_provider->cancel();m_busy=false;emit changed();}
void ContactSyncWorker::cancelSync(){cancel();m_status="Contact synchronization canceled.";schedule();emit changed();}
void ContactSyncWorker::setDatabase(const QString &path){
    cancel();m_repository.close();m_databasePath=path;m_path.clear();m_journal={};m_books.clear();m_authFailed=false;
    m_config={{"provider","office365"},{"enabled",false},{"startup",true},{"interval",5},{"export",false},{"account",""},{"sourceBooks",QStringList{}}};
    m_config["credentialService"]=m_organization=="ProjectNotes"?QString("Project Notes iCloud Contacts"):QString("Project Notes iCloud Contacts/")+m_organization;
    if(path.isEmpty()){m_status="Open a database to configure contact synchronization.";emit changed();return;}
    QString databaseError;
    if(!m_repository.open(path,databaseError)){m_status=databaseError;emit changed();return;}
    QString canonical=QFileInfo(path).canonicalFilePath();if(canonical.isEmpty())canonical=QFileInfo(path).absoluteFilePath();
    m_path=m_root+"/contacts/"+hash(canonical)+".json";QFile f(m_path);
    if(f.exists()){
        if(!f.open(QIODevice::ReadOnly)){m_status="Cannot read contact state; sync is paused.";m_path.clear();emit changed();return;}
        QJsonParseError e;auto document=QJsonDocument::fromJson(f.readAll(),&e);
        if(e.error!=QJsonParseError::NoError||!document.isObject()||document.object()["version"].toInt()!=1){m_status="Contact state is damaged or incompatible; sync is paused. Restore the state file before syncing.";m_path.clear();emit changed();return;}
        m_journal=document.object();auto stored=m_journal["configuration"].toObject().toVariantMap();for(auto it=stored.begin();it!=stored.end();++it)m_config[it.key()]=it.value();
        m_books=m_journal["books"].toArray().toVariantList();
    }else {
        QSettings legacy(m_organization,"PluginSettings");legacy.setFallbacksEnabled(false);
        QString account=legacy.value("iCloud Contacts/Account").toString();
        if(!account.isEmpty()){m_config["provider"]="icloud";m_config["account"]=account;m_config["export"]=legacy.value("iCloud Contacts/ExportNewContacts",false).toBool();}
        else if(legacy.value("Outlook Integration/IntegrationType").toString()=="Outlook Application")m_config["provider"]="outlook";
        m_journal["version"]=1;m_status="Settings migrated where available. Confirm the source and enable native sync.";
        if(!account.isEmpty()){
            QString legacyId=hash(QFileInfo(path).absoluteFilePath()+QChar(0)+account.toLower());
            QString cache=QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
            if(m_organization.startsWith("ProjectNotes")&&m_organization!="ProjectNotes")cache+="/"+m_organization.mid(12);
            QFile old(cache+"/icloud_contacts_"+legacyId+".json");
            if(old.open(QIODevice::ReadOnly)){auto state=QJsonDocument::fromJson(old.readAll()).object();m_journal["legacyICloudLinks"]=state["links"].toObject();m_journal["legacyICloudAccount"]=account.trimmed().toCaseFolded();}
            if(m_organization!="ProjectNotes"){
                QString service=m_config.value("credentialService").toString();
                if(CredentialStore::read(service,account).isEmpty()){
                    auto secret=CredentialStore::read("Project Notes iCloud Contacts",account);
                    if(!secret.isEmpty())CredentialStore::write(service,account,secret);
                }
            }
        }
        if(!persist())return;
    }
    // Export routing is now resolved from the current provider snapshot.
    m_config.remove("destination");
    // Socket name is per OS user; access is further restricted by QLocalServer.
    m_config["bridgeSocket"]="projectnotes-contacts-"+hash(QDir::homePath()).left(20);
    schedule();emit changed();
    if(m_config.value("enabled").toBool()&&m_config.value("startup").toBool())QTimer::singleShot(0,this,&ContactSyncWorker::syncNow);
}
bool ContactSyncWorker::persist(){
    if(m_path.isEmpty()){m_status="Contact state is unavailable. Open a database with valid contact state.";emit changed();return false;}
    m_journal["version"]=1;m_journal["configuration"]=QJsonObject::fromVariantMap(m_config);m_journal["books"]=QJsonArray::fromVariantList(m_books);
    if(!QDir().mkpath(QFileInfo(m_path).absolutePath())){m_status="Cannot create contact state directory.";emit changed();return false;}
    QFile::setPermissions(QFileInfo(m_path).absolutePath(),QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner);
    QSaveFile file(m_path);const auto data=QJsonDocument(m_journal).toJson();
    if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit()){
        m_status="Cannot save contact state. Sync paused to prevent duplicate contacts.";m_timer.stop();m_config["enabled"]=false;emit changed();return false;
    }QFile::setPermissions(m_path,QFileDevice::ReadOwner|QFileDevice::WriteOwner);return true;
}
void ContactSyncWorker::schedule(){m_timer.stop();if(m_config.value("enabled").toBool()&&!m_authFailed&&!m_path.isEmpty()&&m_config.value("provider")!="none")m_timer.start(qBound(1,m_config.value("interval").toInt(),1440)*60000);}
void ContactSyncWorker::setOption(const QString&key,const QVariant&value){
    if(!QStringList{"provider","enabled","startup","interval","export","account","sourceBooks"}.contains(key))return;
    if(key=="provider"&&!QStringList{"outlook","office365","thunderbird","icloud","none"}.contains(value.toString()))return;
    cancel();m_authFailed=false;
    if(key=="provider"&&value!=m_config.value(key)){m_config["sourceBooks"]=QStringList{};m_books.clear();}
    if(key=="account"&&value!=m_config.value(key)){m_config["sourceBooks"]=QStringList{};m_books.clear();}
    if((key=="provider"||key=="account")&&value!=m_config.value(key))m_journal["activeScope"]="";
    m_config[key]=key=="interval"?QVariant(qBound(1,value.toInt(),1440)):value;
    if(!persist())return;
    if(m_config.value("enabled").toBool()){
        QSettings legacy(m_organization,"PluginSettings");legacy.setValue("Native Contacts/Enabled",true);
        legacy.setValue("iCloud Contacts/Enabled",false);legacy.setValue("Outlook Integration/ImportContacts",false);legacy.setValue("Outlook Integration/ExportContacts",false);legacy.sync();
    }
    schedule();emit changed();
}
void ContactSyncWorker::savePassword(const QString&password){
    if(password.isEmpty())return;QString error;
    if(m_config.value("account").toString().trimmed().isEmpty()){m_status="Enter the Apple Account first.";emit changed();return;}
    if(!CredentialStore::write(m_config.value("credentialService").toString(),m_config.value("account").toString().trimmed(),password,&error))m_status="Cannot store iCloud password: "+error;
    else {m_authFailed=false;m_status="App-specific password saved in the operating system credential store.";schedule();}emit changed();
}
void ContactSyncWorker::syncNow(){run(false);}
void ContactSyncWorker::testConnection(){run(true);}
void ContactSyncWorker::run(bool test){
    if(m_busy)return;
    if(m_config.value("provider")=="none"){complete("Choose an application in Email, Contact and Calendar Integration to synchronize contacts.");return;}
    if(m_path.isEmpty()||!m_repository.isOpen()){m_status="Open a database first.";emit changed();return;}
    if(!test&&!m_config.value("enabled").toBool()){m_status="Enable contact synchronization first.";emit changed();return;}
    if(m_provider&&m_provider!=m_testProvider){m_provider->cancel();m_provider->deleteLater();}
    m_provider=m_testProvider?m_testProvider:new NativeContactProvider(m_config.value("provider").toString(),m_office,this);
    if(auto *provider=dynamic_cast<NativeContactProvider*>(m_provider))provider->setOfficeAccount(m_account);
    m_busy=true;m_imported=0;m_exported=0;m_unchanged=0;m_status=test?"Testing contact connection…":"Synchronizing contacts…";emit changed();auto gen=++m_generation;
    m_officeGeneration=m_account.generation;
    m_provider->fetch(m_config,m_journal,[this,guard=QPointer<ContactSyncWorker>(this),gen,test](Snapshot s){if(guard&&gen==m_generation)receive(std::move(s),test);});
}
void ContactSyncWorker::addReview(const QString&key,const Contact&contact,const Match&people,const Match&clients,const QString&reason){
    QJsonArray ps,cs;for(const auto&c:people.candidates)ps<<QJsonObject{{"id",c.id},{"label",c.label},{"reason",c.reason},{"score",c.score}};
    for(const auto&c:clients.candidates)cs<<QJsonObject{{"id",c.id},{"label",c.label},{"reason",c.reason},{"score",c.score}};
    auto reviews=m_journal["reviews"].toObject();reviews[key]=QJsonObject{{"contact",encode(contact)},{"people",ps},{"clients",cs},{"selectedClientId",clients.id},{"reason",reason},{"scope",m_scope}};m_journal["reviews"]=reviews;
}
bool ContactSyncWorker::apply(Contact c,const QString&key,QString personId,QString clientId,QString displayName,bool review,bool keepBoth){
    QList<Person> people;QList<Client> clients;QString error;
    if(!m_repository.read(people,clients,error)){m_status=error;return false;}
    auto links=m_journal["links"].toObject();auto intent=m_journal["imports"].toObject();auto saved=intent[key].toObject();
    QVariantMap expected;bool foundPerson=false;
    for(const auto&p:people)if(p.id==personId){foundPerson=true;if(p.deleted){m_status="Linked person was deleted; skipped.";return true;}expected=p.fields;}
    if(!personId.isEmpty()&&!foundPerson){m_status="The selected person no longer exists. Refresh synchronization before reviewing this contact.";emit changed();return false;}
    if(displayName.isEmpty())displayName=c.name;
    if(keepBoth&&personId.isEmpty()){
        // The database requires unique names. Derive a name from the chosen
        // contact automatically, and persist it for subsequent synchronization.
        auto used=[&](const QString &name){for(const auto &person:people)if(ContactMatcher::normalize(person.name)==ContactMatcher::normalize(name))return true;return false;};
        if(used(displayName)){
            const auto base=displayName+" ("+(c.company.isEmpty()?m_scope.section('/',0,0):c.company)+")";
            displayName=base;int suffix=2;
            while(used(displayName))displayName=base+QString(" %1").arg(suffix++);
        }
    }
    if(personId.isEmpty())personId=saved["personId"].toString();if(personId.isEmpty())personId=QUuid::createUuid().toString();
    QString newClientId;
    if(!c.company.isEmpty()&&clientId.isEmpty()){
        auto cm=companyMatch(c.company,clients);
        if(cm.review&&!review){addReview(key,c,{},cm,"Company match needs review.");return true;}
        if(!review)clientId=cm.id;if(clientId.isEmpty()){newClientId=saved["clientId"].toString();if(newClientId.isEmpty())newClientId=QUuid::createUuid().toString();}
    }
    intent[key]=QJsonObject{{"personId",personId},{"clientId",newClientId}};m_journal["imports"]=intent;if(!persist())return false;
    // Re-read identity for journal recovery: the previous attempt may have committed.
    for(const auto&p:people)if(p.id==personId && expected.isEmpty())expected=p.fields;
    // A durable intent alone is not an existing identity: a previous write may
    // have rolled back. Keep fuzzy revalidation enabled until that UUID exists.
    bool recoveredIdentity=false;for(const auto&p:people)if(p.id==personId)recoveredIdentity=true;
    bool knownIdentity=!links[key].toString().isEmpty()||(!saved["personId"].toString().isEmpty()&&recoveredIdentity);
    if(!m_repository.apply(c,personId,clientId,newClientId,displayName,expected,error,review,knownIdentity)){
        addReview(key,c,ContactMatcher::person(c,clientId,people,m_journal["rejected"].toObject()[key].toVariant().toStringList()),ContactMatcher::company(c.company,clients),error);persist();return true;
    }
    links[key]=personId;m_journal["links"]=links;intent.remove(key);m_journal["imports"]=intent;
    if(review){auto decisions=m_journal["decisions"].toObject();decisions[key]=QJsonObject{{"company",c.company},{"clientId",clientId.isEmpty()?newClientId:clientId},{"externalName",c.name},{"displayName",displayName}};m_journal["decisions"]=decisions;}
    auto pending=m_journal["reviews"].toObject();pending.remove(key);m_journal["reviews"]=pending;
    ++m_imported;
    m_people.clear();m_clients.clear();if(!m_repository.read(m_people,m_clients,error)){m_status=error;return false;}
    return persist();
}
void ContactSyncWorker::receive(Snapshot snapshot,bool test){
    if(!snapshot.error.isEmpty()){m_authFailed=snapshot.authenticationError;complete(snapshot.error);return;}
    if(!snapshot.complete){complete("The contact source returned incomplete data; no changes applied.");return;}
    m_authFailed=false;
    if(snapshot.account.isEmpty()){complete("Contact provider did not return an account identity.");return;}
    auto provider=m_config.value("provider").toString();
    snapshot.defaultExportBookId=exportBook(snapshot.books,snapshot.defaultExportBookId,provider=="office365"||provider=="outlook");
    m_books=snapshot.books;m_snapshot=std::move(snapshot);m_scope=scope(m_snapshot.account);
    m_journal["activeScope"]=m_scope;
    if(test){persist();complete(QString("Connected. Found %1 address books and %2 contacts.").arg(m_books.size()).arg(m_snapshot.contacts.size()));return;}
    QString error;m_people.clear();m_clients.clear();if(!m_repository.read(m_people,m_clients,error)){complete(error);return;}
    if(!prepareClientReviews()){complete(m_status);return;}
    m_importIndex=0;importNext();
}
void ContactSyncWorker::importNext(){
    QElapsedTimer slice;slice.start();
    QString error;QStringList sourceBooks=m_config.value("sourceBooks").toStringList();QSet<QString> present;
    while(m_importIndex<m_snapshot.contacts.size()){
        if(slice.elapsed()>=10){auto generation=m_generation;QTimer::singleShot(0,this,[this,generation]{if(generation==m_generation)importNext();});return;}
        const auto c=m_snapshot.contacts[m_importIndex++];
        if(!sourceBooks.isEmpty()&&!sourceBooks.contains(c.book))continue;
        QString key=sourceKey(m_scope,c);present.insert(key);if(m_journal["ignored"].toObject()[key].toBool())continue;
        if(c.id.isEmpty()||c.name.trimmed().isEmpty()){addReview(key,c,{}, {},"Contact is missing a name or stable identity.");continue;}
        QString linked=m_journal["links"].toObject()[key].toString();
        if(linked.isEmpty()&&m_config.value("provider")=="icloud"&&m_journal["legacyICloudAccount"].toString()==m_snapshot.account){
            auto legacy=m_journal["legacyICloudLinks"].toObject()[hash(c.id)].toString();
            for(const auto&p:m_people)if(p.id==legacy){linked=legacy;break;}
        }
        if(!linked.isEmpty()){
            bool active=false;for(const auto&p:m_people)if(p.id==linked&&!p.deleted)active=true;
            if(!active)continue;
        }
        auto cm=companyMatch(c.company,m_clients);
        auto decision=m_journal["decisions"].toObject()[key].toObject();
        if(!m_journal["clientDecisions"].toObject().contains(clientKey(m_scope,c.company))&&decision["company"].toString()==c.company && !decision["clientId"].toString().isEmpty()){
            for(const auto &client:m_clients)if(!client.deleted&&client.id==decision["clientId"].toString()){cm.id=client.id;cm.review=false;break;}
        }
        auto pm=linked.isEmpty()?ContactMatcher::person(c,cm.id,m_people,m_journal["rejected"].toObject()[key].toVariant().toStringList()):Match{};
        if(cm.review){prepareClientReviews();complete(m_status);return;}
        if(linked.isEmpty()&&pm.review){addReview(key,c,pm,cm,"Possible duplicate; choose a match before importing.");continue;}
        QString displayName=decision["externalName"].toString()==c.name && !decision["displayName"].toString().isEmpty()?decision["displayName"].toString():c.name;
        if(!linked.isEmpty()&&!m_journal["reviews"].toObject().contains(key)){
            bool unchanged=false;
            for(const auto&p:m_people)if(p.id==linked){auto desired=p.fields;
                for(const auto&field:QStringList{"name","email","office_phone","cell_phone","role"})if(c.fields.contains(field))desired[field]=c.fields[field];
                desired["name"]=displayName;if(c.fields.contains("company"))desired["client_id"]=cm.id;
                unchanged=desired==p.fields && (c.company.isEmpty()||!cm.id.isEmpty());break;
            }
            if(unchanged){++m_unchanged;continue;}
        }
        if(!apply(c,key,linked.isEmpty()?pm.id:linked,cm.id,displayName,false)){complete(m_status);return;}
        auto generation=m_generation;
        QTimer::singleShot(0,this,[this,generation]{if(generation==m_generation)importNext();});
        return;
    }
    for(const auto&c:m_snapshot.contacts)if(sourceBooks.isEmpty()||sourceBooks.contains(c.book))present.insert(sourceKey(m_scope,c));
    if(m_snapshot.complete){
        auto scopes=m_journal["seen"].toObject();auto old=scopes[m_scope].toObject();auto suppressed=m_journal["suppressed"].toObject();
        for(auto it=old.begin();it!=old.end();++it)if(!present.contains(it.key())&&(sourceBooks.isEmpty()||sourceBooks.contains(it.value().toString()))){
            QString person=m_journal["links"].toObject()[it.key()].toString();if(!person.isEmpty())suppressed[m_scope+"/"+person]=true;
        }
        QJsonObject current;for(const auto&c:m_snapshot.contacts)if(present.contains(sourceKey(m_scope,c)))current[sourceKey(m_scope,c)]=c.book;
        scopes[m_scope]=current;m_journal["seen"]=scopes;m_journal["suppressed"]=suppressed;
    }
    auto providerStates=m_journal["providerStates"].toObject();providerStates[m_scope]=m_snapshot.cursors;m_journal["providerStates"]=providerStates;
    if(!persist()){complete(m_status);return;}
    m_exportIds.clear();m_people.clear();m_clients.clear();if(!m_repository.read(m_people,m_clients,error)){complete(error);return;}
    if(m_config.value("export").toBool()){
        if(m_snapshot.defaultExportBookId.isEmpty()){complete(QString("Applied %1 contacts; export unavailable: no writable address book is available for automatic export.").arg(m_imported));return;}
        for(const auto&p:m_people)if(!p.deleted)m_exportIds<<p.id;
    }exportNext();
}
void ContactSyncWorker::exportNext(){
    while(!m_exportIds.isEmpty()){
        QString id=m_exportIds.takeFirst();Person person;for(const auto&p:m_people)if(p.id==id)person=p;
        if(m_journal["suppressed"].toObject()[m_scope+"/"+id].toBool())continue;
        Contact contact;contact.name=person.name;contact.company=person.company;contact.fields=person.fields;contact.fields["company"]=person.company;contact.fields["id"]=person.id;
        contact.emails={person.fields.value("email").toString()};contact.phones={person.fields.value("office_phone").toString(),person.fields.value("cell_phone").toString()};
        bool found=false,uncertain=false;QString foundKey;
        for(const auto&remote:m_snapshot.contacts){
            QString key=sourceKey(m_scope,remote);if(m_journal["links"].toObject()[key].toString()==id){found=true;foundKey=key;break;}
            auto match=ContactMatcher::person(remote,person.clientId,QList<Person>{person});
            if(match.id==id){found=true;foundKey=key;break;}if(match.review)uncertain=true;
        }
        auto pending=m_journal["reviews"].toObject();for(auto it=pending.begin();it!=pending.end();++it)if(!it.key().startsWith("export/")&&it.value().toObject()["scope"].toString()==m_scope) {
            auto entry=it.value().toObject();auto incoming=decode(entry["contact"].toObject());if(ContactMatcher::person(incoming,person.clientId,QList<Person>{person}).review)uncertain=true;
        }
        auto intents=m_journal["exports"].toObject();QString intentKey=m_scope+"/"+id;
        if(found){auto links=m_journal["links"].toObject();bool changed=links[foundKey].toString()!=id||intents.contains(intentKey);links[foundKey]=id;m_journal["links"]=links;intents.remove(intentKey);m_journal["exports"]=intents;
            auto reviews=m_journal["reviews"].toObject();QString reviewKey="export/"+hash(intentKey);if(reviews.contains(reviewKey)){reviews.remove(reviewKey);m_journal["reviews"]=reviews;changed=true;}
            if(changed&&!persist()){complete(m_status);return;}continue;}
        if(m_journal["ignored"].toObject()["export/"+hash(intentKey)].toBool())continue;
        if(uncertain){addReview("export/"+hash(intentKey),contact,{}, {},"Possible external duplicate; export is held.");continue;}
        if(intents.contains(intentKey)){
            addReview("export/"+hash(intentKey),contact,{}, {},"An earlier export has an uncertain outcome. Check the external address book before retrying.");continue;
        }
        QString intentId=QUuid::createUuid().toString(QUuid::WithoutBraces);intents[intentKey]=intentId;m_journal["exports"]=intents;
        if(!persist()){complete(m_status);return;}auto gen=m_generation;
        auto exportConfig=m_config;exportConfig["exportBookId"]=m_snapshot.defaultExportBookId;
        m_provider->create(exportConfig,contact,intentId,[this,guard=QPointer<ContactSyncWorker>(this),gen,id,intentKey](Contact created,QString error){if(!guard||gen!=m_generation)return;
            if(!error.isEmpty()||created.id.isEmpty()){complete("Export outcome needs reconciliation before retry: "+error);return;}
            auto links=m_journal["links"].toObject();links[sourceKey(m_scope,created)]=id;m_journal["links"]=links;
            auto intents=m_journal["exports"].toObject();intents.remove(intentKey);m_journal["exports"]=intents;
            m_snapshot.contacts<<created;++m_exported;if(!persist()){complete(m_status);return;}exportNext();
        });return;
    }
    m_journal["lastSuccess"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if(!persist()){complete(m_status);return;}complete();
}
void ContactSyncWorker::complete(const QString&error){m_busy=false;
    m_status=error.isEmpty()?QString("Applied %1 contacts; %2 unchanged; exported %3; %4 awaiting review.").arg(m_imported).arg(m_unchanged).arg(m_exported).arg(reviews().size()):error;
    schedule();emit changed();
}
void ContactSyncWorker::resolve(const QString&key,const QString&action,const QString&personId,const QString&clientId,const QString&displayName){
    if(m_busy)return;auto entries=m_journal["reviews"].toObject();if(!entries.contains(key))return;auto entry=entries[key].toObject();
    if(entry["scope"].toString()!=m_journal["activeScope"].toString()||entry["scope"].toString().section('/',0,0)!=m_config.value("provider").toString()){
        m_status="Connect to this contact's original source and account before reviewing it.";emit changed();return;
    }
    if(m_config.value("provider")=="office365"&&m_office&&entry["scope"].toString()!=scope(m_account.label)){
        m_status="Sign in to this contact's original Office 365 account before reviewing it.";emit changed();return;
    }
    m_scope=entry["scope"].toString();
    if(entry["stage"].toString()=="client"){
        if(action!="clientLink"&&action!="clientCreate")return;
        QList<Person> people;QList<Client> clients;QString error;
        if(!m_repository.read(people,clients,error)){m_status=error;emit changed();return;}
        QString chosen=clientId;
        if(action=="clientLink"){
            bool available=false;for(const auto &client:clients)if(!client.deleted&&client.id==chosen)available=true;
            if(!available){m_status="The selected client is no longer available. Sync again.";emit changed();return;}
        } else {
            auto intents=m_journal["clientImports"].toObject();auto intent=intents[key].toObject();
            chosen=intent["id"].toString();QString name=intent["name"].toString();
            if(chosen.isEmpty()){
                chosen=QUuid::createUuid().toString();name=entry["company"].toString();
                auto used=[&](const QString &value){for(const auto &client:clients)if(!client.deleted&&client.name.trimmed().compare(value.trimmed(),Qt::CaseInsensitive)==0)return true;return false;};
                if(used(name)){
                    const auto base=name+" ("+m_config.value("provider").toString()+")";name=base;int suffix=2;
                    while(used(name))name=base+QString(" %1").arg(suffix++);
                }
            }
            intents[key]=QJsonObject{{"id",chosen},{"name",name}};m_journal["clientImports"]=intents;if(!persist())return;
            if(!m_repository.createClient(chosen,name,error)){m_status=error;emit changed();return;}
        }
        auto decisions=m_journal["clientDecisions"].toObject();decisions[key]=QJsonObject{{"clientId",chosen}};m_journal["clientDecisions"]=decisions;
        auto intents=m_journal["clientImports"].toObject();intents.remove(key);m_journal["clientImports"]=intents;
        entries.remove(key);m_journal["reviews"]=entries;if(!persist())return;
        bool pending=false;for(auto it=entries.begin();it!=entries.end();++it)if(it.value().toObject()["stage"]=="client"&&it.value().toObject()["scope"].toString()==m_scope)pending=true;
        if(!pending){syncNow();return;}
        m_status="Resolve the remaining client matches before reviewing contact duplicates.";emit changed();return;
    }
    for(auto it=entries.begin();it!=entries.end();++it)if(it.value().toObject()["stage"]=="client"&&it.value().toObject()["scope"].toString()==m_scope){
        m_status="Resolve client matches before reviewing contact duplicates.";emit changed();return;
    }
    if(action=="ignore"){auto ignored=m_journal["ignored"].toObject();ignored[key]=true;m_journal["ignored"]=ignored;entries.remove(key);m_journal["reviews"]=entries;persist();}
    else if(!key.startsWith("export/")&&(action=="useLeft"||action=="useBoth")){
        auto contact=decode(entry["contact"].toObject());
        QList<Person> people;QList<Client> clients;QString error;
        if(!m_repository.read(people,clients,error)){m_status=error;emit changed();return;}
        QString companyId=entry["selectedClientId"].toString();
        const auto match=companyMatch(contact.company,clients);
        if(!match.id.isEmpty())companyId=match.id;
        apply(contact,key,action=="useBoth"?QString():personId,companyId,contact.name,true,action=="useBoth");
    }
    else if(!key.startsWith("export/")&&(action=="link"||action=="create")){
        if(action=="link"&&personId.isEmpty()){m_status="Select an existing person.";emit changed();return;}
        if(action=="create"&&displayName.trimmed().isEmpty()){m_status="Enter a unique display name.";emit changed();return;}
        apply(decode(entry["contact"].toObject()),key,action=="create"?QString():personId,clientId,displayName,true);
    }else if(action=="retry"&&key.startsWith("export/")){
        auto contact=decode(entry["contact"].toObject());auto intents=m_journal["exports"].toObject();intents.remove(m_scope+"/"+contact.fields.value("id").toString());
        m_journal["exports"]=intents;entries.remove(key);m_journal["reviews"]=entries;persist();
    }
    emit changed();
}
void ContactSyncWorker::rejectCandidate(const QString&key,const QString&id){
    if(m_busy)return;auto rejected=m_journal["rejected"].toObject();auto list=rejected[key].toArray();if(!list.contains(id))list<<id;rejected[key]=list;m_journal["rejected"]=rejected;
    auto reviews=m_journal["reviews"].toObject();auto entry=reviews[key].toObject();QJsonArray candidates;
    for(const auto&v:entry["people"].toArray())if(v.toObject()["id"].toString()!=id)candidates<<v;
    entry["people"]=candidates;reviews[key]=entry;m_journal["reviews"]=reviews;persist();emit changed();
}
}
