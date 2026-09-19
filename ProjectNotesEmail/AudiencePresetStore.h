#pragma once
#include "RecipientAudienceResolver.h"
namespace PN::Comm {
struct AudiencePreset {
    QString id, name, projectId;
    Workflow workflow = Workflow::SendMeetingNotes;
    AudienceRule rule;
    QList<RecipientOverride> overrides;
    bool isDefault = false;
};
// Database-backed only. Legacy QSettings audience values are intentionally ignored.
class AudiencePresetStore final {
public:
    QList<AudiencePreset> presets(const QString &projectId, Workflow workflow,
                                  ValidationResult *validation = nullptr) const;
    ValidationResult save(AudiencePreset preset);
    std::optional<AudiencePreset> defaultFor(Workflow workflow, const QString &projectId) const;
    ValidationResult setDefault(const QString &presetId, Workflow workflow, const QString &projectId);
};
}
