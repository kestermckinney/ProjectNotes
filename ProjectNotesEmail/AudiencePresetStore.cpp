#include "AudiencePresetStore.h"
#include "databaseobjects.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QUuid>

namespace PN::Comm {
namespace {
QString sourceName(PeopleSource v) { static const QStringList n{"project-team","meeting-attendees","status-recipients","current-selection","chosen-people"}; return n.value(static_cast<int>(v)); }
QString companyName(CompanyFilter v) { static const QStringList n{"all","managing-company","project-client","except-project-client","selected-companies"}; return n.value(static_cast<int>(v)); }
std::optional<PeopleSource> sourceFromName(const QString &v) { const int i=QStringList{"project-team","meeting-attendees","status-recipients","current-selection","chosen-people"}.indexOf(v); return i<0?std::optional<PeopleSource>{}:static_cast<PeopleSource>(i); }
std::optional<CompanyFilter> companyFromName(const QString &v) { const int i=QStringList{"all","managing-company","project-client","except-project-client","selected-companies"}.indexOf(v); return i<0?std::optional<CompanyFilter>{}:static_cast<CompanyFilter>(i); }
QString roleName(RecipientRole v) { static const QStringList n{"to","cc","bcc"}; return n.value(static_cast<int>(v)); }
std::optional<RecipientRole> roleFromName(const QString &v) { const int i=QStringList{"to","cc","bcc"}.indexOf(v); return i<0?std::optional<RecipientRole>{}:static_cast<RecipientRole>(i); }
QString listJson(const QStringList &v) { return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(v)).toJson(QJsonDocument::Compact)); }
QString overridesJson(const QList<RecipientOverride> &values) { QJsonArray a; for(const auto &v:values)a.append(QJsonObject{{"personId",v.personId},{"selected",v.selected},{"role",roleName(v.role)}}); return QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact)); }
QStringList decodeList(const QString &json,bool *ok) { const auto d=QJsonDocument::fromJson(json.toUtf8()); if(!d.isArray()){*ok=false;return{};} QStringList r; for(const auto &v:d.array()){if(!v.isString()){*ok=false;return{};}r.append(v.toString());} return r; }
QList<RecipientOverride> decodeOverrides(const QString &json,bool *ok) { const auto d=QJsonDocument::fromJson(json.toUtf8()); if(!d.isArray()){*ok=false;return{};} QList<RecipientOverride> r; for(const auto &v:d.array()){const auto o=v.toObject();const auto role=roleFromName(o["role"].toString());const auto id=o["personId"].toString();if(!role||id.isEmpty()){*ok=false;return{};}r.append({id,o["selected"].toBool(true),*role});}return r; }
void error(ValidationResult *v,const QString &code){if(v)v->addError(code,"audience");}
}

QList<AudiencePreset> AudiencePresetStore::presets(const QString &projectId,Workflow workflow,ValidationResult *validation) const
{
    QList<AudiencePreset> result;
    if(!global_DBObjects.isOpen()||projectId.trimmed().isEmpty()){error(validation,"audience-store-unavailable");return result;}
    QSqlQuery q(global_DBObjects.getDb());
    q.prepare("SELECT id,audience_name,people_source,company_filter,selected_company_ids_json,selected_person_ids_json,recipient_distribution_json,is_default,settings_version FROM project_email_audiences WHERE project_id=? AND workflow=? AND deleted=0 ORDER BY audience_name COLLATE NOCASE,id");
    q.addBindValue(projectId);q.addBindValue(toStableString(workflow));
    if(!q.exec()){error(validation,"audience-store-read-failed");return result;}
    while(q.next()){
        const auto source=sourceFromName(q.value(2).toString());const auto company=companyFromName(q.value(3).toString());bool ok=source&&company&&q.value(8).toInt()==1;
        AudiencePreset p; p.id=q.value(0).toString();p.name=q.value(1).toString();p.projectId=projectId;p.workflow=workflow;p.isDefault=q.value(7).toBool();
        if(source)p.rule.source=*source;if(company)p.rule.companyFilter=*company;
        p.rule.companyIds=decodeList(q.value(4).toString(),&ok);p.rule.chosenPersonIds=decodeList(q.value(5).toString(),&ok);p.overrides=decodeOverrides(q.value(6).toString(),&ok);
        p.rule.source=PeopleSource::ProjectTeam;p.rule.chosenPersonIds.clear();p.rule.companyFilter=CompanyFilter::All;p.rule.companyIds.clear();p.rule.includeUnknownCompany=true;p.rule.excludeProjectManager=true;
        if(!ok||p.id.isEmpty()||p.name.isEmpty()){error(validation,"audience-store-corrupt");continue;}result.append(std::move(p));
    }
    return result;
}

ValidationResult AudiencePresetStore::save(AudiencePreset p)
{
    ValidationResult r;p.name=p.name.trimmed();
    if(!global_DBObjects.isOpen()||p.projectId.trimmed().isEmpty()||p.name.isEmpty()){r.addError("audience-preset-invalid","preset");return r;}
    p.rule.source=PeopleSource::ProjectTeam;p.rule.chosenPersonIds.clear();p.rule.companyFilter=CompanyFilter::All;p.rule.companyIds.clear();p.rule.includeUnknownCompany=true;p.rule.excludeProjectManager=true;
    QSqlQuery old(global_DBObjects.getDb());old.prepare("SELECT id,is_default FROM project_email_audiences WHERE project_id=? AND workflow=? AND audience_name=? COLLATE NOCASE AND deleted=0");old.addBindValue(p.projectId);old.addBindValue(toStableString(p.workflow));old.addBindValue(p.name);
    if(!old.exec()){r.addError("audience-store-read-failed","preset");return r;}if(old.next()){p.id=old.value(0).toString();p.isDefault=old.value(1).toBool();}if(p.id.isEmpty())p.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlQuery q(global_DBObjects.getDb());q.prepare("INSERT INTO project_email_audiences(id,project_id,workflow,audience_name,people_source,company_filter,selected_company_ids_json,selected_person_ids_json,recipient_distribution_json,is_default,settings_version,updateddate,deleted) VALUES(?,?,?,?,?,?,?,?,?,?,1,CAST(strftime('%s','now') AS INTEGER),0) ON CONFLICT(id) DO UPDATE SET audience_name=excluded.audience_name,people_source=excluded.people_source,company_filter=excluded.company_filter,selected_company_ids_json=excluded.selected_company_ids_json,selected_person_ids_json=excluded.selected_person_ids_json,recipient_distribution_json=excluded.recipient_distribution_json,deleted=0");
    q.addBindValue(p.id);q.addBindValue(p.projectId);q.addBindValue(toStableString(p.workflow));q.addBindValue(p.name);q.addBindValue(sourceName(p.rule.source));q.addBindValue(companyName(p.rule.companyFilter));q.addBindValue(listJson(p.rule.companyIds));q.addBindValue(listJson(p.rule.chosenPersonIds));q.addBindValue(overridesJson(p.overrides));q.addBindValue(p.isDefault?1:0);
    if(!q.exec())r.addError("audience-store-write-failed","preset");return r;
}

std::optional<AudiencePreset> AudiencePresetStore::defaultFor(Workflow workflow,const QString &projectId) const { for(const auto &p:presets(projectId,workflow))if(p.isDefault)return p;return{}; }

ValidationResult AudiencePresetStore::setDefault(const QString &id,Workflow workflow,const QString &projectId)
{
    ValidationResult r;if(!global_DBObjects.isOpen()||id.trimmed().isEmpty()||projectId.trimmed().isEmpty()){r.addError("audience-preset-invalid","presetId");return r;}
    auto &db=global_DBObjects.getDb();if(!db.transaction()){r.addError("audience-store-write-failed","presetId");return r;}
    QSqlQuery clear(db);clear.prepare("UPDATE project_email_audiences SET is_default=0 WHERE project_id=? AND workflow=? AND deleted=0");clear.addBindValue(projectId);clear.addBindValue(toStableString(workflow));
    QSqlQuery set(db);set.prepare("UPDATE project_email_audiences SET is_default=1 WHERE id=? AND project_id=? AND workflow=? AND deleted=0");set.addBindValue(id);set.addBindValue(projectId);set.addBindValue(toStableString(workflow));
    if(!clear.exec()||!set.exec()||set.numRowsAffected()!=1||!db.commit()){db.rollback();r.addError("audience-store-write-failed","presetId");}return r;
}
}
