// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "AudiencePresetStore.h"
#include "RecipientSelectionModel.h"

#include <QObject>
#include <QVariantList>

namespace PN::Comm {

// Every generated report starts from the project team, excluding the project
// manager, before any saved audience preset is applied.
AudienceRule projectTeamAudienceRule();
// The rule a saved audience resolves with when a workflow loads it.
AudienceRule reviewPresetRule(Workflow workflow, const CommunicationSnapshot &snapshot, const AudiencePreset &preset);

// A recipient choice made outside a native review, for a plugin's
// email.chooseRecipients(). It owns its own snapshot and selection model, so it
// never disturbs a report review that is open at the same time. Its invokables
// carry the names DesktopAppController gives the review's, so
// RecipientSelectionPane drives either one unchanged.
class AudienceReviewSession final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *recipientModel READ recipientModel CONSTANT)
public:
    AudienceReviewSession(CommunicationSnapshot snapshot, Workflow workflow, QObject *parent = nullptr);

    [[nodiscard]] RecipientSelectionModel *model() const { return m_model; }
    [[nodiscard]] QObject *recipientModel() const { return m_model; }
    [[nodiscard]] Workflow workflow() const { return m_workflow; }
    [[nodiscard]] QString appliedPresetId() const { return m_presetId; }

    Q_INVOKABLE QVariantList reviewAudiencePresets() const;
    Q_INVOKABLE bool saveReviewAudiencePreset(const QString &name);
    Q_INVOKABLE bool applyReviewAudiencePreset(const QString &presetId);
    Q_INVOKABLE bool setReviewAudiencePresetDefault(const QString &presetId);
    // Empty when the selection can be used; otherwise the reason it cannot.
    Q_INVOKABLE QString problem() const;

    [[nodiscard]] RecipientResolution resolve() const { return m_model->resolve(); }

signals:
    void reviewAudiencePresetChanged();

private:
    void applyPreset(const AudiencePreset &preset);
    void setAppliedPreset(const QString &presetId);

    CommunicationSnapshot m_snapshot;
    Workflow m_workflow;
    RecipientSelectionModel *m_model = nullptr;
    AudienceRule m_rule;
    QString m_presetId;
};

} // namespace PN::Comm
