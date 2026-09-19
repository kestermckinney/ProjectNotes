#pragma once
#include "RecipientAudienceResolver.h"
namespace PN::Comm {
// A named audience shared by every report of one project. defaultFor lists the
// reports that load it automatically; each report has at most one default.
struct AudiencePreset {
    QString id, name, projectId;
    AudienceRule rule;
    QList<RecipientOverride> overrides;
    QList<Workflow> defaultFor;
};
// Database-backed only. Legacy QSettings audience values are intentionally ignored.
class AudiencePresetStore final {
public:
    QList<AudiencePreset> presets(const QString &projectId, ValidationResult *validation = nullptr) const;
    ValidationResult save(AudiencePreset preset);
    std::optional<AudiencePreset> defaultFor(Workflow workflow, const QString &projectId) const;
    ValidationResult setDefault(const QString &presetId, Workflow workflow, const QString &projectId);
};
}
