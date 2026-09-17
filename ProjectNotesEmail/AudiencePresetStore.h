#pragma once
#include "RecipientAudienceResolver.h"
#include <QSettings>
namespace PN::Comm {
struct AudiencePreset { QString id,name,projectId; Workflow workflow=Workflow::SendMeetingNotes; AudienceRule rule; QList<RecipientOverride> overrides; };
class AudiencePresetStore final {
public:
    AudiencePresetStore(QSettings &settings, QString databaseKey);
    QList<AudiencePreset> presets() const;
    ValidationResult save(AudiencePreset preset);
    std::optional<AudiencePreset> defaultFor(Workflow workflow, const QString &projectId = {}) const;
    ValidationResult setDefault(const QString &presetId, Workflow workflow, const QString &projectId = {});
private:
    QString key() const; QString defaultKey(Workflow workflow, const QString &projectId) const;
    QSettings &m_settings; QString m_databaseKey;
};
}
