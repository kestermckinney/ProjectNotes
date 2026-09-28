// SPDX-License-Identifier: GPL-3.0-only
#include "ContactRepository.h"
#include "ContactMatcher.h"
#include "databaseobjects.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QWriteLocker>
#include <QUuid>
namespace PN::Contacts {
QSqlDatabase ContactRepository::database() const {return m_database?m_database->getDb():m_connection;}
bool ContactRepository::open(const QString &path,QString &error) {
    close();m_connectionName="ProjectNotesContacts-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_connection=QSqlDatabase::addDatabase("QSQLITE",m_connectionName);
    m_connection.setDatabaseName(path);m_connection.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    if(!m_connection.open()){error=m_connection.lastError().text();close();return false;}return true;
}
void ContactRepository::close(){
    if(m_connectionName.isEmpty())return;
    m_connection.close();m_connection=QSqlDatabase();QSqlDatabase::removeDatabase(m_connectionName);m_connectionName.clear();
}
bool ContactRepository::read(QList<Person>&people,QList<Client>&clients,QString&error) const {
    QReadLocker lock(&db_rwlock); QSqlQuery q(database());
    if(!q.exec("SELECT id,client_name,deleted FROM clients")) {error=q.lastError().text();return false;}
    while(q.next()) clients.append({q.value(0).toString(),q.value(1).toString(),q.value(2).toBool()});
    if(!q.exec("SELECT p.id,p.name,p.email,p.office_phone,p.cell_phone,p.client_id,p.role,p.deleted,c.client_name FROM people p LEFT JOIN clients c ON p.client_id=c.id")) {error=q.lastError().text();return false;}
    while(q.next()) {
        Person p; p.id=q.value(0).toString();p.name=q.value(1).toString();p.clientId=q.value(5).toString();
        p.company=q.value(8).toString();p.deleted=q.value(7).toBool();
        for(int i=1;i<=6;++i) p.fields.insert(q.record().fieldName(i),q.value(i).toString());
        people.append(p);
    } return true;
}
bool ContactRepository::createClient(const QString &id,const QString &name,QString &error) {
    QWriteLocker lock(&db_rwlock);auto db=database();
    if(!db.transaction()){error=db.lastError().text();return false;}
    QSqlQuery q(db);q.prepare("SELECT client_name,deleted FROM clients WHERE id=?");q.addBindValue(id);
    if(!q.exec()){error=q.lastError().text();db.rollback();return false;}
    if(q.next()) {
        if(q.value(1).toBool()||q.value(0).toString()!=name){error="The client changed. Sync again before reviewing it.";db.rollback();return false;}
    } else {
        q.prepare("INSERT INTO clients(id,client_name) VALUES(?,?)");q.addBindValue(id);q.addBindValue(name);
        if(!q.exec()){error=q.lastError().text();db.rollback();return false;}
    }
    if(!db.commit()){error=db.lastError().text();db.rollback();return false;}
    lock.unlock();if(onChanged)onChanged();return true;
}
bool ContactRepository::apply(const Contact&c,const QString&id,const QString&clientId,const QString&newClientId,
                              const QString&displayName,const QVariantMap&expected,QString&error,bool reviewed,bool linked) {
    QWriteLocker lock(&db_rwlock); auto db=database();
    if(!db.transaction()){error=db.lastError().text();return false;}
    auto fail=[&](const QString&e){error=e;db.rollback();return false;};
    QSqlQuery q(db); q.prepare("SELECT name,email,office_phone,cell_phone,client_id,role,deleted FROM people WHERE id=?");q.addBindValue(id);
    if(!q.exec()) return fail(q.lastError().text());
    bool exists=q.next(); QVariantMap fields;
    if(exists) {
        if(q.value(6).toBool()) return fail("The linked person was deleted; sync will not restore it.");
        for(int i=0;i<6;++i) fields.insert(q.record().fieldName(i),q.value(i).toString());
        if(!expected.isEmpty() && fields!=expected) return fail("The contact changed while sync was running. Sync again.");
    } else if(!expected.isEmpty()) return fail("The matched contact no longer exists.");
    // Revalidate the candidate set under the same lock as the write. Another
    // sync writer may have inserted a matching person/company after our preview.
    QList<Client> clients;QList<Person> people;
    if(!q.exec("SELECT id,client_name,deleted FROM clients"))return fail(q.lastError().text());
    while(q.next())clients.append({q.value(0).toString(),q.value(1).toString(),q.value(2).toBool()});
    if(!q.exec("SELECT id,name,email,office_phone,cell_phone,client_id,role,deleted FROM people"))return fail(q.lastError().text());
    while(q.next()){
        Person p;p.id=q.value(0).toString();p.name=q.value(1).toString();p.clientId=q.value(5).toString();p.deleted=q.value(7).toBool();
        for(const auto&client:clients)if(client.id==p.clientId)p.company=client.name;
        for(int i=1;i<=6;++i)p.fields[q.record().fieldName(i)]=q.value(i).toString();people<<p;
        if(!p.deleted&&p.id!=id&&ContactMatcher::normalize(p.name)==ContactMatcher::normalize(displayName))return fail("Another person already uses this name. Choose a unique display name in Review Matches.");
    }
    if(!reviewed&&!newClientId.isEmpty()){
        auto match=ContactMatcher::company(c.company,clients);
        if(match.review||(!match.id.isEmpty()&&match.id!=newClientId))return fail("A matching company appeared during sync. Review the company match.");
    }
    if(!reviewed&&!linked){
        auto match=ContactMatcher::person(c,clientId.isEmpty()?newClientId:clientId,people);
        if(match.review||(!match.id.isEmpty()&&match.id!=id))return fail("The person match changed during sync. Review the candidates.");
    }
    for(auto it=c.fields.begin();it!=c.fields.end();++it) if(QStringList{"name","email","office_phone","cell_phone","role"}.contains(it.key())) fields[it.key()]=it.value();
    fields["name"]=displayName;
    if(c.fields.contains("company")) fields["client_id"]=clientId.isEmpty()?newClientId:clientId;
    if(displayName.trimmed().isEmpty()) return fail("A person name is required.");
    q.prepare("SELECT id FROM people WHERE deleted=0 AND lower(trim(name))=lower(trim(?)) AND id<>?");q.addBindValue(displayName);q.addBindValue(id);
    if(!q.exec())return fail(q.lastError().text());if(q.next())return fail("Another person already uses this name. Choose a unique display name in Review Matches.");
    if(!newClientId.isEmpty()) {
        q.prepare("SELECT id,deleted FROM clients WHERE id=? OR (deleted=0 AND lower(trim(client_name))=lower(trim(?)))");q.addBindValue(newClientId);q.addBindValue(c.company);
        if(!q.exec())return fail(q.lastError().text());
        if(q.next()) {if(q.value(0).toString()!=newClientId || q.value(1).toBool())return fail("The company changed while sync was running. Sync again.");}
        else {q.prepare("INSERT INTO clients(id,client_name) VALUES(?,?)");q.addBindValue(newClientId);q.addBindValue(c.company);if(!q.exec())return fail(q.lastError().text());}
    } else if(!clientId.isEmpty()) {
        q.prepare("SELECT id FROM clients WHERE id=? AND deleted=0");q.addBindValue(clientId);
        if(!q.exec() || !q.next())return fail("The matched company is no longer available.");
    }
    if(exists && fields==expected) {db.rollback();return true;}
    q.prepare(exists?"UPDATE people SET name=?,email=?,office_phone=?,cell_phone=?,client_id=?,role=? WHERE id=? AND deleted=0":
                        "INSERT INTO people(name,email,office_phone,cell_phone,client_id,role,id) VALUES(?,?,?,?,?,?,?)");
    for(const auto&key:QStringList{"name","email","office_phone","cell_phone","client_id","role"}) {
        QString v=fields.value(key).toString();q.addBindValue(v.isEmpty()?QVariant(QMetaType::fromType<QString>()):QVariant(v));
    }q.addBindValue(id);
    if(!q.exec())return fail(q.lastError().text());
    if(!db.commit())return fail(db.lastError().text());
    lock.unlock();
    if(onChanged)onChanged();
    else if(m_database)for(auto *model:m_database->getOpenModels()) if(model->tablename()=="people" || model->tablename()=="clients")model->refresh();
    return true;
}
}
