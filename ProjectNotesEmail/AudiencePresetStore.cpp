#include "AudiencePresetStore.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
namespace PN::Comm {
namespace {
QJsonObject encode(const AudiencePreset &p) { QJsonArray o; for(const auto &v:p.overrides)o.append(QJsonObject{{"person",v.personId},{"selected",v.selected},{"role",static_cast<int>(v.role)}}); return {{"id",p.id},{"name",p.name},{"project",p.projectId},{"workflow",toStableString(p.workflow)},{"source",static_cast<int>(p.rule.source)},{"company",static_cast<int>(p.rule.companyFilter)},{"companies",QJsonArray::fromStringList(p.rule.companyIds)},{"people",QJsonArray::fromStringList(p.rule.chosenPersonIds)},{"unknown",p.rule.includeUnknownCompany},{"manager",p.rule.excludeProjectManager},{"overrides",o}}; }
std::optional<AudiencePreset> decode(const QJsonObject&o){auto w=workflowFromStableString(o["workflow"].toString()); const int s=o["source"].toInt(-1),c=o["company"].toInt(-1);if(!w||o["id"].toString().isEmpty()||s<0||s>4||c<0||c>4)return{}; AudiencePreset p{o["id"].toString(),o["name"].toString(),o["project"].toString(),*w};p.rule.source=static_cast<PeopleSource>(s);p.rule.companyFilter=static_cast<CompanyFilter>(c);for(auto v:o["companies"].toArray())p.rule.companyIds.append(v.toString());for(auto v:o["people"].toArray())p.rule.chosenPersonIds.append(v.toString());p.rule.includeUnknownCompany=o["unknown"].toBool();p.rule.excludeProjectManager=o["manager"].toBool(true);for(auto v:o["overrides"].toArray()){const auto x=v.toObject();int role=x["role"].toInt(-1);if(role>=0&&role<=2)p.overrides.append({x["person"].toString(),x["selected"].toBool(true),static_cast<RecipientRole>(role)});}return p;}
}
AudiencePresetStore::AudiencePresetStore(QSettings&s,QString db):m_settings(s),m_databaseKey(std::move(db)){m_settings.setFallbacksEnabled(false);}
QString AudiencePresetStore::key()const{return "Email/v1/Audiences/"+(m_databaseKey.isEmpty()?"global":m_databaseKey)+"/Presets";}
QString AudiencePresetStore::defaultKey(Workflow w,const QString&p)const{return "Email/v1/Audiences/"+(m_databaseKey.isEmpty()?"global":m_databaseKey)+"/Defaults/"+toStableString(w)+(p.isEmpty()?QString():"/"+p);}
QList<AudiencePreset> AudiencePresetStore::presets()const{QList<AudiencePreset>r;auto d=QJsonDocument::fromJson(m_settings.value(key()).toByteArray());for(auto v:d.array())if(auto p=decode(v.toObject()))r.append(*p);return r;}
ValidationResult AudiencePresetStore::save(AudiencePreset p){ValidationResult r;if(p.id.trimmed().isEmpty()||p.name.trimmed().isEmpty()){r.addError("audience-preset-invalid","preset");return r;}auto all=presets();bool replaced=false;for(auto &e:all)if(e.id==p.id){e=p;replaced=true;}if(!replaced)all.append(p);QJsonArray a;for(auto&e:all)a.append(encode(e));m_settings.setValue(key(),QJsonDocument(a).toJson(QJsonDocument::Compact));m_settings.sync();return r;}
std::optional<AudiencePreset> AudiencePresetStore::defaultFor(Workflow w,const QString&p)const{
    const auto matchingPreset = [w, &p, this](const QString &id) -> std::optional<AudiencePreset> {
        for (const AudiencePreset &preset : presets())
            if (preset.id == id && preset.workflow == w
                && (preset.projectId.isEmpty() || preset.projectId == p))
                return preset;
        return {};
    };
    const QString scopedId=m_settings.value(defaultKey(w,p)).toString();
    if (const auto scoped = matchingPreset(scopedId)) return scoped;
    if (!p.isEmpty()) return matchingPreset(m_settings.value(defaultKey(w,{})).toString());
    return {};
}
ValidationResult AudiencePresetStore::setDefault(const QString&id,Workflow w,const QString&p){
    ValidationResult r; bool found=false; bool scopeMatches=false;
    for(const AudiencePreset &preset:presets()) if(preset.id==id&&preset.workflow==w) {
        found=true; scopeMatches = preset.projectId.isEmpty() || (!p.isEmpty() && preset.projectId==p);
    }
    if(!found){r.addError("audience-preset-not-found","presetId");return r;}
    if(!scopeMatches){r.addError("audience-preset-scope-mismatch","presetId");return r;}
    m_settings.setValue(defaultKey(w,p),id);m_settings.sync();return r;
}
}
