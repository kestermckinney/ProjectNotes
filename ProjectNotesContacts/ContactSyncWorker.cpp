// SPDX-License-Identifier: GPL-3.0-only
#include "ContactSyncWorker.h"
#include "ContactMatcher.h"
#include "ThunderbirdBridge.h"
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
static QString legacyClientKey(const QString &scope,const QString &name){return "client/"+hash(scope+QChar(0)+name.trimmed().toCaseFolded());}
static QString clientKey(const QString &scope,const QString &name){
    auto normalized=ContactMatcher::normalize(name,true);if(normalized.isEmpty())normalized=name.trimmed().toCaseFolded();
    return "client/"+hash(scope+QChar(0)+normalized);
}
static bool sameTrimmed(const QString &left,const QString &right){return left.trimmed()==right.trimmed();}
Match ContactSyncWorker::companyMatch(const QString &name,const QList<Client> &clients) const {
    const auto decisions=m_journal["clientDecisions"].toObject();
    auto decision=decisions[clientKey(m_scope,name)].toObject();
    if(decision.isEmpty())decision=decisions[legacyClientKey(m_scope,name)].toObject();
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
    if(m_busy&&m_config.value("provider")=="office365"&&m_account.generation!=m_officeGeneration){
        const bool fullExport=m_runningFullExport;const int exported=m_exported;cancel();
        m_status=fullExport?QString("Office 365 account changed; full export canceled after %1 contacts.").arg(exported):QString("Office 365 account changed; synchronization canceled.");
        schedule();emit changed();
    }
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
QVariantList ContactSyncWorker::exportReviews() const {
    QVariantList out;
    for(const auto &item:m_exportPlan){
        if(!item.action.isEmpty()||item.candidates.isEmpty())continue;
        QVariantList candidates;
        for(const auto &candidate:item.candidates)candidates<<QVariantMap{
            {"id",candidate.contact.id},{"book",candidate.contact.book},{"bookName",candidate.bookName},
            {"writable",candidate.writable},{"reason",candidate.reason},
            {"label",candidate.contact.name+" — "+candidate.contact.company+" — "+candidate.bookName},
            {"contact",encode(candidate.contact).toVariantMap()}};
        out<<QVariantMap{{"personId",item.personId},{"contact",encode(item.contact).toVariantMap()},{"candidates",candidates}};
    }
    return out;
}
QString ContactSyncWorker::scope(const QString &account)const{return m_config.value("provider").toString()+"/"+account;}
void ContactSyncWorker::cancel(){++m_generation;m_timer.stop();if(m_provider)m_provider->cancel();m_busy=false;m_runningFullExport=false;m_exportPlan.clear();emit changed();}
void ContactSyncWorker::cancelSync(){const bool fullExport=m_runningFullExport;const int changedCount=m_exported+m_updated;cancel();m_status=fullExport?QString("Full contact export canceled after %1 contacts were changed.").arg(changedCount):QString("Contact synchronization canceled.");schedule();emit changed();}
void ContactSyncWorker::setDatabase(const QString &path){
    cancel();m_repository.close();m_databasePath=path;m_path.clear();m_journal={};m_books.clear();m_authFailed=false;
    m_config={{"provider","office365"},{"enabled",false},{"startup",true},{"interval",5},{"account",""},{"mailAccount",""},{"sourceBooks",QStringList{}}};
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
        if(!account.isEmpty()){m_config["provider"]="icloud";m_config["account"]=account;}
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
    // Older releases stored an automatic sync-export toggle. It must never
    // turn into an implicit full export after upgrading.
    const bool hadLegacyExport=m_config.remove("export");
    // Socket name is per OS user; access is further restricted by QLocalServer.
    m_config["bridgeSocket"]=thunderbirdBridgeSocket();
    if(hadLegacyExport&&!persist())return;
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
    if(!QStringList{"provider","enabled","startup","interval","account","mailAccount","sourceBooks"}.contains(key))return;
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
void ContactSyncWorker::fullExport(){
    if(m_busy||m_runningFullExport)return;
    m_runningFullExport=true;m_exported=0;m_updated=0;m_skipped=0;m_snapshot={};m_exportPlan.clear();
    if(m_config.value("provider")=="none"){complete("Choose an application in Email, Contact and Calendar Integration to export contacts.");return;}
    if(m_path.isEmpty()||!m_repository.isOpen()){complete("Open a database first.");return;}
    if(m_provider&&m_provider!=m_testProvider){m_provider->cancel();m_provider->deleteLater();}
    m_provider=m_testProvider?m_testProvider:new NativeContactProvider(m_config.value("provider").toString(),m_office,this);
    if(auto *provider=dynamic_cast<NativeContactProvider*>(m_provider))provider->setOfficeAccount(m_account);
    m_busy=true;m_runningFullExport=true;m_exported=0;m_status="Preparing full contact export…";emit changed();auto gen=++m_generation;
    m_officeGeneration=m_account.generation;
    m_provider->fetch(m_config,m_journal,[this,guard=QPointer<ContactSyncWorker>(this),gen](Snapshot snapshot){
        if(!guard||gen!=m_generation)return;
        m_authFailed=snapshot.authenticationError;
        if(!snapshot.error.isEmpty()){complete(snapshot.error);return;}
        if(!snapshot.complete){complete("The contact source returned incomplete data; no contacts were exported.");return;}
        if(snapshot.account.isEmpty()){complete("Contact provider did not return an account identity.");return;}
        auto provider=m_config.value("provider").toString();
        snapshot.defaultExportBookId=exportBook(snapshot.books,snapshot.defaultExportBookId,provider=="office365"||provider=="outlook");
        m_books=snapshot.books;m_snapshot=std::move(snapshot);m_scope=scope(m_snapshot.account);
        if(m_snapshot.defaultExportBookId.isEmpty()){complete("Full export unavailable: no writable address book is available.");return;}
        QString error;m_people.clear();m_clients.clear();
        if(!m_repository.read(m_people,m_clients,error)){complete(error);return;}
        prepareFullExport();
    });
}
void ContactSyncWorker::prepareFullExport(){
    QHash<QString,QVariantMap> books;
    for(const auto &value:m_snapshot.books){const auto book=value.toMap();books.insert(book.value("id").toString(),book);}
    const auto sameText=[](const QString &left,const QString &right){
        return left.normalized(QString::NormalizationForm_KC).simplified().toCaseFolded()==
               right.normalized(QString::NormalizationForm_KC).simplified().toCaseFolded();
    };
    const auto samePhone=[&](const QString &left,const QString &right){
        const auto normalizedLeft=ContactMatcher::phone(left),normalizedRight=ContactMatcher::phone(right);
        if(!normalizedLeft.isEmpty()||!normalizedRight.isEmpty())return !normalizedLeft.isEmpty()&&normalizedLeft==normalizedRight;
        return sameText(left,right);
    };
    const auto identical=[&](const Contact &left,const Contact &right){
        return sameText(left.name,right.name) && sameText(left.company,right.company) &&
               sameText(left.fields.value("email").toString(),right.fields.value("email").toString()) &&
               samePhone(left.fields.value("office_phone").toString(),right.fields.value("office_phone").toString()) &&
               samePhone(left.fields.value("cell_phone").toString(),right.fields.value("cell_phone").toString()) &&
               sameText(left.fields.value("role").toString(),right.fields.value("role").toString());
    };
    const auto savedLinks=m_journal["exportLinks"].toObject();int conflicts=0;
    for(const auto &person:m_people){
        if(person.deleted)continue;
        ExportItem item;item.personId=person.id;item.contact.name=person.name;item.contact.company=person.company;item.contact.fields=person.fields;
        item.contact.fields["company"]=person.company;item.contact.fields["id"]=person.id;
        const auto email=person.fields.value("email").toString();if(!email.trimmed().isEmpty())item.contact.emails={email};
        for(const auto &phone:QStringList{person.fields.value("office_phone").toString(),person.fields.value("cell_phone").toString()})if(!phone.trimmed().isEmpty())item.contact.phones<<phone;
        const auto saved=savedLinks.value(m_scope+"/"+person.id).toObject();
        const auto add=[&](const Contact &remote,const QString &reason){
            const auto book=books.value(remote.book);item.candidates<<ExportCandidate{remote,reason,book.value("writable").toBool(),book.value("name",remote.book).toString()};};
        for(const auto &remote:m_snapshot.contacts)if(!saved.isEmpty()&&remote.id==saved["id"].toString()&&remote.book==saved["book"].toString()){
            const bool unchanged=identical(item.contact,remote);add(remote,unchanged?"Identical managed fields":"Previously exported contact");
            if(unchanged){item.action="skip";item.targetId=remote.id;}break;
        }
        if(item.candidates.isEmpty())for(const auto &remote:m_snapshot.contacts)if(identical(item.contact,remote)){
            add(remote,"Identical managed fields");item.action="skip";item.targetId=remote.id;break;
        }
        if(item.candidates.isEmpty())for(const auto &remote:m_snapshot.contacts){
            bool emailMatch=false,phoneMatch=false;
            for(const auto &left:item.contact.emails)for(const auto &right:remote.emails)if(!left.trimmed().isEmpty()&&left.trimmed().compare(right.trimmed(),Qt::CaseInsensitive)==0)emailMatch=true;
            for(const auto &left:item.contact.phones)for(const auto &right:remote.phones)if(!ContactMatcher::phone(left).isEmpty()&&ContactMatcher::phone(left)==ContactMatcher::phone(right))phoneMatch=true;
            const bool sameName=ContactMatcher::normalize(item.contact.name)==ContactMatcher::normalize(remote.name);
            const bool sameCompany=ContactMatcher::normalize(item.contact.company,true)==ContactMatcher::normalize(remote.company,true);
            const auto nameSimilarity=ContactMatcher::nameSimilarity(item.contact.name,remote.name);
            const bool similarName=nameSimilarity>=.92;
            bool exactProfile=sameName&&sameCompany;
            for(const auto &field:QStringList{"email","office_phone","cell_phone","role"})exactProfile=exactProfile&&item.contact.fields.value(field).toString().trimmed().compare(remote.fields.value(field).toString().trimmed(),Qt::CaseInsensitive)==0;
            if(exactProfile||((emailMatch||phoneMatch)&&nameSimilarity>=.85)||(similarName&&sameCompany&&!item.contact.company.trimmed().isEmpty()))
                add(remote,emailMatch?"Exact email":phoneMatch?"Exact phone":exactProfile?"Exact contact fields":"Name and company similarity");
        }
        if(item.candidates.isEmpty())item.action="create";else if(item.action.isEmpty())++conflicts;
        m_exportPlan<<item;
    }
    m_fullExportIndex=0;
    if(conflicts){m_status=QString("Review %1 possible existing contacts before export.").arg(conflicts);emit changed();emit exportReviewReady();return;}
    m_status=QString("Exporting %1 contacts…").arg(m_exportPlan.size());emit changed();fullExportNext();
}
void ContactSyncWorker::resolveExport(const QString &personId,const QString &action,const QString &externalId){
    if(!m_runningFullExport||!QStringList{"update","skip","create"}.contains(action))return;
    for(auto &item:m_exportPlan)if(item.personId==personId&&item.action.isEmpty()){
        if(action=="update"||(action=="skip"&&!externalId.isEmpty())){
            bool valid=false;for(const auto &candidate:item.candidates)if(candidate.contact.id==externalId&&(action!="update"||candidate.writable))valid=true;
            if(!valid){m_status=action=="update"?"Choose a writable existing contact to update.":"Choose an existing contact to keep.";emit changed();return;}
            item.targetId=externalId;
        }
        item.action=action;break;
    }
    if(!exportReviews().isEmpty()){m_status=QString("Review %1 remaining possible contacts.").arg(exportReviews().size());emit changed();return;}
    m_status=QString("Exporting %1 contacts…").arg(m_exportPlan.size());m_fullExportIndex=0;emit changed();fullExportNext();
}
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
    if(displayName.trimmed().isEmpty())displayName=c.name;
    if(keepBoth&&personId.isEmpty()){
        // The database requires unique names. Derive a name from the chosen
        // contact automatically, and persist it for subsequent synchronization.
        auto used=[&](const QString &name){for(const auto &person:people)if(ContactMatcher::normalize(person.name)==ContactMatcher::normalize(name))return true;return false;};
        if(used(displayName)){
            const auto base=displayName+" ("+(c.company.trimmed().isEmpty()?m_scope.section('/',0,0):c.company.trimmed())+")";
            displayName=base;int suffix=2;
            while(used(displayName))displayName=base+QString(" %1").arg(suffix++);
        }
    }
    if(personId.isEmpty())personId=saved["personId"].toString();if(personId.isEmpty())personId=QUuid::createUuid().toString();
    QString newClientId;
    if(!c.company.trimmed().isEmpty()&&clientId.isEmpty()){
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
        const auto clientDecisions=m_journal["clientDecisions"].toObject();
        const bool hasClientDecision=clientDecisions.contains(clientKey(m_scope,c.company))||clientDecisions.contains(legacyClientKey(m_scope,c.company));
        if(!hasClientDecision&&ContactMatcher::equivalent("company",decision["company"].toString(),c.company) && !decision["clientId"].toString().isEmpty()){
            for(const auto &client:m_clients)if(!client.deleted&&client.id==decision["clientId"].toString()){cm.id=client.id;cm.review=false;break;}
        }
        auto pm=linked.isEmpty()?ContactMatcher::person(c,cm.id,m_people,m_journal["rejected"].toObject()[key].toVariant().toStringList()):Match{};
        if(cm.review){prepareClientReviews();complete(m_status);return;}
        if(linked.isEmpty()&&pm.review){addReview(key,c,pm,cm,"Possible duplicate; choose a match before importing.");continue;}
        QString displayName=ContactMatcher::equivalent("name",decision["externalName"].toString(),c.name) && !decision["displayName"].toString().isEmpty()?decision["displayName"].toString():c.name;
        if(!linked.isEmpty()&&!m_journal["reviews"].toObject().contains(key)){
            bool unchanged=false;
            for(const auto&p:m_people)if(p.id==linked){auto desired=p.fields;
                for(const auto&field:QStringList{"name","email","office_phone","cell_phone","role"})if(c.fields.contains(field))desired[field]=c.fields[field];
                desired["name"]=displayName;if(c.fields.contains("company"))desired["client_id"]=cm.id;
                unchanged=ContactMatcher::equivalentFields(desired,p.fields) && (c.company.trimmed().isEmpty()||!cm.id.isEmpty());break;
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
    m_people.clear();m_clients.clear();if(!m_repository.read(m_people,m_clients,error)){complete(error);return;}
    finishSync();
}
void ContactSyncWorker::finishSync(){
    m_journal["lastSuccess"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if(!persist()){complete(m_status);return;}
    complete();
}
void ContactSyncWorker::fullExportNext(){
    if(m_fullExportIndex>=m_exportPlan.size()){
        QString bookName=m_snapshot.defaultExportBookId;
        for(const auto &value:m_snapshot.books){const auto book=value.toMap();if(book.value("id")==m_snapshot.defaultExportBookId){bookName=book.value("name").toString();break;}}
        complete(QString("Contact export complete: %1 created in %2, %3 updated, %4 skipped.").arg(m_exported).arg(bookName).arg(m_updated).arg(m_skipped),true);return;
    }
    const auto item=m_exportPlan.at(m_fullExportIndex++);
    if(item.action=="skip"){
        if(!item.targetId.isEmpty())for(const auto &candidate:item.candidates)if(candidate.contact.id==item.targetId){
            auto links=m_journal["exportLinks"].toObject();links[m_scope+"/"+item.personId]=QJsonObject{{"id",candidate.contact.id},{"book",candidate.contact.book}};m_journal["exportLinks"]=links;
            if(!persist()){complete(m_status);return;}break;
        }
        ++m_skipped;fullExportNext();return;
    }
    auto exportConfig=m_config;exportConfig["exportBookId"]=m_snapshot.defaultExportBookId;
    const auto gen=m_generation;auto completed=[this,guard=QPointer<ContactSyncWorker>(this),gen,item](Contact result,QString error){
            if(!guard||gen!=m_generation)return;
            if(!error.isEmpty()||result.id.isEmpty()){
                complete(QString("Contact export stopped after %1 contacts were changed. The last result may be uncertain; refresh the address book before retrying. %2")
                         .arg(m_exported+m_updated).arg(error));return;
            }
            auto links=m_journal["exportLinks"].toObject();links[m_scope+"/"+item.personId]=QJsonObject{{"id",result.id},{"book",result.book}};m_journal["exportLinks"]=links;
            if(!persist()){complete(m_status);return;}
            if(item.action=="update")++m_updated;else ++m_exported;fullExportNext();
        };
    if(item.action=="update"){
        for(const auto &candidate:item.candidates)if(candidate.contact.id==item.targetId){m_provider->update(exportConfig,candidate.contact,item.contact,std::move(completed));return;}
        complete("The selected existing contact is no longer available; start the export again.");return;
    }
    const auto intent=hash(m_scope+QChar(0)+item.personId).left(32);
    m_provider->create(exportConfig,item.contact,intent,std::move(completed));
}
void ContactSyncWorker::complete(const QString&error,bool exportSuccess){m_busy=false;
    m_status=error.isEmpty()?QString("Applied %1 contacts; %2 unchanged; exported %3; %4 awaiting review.").arg(m_imported).arg(m_unchanged).arg(m_exported).arg(reviews().size()):error;
    const bool wasExport=m_runningFullExport;
    m_runningFullExport=false;schedule();emit changed();
    if(wasExport)emit fullExportFinished(m_status,exportSuccess);
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
        QString clientAction=action;
        if(clientAction=="clientLink")clientAction="clientUseRight";
        if(clientAction=="clientCreate")clientAction="clientUseBoth";
        if(!QStringList{"clientUseLeft","clientUseRight","clientUseBoth"}.contains(clientAction))return;
        QList<Person> people;QList<Client> clients;QString error;
        if(!m_repository.read(people,clients,error)){m_status=error;emit changed();return;}
        QString chosen=clientId;
        if(clientAction=="clientUseLeft"||clientAction=="clientUseRight"){
            QString expectedName;for(const auto &candidate:entry["clients"].toArray())if(candidate.toObject()["id"].toString()==chosen)expectedName=candidate.toObject()["label"].toString();
            bool available=false;for(const auto &client:clients)if(!client.deleted&&client.id==chosen&&sameTrimmed(client.name,expectedName))available=true;
            if(!available){m_status="The selected client is no longer available. Sync again.";emit changed();return;}
            if(clientAction=="clientUseLeft"&&!m_repository.updateClientName(chosen,expectedName,entry["company"].toString(),error)){
                m_status=error;emit changed();return;
            }
        } else {
            auto intents=m_journal["clientImports"].toObject();auto intent=intents[key].toObject();
            chosen=intent["id"].toString();QString name=intent["name"].toString();
            if(chosen.isEmpty()){
                chosen=QUuid::createUuid().toString();name=entry["company"].toString().trimmed();
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
