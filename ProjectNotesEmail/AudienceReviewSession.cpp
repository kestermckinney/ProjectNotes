// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "AudienceReviewSession.h"

#include <QUuid>

namespace PN::Comm {

AudienceRule projectTeamAudienceRule()
{
    AudienceRule rule;
    rule.source = ContactSource::ProjectTeam;
    rule.excludeProjectManager = true;
    return rule;
}

AudienceRule reviewPresetRule(Workflow workflow, const CommunicationSnapshot &snapshot, const AudiencePreset &preset)
{
    auto rule = preset.rule;
    if (workflow != Workflow::ProjectEmail) return rule;
    // Saved report audiences default to team members excluding the manager.
    // Project emails can explicitly select a manager or meeting guest too;
    // retain those IDs and their overrides without changing report behavior.
    rule.source = ContactSource::ChosenContacts;
    rule.excludeProjectManager = false;
    for (const auto &contact : snapshot.contacts) {
        bool available = contact.projectTeamMember && contact.id != snapshot.projectManagerId;
        for (const auto &overrideValue : preset.overrides)
            available |= overrideValue.contactId == contact.id;
        if (available) rule.chosenContactIds.append(contact.id);
    }
    rule.chosenContactIds.removeDuplicates();
    return rule;
}

AudienceReviewSession::AudienceReviewSession(CommunicationSnapshot snapshot, Workflow workflow, QObject *parent)
    : QObject(parent), m_snapshot(std::move(snapshot)), m_workflow(workflow),
      m_model(new RecipientSelectionModel(this)), m_rule(projectTeamAudienceRule())
{
    // The same starting point as the native reviews: the project team, with the
    // status and tracker reports preselecting those who receive status reports.
    AudienceResolution audience = resolveAudience(m_snapshot, m_rule);
    if (m_workflow == Workflow::StatusReport || m_workflow == Workflow::TrackerItemsReport)
        for (const SnapshotContact &contact : audience.contacts)
            if (!contact.receivesStatus)
                audience.initialOverrides.append({contact.id, false, RecipientRole::To});
    m_model->setAudience(std::move(audience));
    if (const auto preset = AudiencePresetStore().defaultFor(m_workflow, m_snapshot.projectId))
        applyPreset(*preset);
}

QVariantList AudienceReviewSession::reviewAudiencePresets() const
{
    QVariantList result;
    for (const AudiencePreset &preset : AudiencePresetStore().presets(m_snapshot.projectId))
        result.append(QVariantMap{{QStringLiteral("id"), preset.id},
                                  {QStringLiteral("name"), preset.name},
                                  {QStringLiteral("reportDefault"), preset.defaultFor.contains(m_workflow)},
                                  {QStringLiteral("applied"), preset.id == m_presetId}});
    return result;
}

bool AudienceReviewSession::saveReviewAudiencePreset(const QString &name)
{
    if (m_snapshot.projectId.isEmpty() || name.trimmed().isEmpty())
        return false;
    AudiencePreset preset;
    preset.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    preset.name = name.trimmed();
    preset.projectId = m_snapshot.projectId;
    preset.rule = m_rule;
    preset.overrides = m_model->overrides();
    AudiencePresetStore store;
    if (!store.save(preset).ok())
        return false;
    // Saving over an existing name keeps that audience's ID.
    for (const AudiencePreset &saved : store.presets(m_snapshot.projectId))
        if (saved.name.compare(preset.name, Qt::CaseInsensitive) == 0)
            setAppliedPreset(saved.id);
    return true;
}

bool AudienceReviewSession::applyReviewAudiencePreset(const QString &presetId)
{
    if (presetId.trimmed().isEmpty())
        return false;
    for (const AudiencePreset &preset : AudiencePresetStore().presets(m_snapshot.projectId)) {
        if (preset.id == presetId) {
            applyPreset(preset);
            return true;
        }
    }
    return false;
}

bool AudienceReviewSession::setReviewAudiencePresetDefault(const QString &presetId)
{
    if (presetId.trimmed().isEmpty())
        return false;
    return AudiencePresetStore().setDefault(presetId, m_workflow, m_snapshot.projectId).ok();
}

QString AudienceReviewSession::problem() const
{
    const RecipientResolution resolution = m_model->resolve();
    for (const ValidationIssue &issue : resolution.validation.issues) {
        if (issue.severity != IssueSeverity::Error)
            continue;
        if (issue.code == QLatin1String("recipient-role-conflict"))
            return tr("The same address is selected as more than one of To, Cc and Bcc.");
        if (issue.code == QLatin1String("recipient-address-invalid"))
            return tr("A selected recipient has an email address that cannot be used.");
        return issue.displayText.isEmpty() ? issue.code : issue.displayText;
    }
    if (resolution.recipients.isEmpty())
        return tr("Select at least one recipient.");
    return {};
}

void AudienceReviewSession::applyPreset(const AudiencePreset &preset)
{
    m_rule = reviewPresetRule(m_workflow, m_snapshot, preset);
    m_model->setAudience(resolveAudience(m_snapshot, m_rule));
    m_model->applyOverrides(preset.overrides);
    setAppliedPreset(preset.id);
}

void AudienceReviewSession::setAppliedPreset(const QString &presetId)
{
    m_presetId = presetId;
    emit reviewAudiencePresetChanged();
}

} // namespace PN::Comm
