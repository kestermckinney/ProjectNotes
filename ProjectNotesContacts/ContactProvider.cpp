// SPDX-License-Identifier: GPL-3.0-only
#include "ContactProvider.h"
#include "VCard.h"
#include "ContactMatcher.h"
#include "credentialstore.h"
#include "ProjectNotesIntegrations/Office365Service.h"
#include "ProjectNotesIntegrations/QtNetworkHttpTransport.h"
#include <QCoreApplication>
#include <QDomDocument>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QProcess>
#include <QTimer>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QSet>
#include <QThread>
namespace PN::Contacts {
static QString graphRoot(){return "https://graph.microsoft.com/v1.0";}
static bool graphUrl(const QUrl &url){return url.scheme()=="https"&&url.host()=="graph.microsoft.com"&&url.userInfo().isEmpty();}
static bool cloudUrl(const QUrl &url){return url.scheme()=="https"&&(url.host()=="icloud.com"||url.host().endsWith(".icloud.com"))&&url.userInfo().isEmpty();}
static QString httpError(const PN::Comm::HttpResponse&r){return !r.error.code.isEmpty()?r.error.displayText:QString("Contact service returned HTTP %1.").arg(r.statusCode);}
static QDomElement child(const QDomNode &n,const QString &name){for(auto c=n.firstChild();!c.isNull();c=c.nextSibling()){auto e=c.toElement();if(e.localName()==name || e.tagName().section(':',-1)==name)return e;}return {};}
static QDomElement descendant(const QDomNode &n,const QString &name){auto direct=child(n,name);if(!direct.isNull())return direct;for(auto c=n.firstChild();!c.isNull();c=c.nextSibling()){auto e=descendant(c,name);if(!e.isNull())return e;}return {};}
static QDomDocument xml(const QByteArray&body){QDomDocument d;d.setContent(body,QDomDocument::ParseOption::UseNamespaceProcessing);return d;}
static QByteArray propfind(const QByteArray &properties){return "<d:propfind xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:carddav\"><d:prop>"+properties+"</d:prop></d:propfind>";}
NativeContactProvider::NativeContactProvider(QString provider,PN::Comm::Office365Service*office,QObject*parent)
    :ContactProvider(parent),m_provider(std::move(provider)),m_office(office),m_transport(new PN::Comm::QtNetworkHttpTransport(this)){}
PN::Comm::Office365Account NativeContactProvider::officeAccount() const {
    return m_office&&m_office->thread()==QThread::currentThread()?m_office->account():m_officeAccount;
}
void NativeContactProvider::sendGraph(PN::Comm::HttpRequest request,PN::Comm::HttpTransport::Completion done) {
    if(!m_office)return;
    if(m_office->thread()==QThread::currentThread()){m_office->sendGraphRequest(std::move(request),std::move(done));return;}
    QPointer<NativeContactProvider> guard(this);auto *office=m_office;
    QMetaObject::invokeMethod(office,[office,guard,request=std::move(request),done=std::move(done)]()mutable{
        if(!guard)return;
        office->sendGraphRequest(std::move(request),[guard,done=std::move(done)](auto response)mutable{
            if(guard)QMetaObject::invokeMethod(guard,[guard,done=std::move(done),response=std::move(response)]()mutable{
                if(guard)done(std::move(response));
            },Qt::QueuedConnection);
        });
    },Qt::QueuedConnection);
}
void NativeContactProvider::cancelGraph(const QUuid &id) {
    if(!m_office)return;auto *office=m_office;
    if(office->thread()==QThread::currentThread())office->cancelGraphRequest(id);
    else QMetaObject::invokeMethod(office,[office,id]{office->cancelGraphRequest(id);},Qt::QueuedConnection);
}
NativeContactProvider::~NativeContactProvider(){cancel();}
void NativeContactProvider::cancel(){++m_generation;m_done={};for(auto id:std::as_const(m_requests)){m_transport->cancel(id);if(m_davTransport)m_davTransport->cancel(id);if(m_office)cancelGraph(id);}m_requests.clear();if(m_process)m_process->kill();if(m_socket)m_socket->abort();}
void NativeContactProvider::finish(QString error,bool authentication){
    m_snapshot.error=error;m_snapshot.authenticationError=authentication;
    if(error.isEmpty())m_snapshot.defaultExportBookId=exportBook(m_snapshot.books,m_snapshot.defaultExportBookId,m_provider=="office365"||m_provider=="outlook");
    if(error.isEmpty()&&(m_provider=="office365"||m_provider=="icloud")){
        m_snapshot.contacts.clear();QSet<QString> activeBooks;for(const auto&b:m_snapshot.books)activeBooks.insert(b.toMap().value("id").toString());
        for(auto it=m_remote.begin();it!=m_remote.end();){auto c=decode(it.value().toObject());if(!activeBooks.contains(c.book))it=m_remote.erase(it);else {m_snapshot.contacts<<c;++it;}}
        m_snapshot.cursors={{"tokens",m_tokens},{"contacts",m_remote}};
    }
    auto done=std::move(m_done);if(done)done(m_snapshot);
}
void NativeContactProvider::fetch(const QVariantMap&config,const QJsonObject&state,Completion done){
    cancel();m_config=config;m_state=state;m_snapshot={};m_done=std::move(done);m_folderQueue.clear();m_remote={};m_tokens={};
    if(m_provider=="office365"){
        if(!m_office || !officeAccount().authenticated){finish("Sign in to Office 365 with contact permission.",true);return;}
        m_snapshot.account=officeAccount().label;
        m_snapshot.defaultExportBookId="default";
        m_accountGeneration=officeAccount().generation;
        auto cached=state["providerStates"].toObject()[m_provider+"/"+m_snapshot.account].toObject();m_remote=cached["contacts"].toObject();m_tokens=cached["tokens"].toObject();
        m_snapshot.books.append(QVariantMap{{"id","default"},{"name","Contacts"},{"writable",true}});
        graphFolders(QUrl(graphRoot()+"/me/contactFolders?$top=100"));
    }else if(m_provider=="icloud"){
        m_snapshot.account=config.value("account").toString().trimmed().toCaseFolded();
        auto cached=state["providerStates"].toObject()[m_provider+"/"+m_snapshot.account].toObject();m_remote=cached["contacts"].toObject();m_tokens=cached["tokens"].toObject();
        davDiscover(0,QUrl("https://contacts.icloud.com/"));
    }else {
        local({{"version",1},{"action","snapshot"}},[this](QJsonObject o,QString e){
            if(!e.isEmpty()){finish(e);return;}m_snapshot.account=o["account"].toString();
            m_snapshot.books=o["books"].toArray().toVariantList();
            m_snapshot.defaultExportBookId=o["defaultExportBookId"].toString();
            for(const auto&v:o["contacts"].toArray())m_snapshot.contacts<<decode(v.toObject());
            finish();
        });
    }
}
void NativeContactProvider::graphFolders(QUrl url){
    if(!graphUrl(url)){finish("Office 365 returned an unsafe page URL.");return;}
    PN::Comm::HttpRequest r;r.operationId=QUuid::createUuid();r.method="GET";r.url=url;auto gen=m_generation;
    m_requests<<r.operationId;
    sendGraph(r,[this,guard=QPointer<NativeContactProvider>(this),gen](auto response){if(!guard||gen!=m_generation)return;
        if(officeAccount().generation!=m_accountGeneration){finish("Office 365 account changed during synchronization.");return;}
        if(response.statusCode!=200){finish(httpError(response),response.statusCode==401||response.statusCode==403);return;}
        auto o=QJsonDocument::fromJson(response.body).object();if(!o["value"].isArray()){finish("Malformed Office 365 folder response.");return;}
        for(const auto&v:o["value"].toArray()){auto f=v.toObject();m_snapshot.books<<QVariantMap{{"id",f["id"].toString()},{"name",f["displayName"].toString()},{"writable",true}};
            if(f["childFolderCount"].toInt()>0)m_folderQueue<<graphRoot()+"/me/contactFolders/"+QString::fromLatin1(QUrl::toPercentEncoding(f["id"].toString()))+"/childFolders?$top=100";}
        auto next=o["@odata.nextLink"].toString();if(!next.isEmpty())graphFolders(QUrl(next));else if(!m_folderQueue.isEmpty())graphFolders(QUrl(m_folderQueue.takeFirst()));else graphBook(0);
    });
}
void NativeContactProvider::graphBook(int index){
    if(index>=m_snapshot.books.size()){finish();return;}
    m_config["bookIndex"]=index;auto id=m_snapshot.books[index].toMap().value("id").toString();
    QString base=id=="default"?graphRoot()+"/me/contacts":graphRoot()+"/me/contactFolders/"+QString::fromLatin1(QUrl::toPercentEncoding(id))+"/contacts";
    QString token=m_tokens[id].toString();m_incremental=!token.isEmpty();
    if(!m_incremental)for(auto it=m_remote.begin();it!=m_remote.end();)if(decode(it.value().toObject()).book==id)it=m_remote.erase(it);else ++it;
    graphPage(QUrl(m_incremental?token:base+"/delta"));
}
void NativeContactProvider::graphPage(QUrl url){
    if(!graphUrl(url)){finish("Office 365 returned an unsafe page URL.");return;}
    PN::Comm::HttpRequest r;r.operationId=QUuid::createUuid();r.method="GET";r.url=url;r.headers["Prefer"]="IdType=\"ImmutableId\"";auto gen=m_generation;
    m_requests<<r.operationId;
    sendGraph(r,[this,guard=QPointer<NativeContactProvider>(this),gen](auto response){if(!guard||gen!=m_generation)return;
        if(officeAccount().generation!=m_accountGeneration){finish("Office 365 account changed during synchronization.");return;}
        if((response.statusCode==410||response.statusCode==404)&&m_incremental){auto book=m_snapshot.books[m_config.value("bookIndex").toInt()].toMap().value("id").toString();m_tokens.remove(book);graphBook(m_config.value("bookIndex").toInt());return;}
        if(response.statusCode!=200){finish(httpError(response),response.statusCode==401||response.statusCode==403);return;}
        auto o=QJsonDocument::fromJson(response.body).object();if(!o["value"].isArray()){finish("Malformed Office 365 contact response.");return;}
        QString book=m_snapshot.books[m_config.value("bookIndex").toInt()].toMap().value("id").toString();
        for(const auto&v:o["value"].toArray()){
            const auto f=v.toObject();Contact c;c.id=f["id"].toString();c.book=book;c.name=f["displayName"].toString();c.company=f["companyName"].toString();
            c.etag=f["changeKey"].toString();
            if(f.contains("@removed")){m_remote.remove(sourceKey(m_snapshot.account,c));continue;}
            for(const auto&e:f["emailAddresses"].toArray())c.emails<<e.toObject()["address"].toString();
            for(const auto&p:f["businessPhones"].toArray())c.phones<<p.toString();
            if(!f["mobilePhone"].toString().isEmpty())c.phones<<f["mobilePhone"].toString();
            for(const auto&pair:QList<QPair<QString,QString>>{{"displayName","name"},{"companyName","company"},{"mobilePhone","cell_phone"},{"jobTitle","role"}})
                if(f.contains(pair.first))c.fields[pair.second]=f[pair.first].toString();
            if(f.contains("emailAddresses"))c.fields["email"]=c.emails.value(0);
            if(f.contains("businessPhones"))c.fields["office_phone"]=f["businessPhones"].toArray().isEmpty()?QString():f["businessPhones"].toArray().first().toString();
            if(c.id.isEmpty()){finish("Office 365 returned a contact without an ID.");return;}
            QString key=sourceKey(m_snapshot.account,c);auto previous=m_remote[key].toObject();
            if(m_incremental&&!previous.isEmpty()){
                auto old=decode(previous);if(!f.contains("displayName"))c.name=old.name;if(!f.contains("companyName"))c.company=old.company;
                if(!f.contains("emailAddresses"))c.emails=old.emails;
                auto merged=old.fields;for(auto it=c.fields.begin();it!=c.fields.end();++it)merged[it.key()]=it.value();c.fields=merged;
                c.phones={merged.value("office_phone").toString(),merged.value("cell_phone").toString()};
            }
            m_remote[key]=encode(c);
        }
        auto next=o["@odata.nextLink"].toString();if(!next.isEmpty())graphPage(QUrl(next));else {m_tokens[book]=o["@odata.deltaLink"].toString();graphBook(m_config.value("bookIndex").toInt()+1);}
    });
}
void NativeContactProvider::dav(QByteArray method,QUrl url,QByteArray body,int depth,std::function<void(PN::Comm::HttpResponse)> done,int redirects,QByteArray condition){
    if(!cloudUrl(url)){PN::Comm::HttpResponse r;r.error.code="unsafe-url";r.error.displayText="iCloud returned an unsafe URL.";done(r);return;}
    QString error;auto password=m_password?m_password():CredentialStore::read(m_config.value("credentialService","Project Notes iCloud Contacts").toString(),m_config.value("account").toString().trimmed(),&error);
    if(password.isEmpty()){PN::Comm::HttpResponse r;r.statusCode=401;r.error.code="credentials";r.error.displayText="Store an iCloud app-specific password first.";done(r);return;}
    PN::Comm::HttpRequest r;r.operationId=QUuid::createUuid();r.method=method;r.url=url;r.body=body;r.manualRedirects=true;
    r.headers["Authorization"]="Basic "+(m_config.value("account").toString().trimmed()+":"+password).toUtf8().toBase64();
    r.headers["Content-Type"]=method=="PUT"?"text/vcard; charset=utf-8":"application/xml; charset=utf-8";
    r.headers["Depth"]=QByteArray::number(depth);if(method=="PUT")r.headers[condition.isEmpty()?"If-None-Match":"If-Match"]=condition.isEmpty()?QByteArray("*"):condition;
    m_requests<<r.operationId;auto gen=m_generation;
    auto transport=m_davTransport?m_davTransport:m_transport;
    transport->send(r,[this,guard=QPointer<NativeContactProvider>(this),gen,method,url,body,depth,done,redirects,condition](auto response){if(!guard||gen!=m_generation)return;
        if(QList<int>{301,302,307,308}.contains(response.statusCode)){
            auto target=url.resolved(QUrl(QString::fromUtf8(response.headers.value("Location",response.headers.value("location")))));
            if(redirects<5&&cloudUrl(target))dav(method,target,body,depth,done,redirects+1,condition);
            else {response.error.code="redirect";response.error.displayText="Unsafe or excessive iCloud redirects.";done(response);}return;
        }done(response);
    });
}
void NativeContactProvider::davDiscover(int step,QUrl url){
    QByteArray body=propfind(step==0?"<d:current-user-principal/>":step==1?"<c:addressbook-home-set/>":"<d:resourcetype/><d:displayname/><d:sync-token/>");
    dav("PROPFIND",url,body,step==2?1:0,[this,step,url](auto r){
        if(r.statusCode!=207&&r.statusCode!=200){finish(httpError(r),r.statusCode==401||r.statusCode==403);return;}
        auto d=xml(r.body);if(d.documentElement().isNull()){finish("Malformed iCloud response.");return;}
        if(step<2){auto prop=descendant(d,step==0?"current-user-principal":"addressbook-home-set");auto href=descendant(prop,"href").text();
            if(href.isEmpty()){finish("iCloud did not return its address-book location.");return;}davDiscover(step+1,url.resolved(QUrl(href)));return;}
        auto root=d.documentElement();for(auto n=root.firstChild();!n.isNull();n=n.nextSibling()){
            if(descendant(n,"addressbook").isNull())continue;
            QString href=child(n,"href").text();m_snapshot.books<<QVariantMap{{"id",url.resolved(QUrl(href)).toString()},{"name",descendant(n,"displayname").text()},{"writable",true},{"syncToken",descendant(n,"sync-token").text()}};
        }
        if(m_snapshot.books.isEmpty()){finish("No iCloud address books were found.");return;}davBook(0);
    });
}
void NativeContactProvider::davBook(int index){
    if(index>=m_snapshot.books.size()){finish();return;}
    auto book=m_snapshot.books[index].toMap().value("id").toString();
    QString token=m_tokens[book].toString();bool incremental=!token.isEmpty();
    if(!incremental)for(auto it=m_remote.begin();it!=m_remote.end();)if(decode(it.value().toObject()).book==book)it=m_remote.erase(it);else ++it;
    QByteArray body="<c:addressbook-query xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:carddav\"><d:prop><d:getetag/><c:address-data/></d:prop><c:filter><c:prop-filter name=\"UID\"/></c:filter></c:addressbook-query>";
    if(incremental){QDomDocument d;auto root=d.createElement("d:sync-collection");root.setAttribute("xmlns:d","DAV:");root.setAttribute("xmlns:c","urn:ietf:params:xml:ns:carddav");d.appendChild(root);
        auto t=d.createElement("d:sync-token");t.appendChild(d.createTextNode(token));root.appendChild(t);auto level=d.createElement("d:sync-level");level.appendChild(d.createTextNode("1"));root.appendChild(level);
        auto prop=d.createElement("d:prop");prop.appendChild(d.createElement("d:getetag"));prop.appendChild(d.createElement("c:address-data"));root.appendChild(prop);body=d.toByteArray();}
    dav("REPORT",QUrl(book),body,1,[this,index,book,incremental](auto r){
        if(incremental&&(r.statusCode==403||r.statusCode==409||r.statusCode==501)){m_tokens.remove(book);davBook(index);return;}
        if(r.statusCode!=207&&r.statusCode!=200){finish(httpError(r),r.statusCode==401||r.statusCode==403);return;}
        auto d=xml(r.body);if(d.documentElement().isNull()||d.documentElement().tagName().section(':',-1)!="multistatus"){finish("Malformed iCloud contact response.");return;}
        for(auto n=d.documentElement().firstChild();!n.isNull();n=n.nextSibling()){
            QString href=child(n,"href").text();Contact identity;identity.book=book;identity.id=QUrl(book).resolved(QUrl(href)).toString();
            if(child(n,"status").text().contains(" 404 ")){m_remote.remove(sourceKey(m_snapshot.account,identity));continue;}
            auto propertyStatus=descendant(child(n,"propstat"),"status").text();
            if(!propertyStatus.isEmpty()&&!propertyStatus.contains(" 200 ")){finish("iCloud could not read a contact; synchronization is held to prevent duplicate exports.");return;}
            auto card=descendant(n,"address-data");if(card.isNull())continue;
            if(href.isEmpty()){finish("iCloud contact has no stable identity.");return;}
            auto contact=parseVCard(card.text(),identity.id,book);contact.etag=descendant(n,"getetag").text();m_remote[sourceKey(m_snapshot.account,contact)]=encode(contact);
        }
        QString next=child(d.documentElement(),"sync-token").text();if(next.isEmpty())next=m_snapshot.books[index].toMap().value("syncToken").toString();m_tokens[book]=next;davBook(index+1);
    });
}
void NativeContactProvider::create(const QVariantMap&config,const Contact&contact,const QString&intentId,Created done){
    m_config=config;QString book=config.value("exportBookId").toString();
    if(book.isEmpty()){done({},"The contact application has no available writable address book.");return;}
    if(m_provider=="office365"){
        QString path=book=="default"?"/me/contacts":"/me/contactFolders/"+QString::fromLatin1(QUrl::toPercentEncoding(book))+"/contacts";
        PN::Comm::HttpRequest r;r.operationId=QUuid::createUuid();r.method="POST";r.url=QUrl(graphRoot()+path);r.headers["Content-Type"]="application/json";r.headers["Prefer"]="IdType=\"ImmutableId\"";
        QJsonObject body{{"displayName",contact.name},{"companyName",contact.company},{"jobTitle",contact.fields.value("role").toString()},
            {"mobilePhone",contact.fields.value("cell_phone").toString()},{"emailAddresses",QJsonArray{QJsonObject{{"address",contact.fields.value("email").toString()}}}},
            {"businessPhones",QJsonArray{contact.fields.value("office_phone").toString()}}};
        r.body=QJsonDocument(body).toJson(QJsonDocument::Compact);auto gen=m_generation;
        m_requests<<r.operationId;
        sendGraph(r,[this,guard=QPointer<NativeContactProvider>(this),gen,done,contact,book](auto response){if(!guard||gen!=m_generation)return;
            auto c=contact;c.book=book;c.id=QJsonDocument::fromJson(response.body).object()["id"].toString();
            done(c,response.statusCode==201&&!c.id.isEmpty()?QString():httpError(response));});
    }else if(m_provider=="icloud"){
        QUrl url=QUrl(book.endsWith('/')?book:book+'/').resolved(QUrl(intentId+".vcf"));
        dav("PUT",url,writeVCard(contact,intentId),0,[done,contact,book,url](auto r){auto c=contact;c.id=url.toString();c.book=book;done(c,(r.statusCode==201||r.statusCode==204)?QString():httpError(r));});
    }else local({{"version",1},{"action","create"},{"book",book},{"intentId",intentId},{"contact",encode(contact)}},
        [done](auto o,QString e){done(decode(o["contact"].toObject()),e);});
}
void NativeContactProvider::update(const QVariantMap&config,const Contact&existing,const Contact&contact,Updated done){
    m_config=config;
    if(existing.id.isEmpty()||existing.book.isEmpty()){done({},"The existing contact has no stable identity.");return;}
    if(m_provider=="office365"){
        QString path=existing.book=="default"?"/me/contacts/":"/me/contactFolders/"+QString::fromLatin1(QUrl::toPercentEncoding(existing.book))+"/contacts/";
        PN::Comm::HttpRequest r;r.operationId=QUuid::createUuid();r.method="PATCH";r.url=QUrl(graphRoot()+path+QString::fromLatin1(QUrl::toPercentEncoding(existing.id)));
        r.headers["Content-Type"]="application/json";r.headers["Prefer"]="IdType=\"ImmutableId\"";
        QJsonArray emails,phones;const auto newEmail=contact.fields.value("email").toString(),oldEmail=existing.fields.value("email").toString();
        if(!newEmail.isEmpty())emails<<QJsonObject{{"address",newEmail}};
        for(const auto &email:existing.emails)if(email.compare(oldEmail,Qt::CaseInsensitive)!=0&&email.compare(newEmail,Qt::CaseInsensitive)!=0)emails<<QJsonObject{{"address",email}};
        const auto newOffice=contact.fields.value("office_phone").toString(),oldOffice=ContactMatcher::phone(existing.fields.value("office_phone").toString()),oldCell=ContactMatcher::phone(existing.fields.value("cell_phone").toString());
        if(!newOffice.isEmpty())phones<<newOffice;
        for(const auto &phone:existing.phones){const auto normalized=ContactMatcher::phone(phone);if(normalized!=oldOffice&&normalized!=oldCell&&normalized!=ContactMatcher::phone(newOffice))phones<<phone;}
        r.body=QJsonDocument(QJsonObject{{"displayName",contact.name},{"companyName",contact.company},{"jobTitle",contact.fields.value("role").toString()},
            {"mobilePhone",contact.fields.value("cell_phone").toString()},{"emailAddresses",emails},{"businessPhones",phones}}).toJson(QJsonDocument::Compact);
        auto gen=m_generation;m_requests<<r.operationId;
        sendGraph(r,[this,guard=QPointer<NativeContactProvider>(this),gen,done,contact,existing](auto response){if(!guard||gen!=m_generation)return;
            auto c=contact;c.id=existing.id;c.book=existing.book;c.etag=QJsonDocument::fromJson(response.body).object()["changeKey"].toString();
            done(c,response.statusCode==200?QString():httpError(response));});
    }else if(m_provider=="icloud"){
        if(existing.etag.isEmpty()){done({},"The iCloud contact changed or has no revision token; refresh the export review.");return;}
        dav("GET",QUrl(existing.id),{},0,[this,done,contact,existing](auto fetched){
            if(fetched.statusCode!=200){done({},httpError(fetched));return;}
            auto body=mergeVCard(fetched.body,contact,existing);
            dav("PUT",QUrl(existing.id),body,0,[done,contact,existing](auto saved){auto c=contact;c.id=existing.id;c.book=existing.book;
                done(c,(saved.statusCode==200||saved.statusCode==204)?QString():httpError(saved));},0,existing.etag.toUtf8());
        });
    }else local({{"version",1},{"action","update"},{"book",existing.book},{"existing",encode(existing)},{"contact",encode(contact)}},
        [done](auto o,QString e){done(decode(o["contact"].toObject()),e);});
}
void NativeContactProvider::local(QJsonObject request,std::function<void(QJsonObject,QString)> done){
    auto finished=std::make_shared<bool>(false);auto gen=m_generation;
    auto complete=[this,gen,finished,done](QJsonObject o,QString e){if(*finished)return;*finished=true;if(gen!=m_generation)return;
        if(e.isEmpty() && o["version"].toInt()!=1)e="Incompatible contact bridge version.";
        if(e.isEmpty())e=o["error"].toString();done(o,e);};
    if(m_provider=="outlook"){
#ifdef Q_OS_WIN
        auto p=new QProcess(this);m_process=p;p->setProgram(QCoreApplication::applicationDirPath()+"/ProjectNotesOutlookContacts.exe");
        auto output=std::make_shared<QByteArray>();connect(p,&QProcess::readyReadStandardOutput,this,[p,output]{*output+=p->readAllStandardOutput();});
        connect(p,&QProcess::started,this,[p,request]{p->write(QJsonDocument(request).toJson(QJsonDocument::Compact));p->closeWriteChannel();});
        connect(p,&QProcess::errorOccurred,this,[complete](auto){complete({},"Classic Outlook helper is unavailable.");});
        connect(p,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[p,output,complete](int code,auto){*output+=p->readAllStandardOutput();complete(QJsonDocument::fromJson(*output).object(),code?"Classic Outlook contact operation failed.":QString());p->deleteLater();});
        QTimer::singleShot(60000,p,[p,complete]{if(p->state()!=QProcess::NotRunning){p->kill();complete({},"Outlook contact operation timed out; check Outlook prompts.");}});p->start();
#else
        complete({},"Local Outlook contacts require classic Outlook on Windows. Use Office 365 for cloud contacts.");
#endif
    }else {
        auto s=new QLocalSocket(this);m_socket=s;auto output=std::make_shared<QByteArray>();
        connect(s,&QLocalSocket::connected,this,[s,request]{s->write(QJsonDocument(request).toJson(QJsonDocument::Compact)+'\n');});
        connect(s,&QLocalSocket::readyRead,this,[s,output,complete]{*output+=s->readAll();int end=output->indexOf('\n');if(end>=0){complete(QJsonDocument::fromJson(output->left(end)).object(),{});s->disconnectFromServer();s->deleteLater();}});
        connect(s,&QLocalSocket::errorOccurred,this,[s,complete](auto){complete({},"Thunderbird bridge is disconnected. Install the add-on and native host, then start Thunderbird.");s->deleteLater();});
        QTimer::singleShot(30000,s,[s,complete]{complete({},"Thunderbird contact bridge timed out.");s->abort();s->deleteLater();});
        s->connectToServer(m_config.value("bridgeSocket").toString());
    }
}
}
