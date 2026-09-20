// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MeetingTemplateEditorModel.h"

#include "TemplateParser.h"

#include <QTextDocument>
#include <QUuid>

#include <algorithm>

namespace PN::Comm {
namespace {

// Validation context for meeting templates.  It is built on the Status Report
// workflow only because every workflow-neutral field applies to it: its
// report-only fields get no value and meeting-note fields do not apply to it,
// so each field outside meetingTemplateFieldPaths() is reported as unresolved.
TemplateContext meetingExampleContext()
{
    TemplateContext context;
    context.workflow = Workflow::StatusReport;
    for (const QString &field : meetingTemplateFieldPaths())
        context.values.insert(field, QStringLiteral("Example"));
    return context;
}

QString issueText(const ValidationIssue &issue)
{
    if (!issue.displayText.isEmpty()) return issue.displayText;
    if (issue.code == QStringLiteral("template-invalid"))
        return QObject::tr("Give the meeting type a name.");
    if (issue.code == QStringLiteral("template-subject-required"))
        return QObject::tr("Enter a subject for the meeting invitation.");
    if (issue.code == QStringLiteral("template-subject-header-injection"))
        return QObject::tr("The subject must be a single line.");
    if (issue.code == QStringLiteral("template-field-unresolved"))
        return QObject::tr("\"%1\" is not a field a meeting type can use.").arg(issue.fieldPath);
    if (issue.code == QStringLiteral("template-token-unclosed"))
        return QObject::tr("A field is missing its closing }}.");
    if (issue.code == QStringLiteral("template-store-corrupt"))
        return QObject::tr("The saved meeting types could not be read.");
    if (issue.code == QStringLiteral("template-store-unavailable"))
        return QObject::tr("Open a database before editing meeting types.");
    if (issue.code == QStringLiteral("template-store-write-failed"))
        return QObject::tr("The meeting type could not be saved.");
    return issue.code;
}

bool richTextIsBlank(const QString &html)
{
    if (html.trimmed().isEmpty()) return true;
    QTextDocument document;
    document.setHtml(html);
    return document.toPlainText().trimmed().isEmpty();
}

} // namespace

MeetingTemplateEditorModel::MeetingTemplateEditorModel(QObject *parent) : QObject(parent)
{
    reload();
}

QVariantList MeetingTemplateEditorModel::templates() const
{
    QVariantList result;
    for (const MeetingTemplate &value : m_templates) {
        // Show the draft's current name so a rename is visible in the chooser
        // as soon as it is typed.
        const bool selected = value.id == m_selectedId;
        result.append(QVariantMap{{QStringLiteral("id"), value.id},
                                  {QStringLiteral("name"), selected ? m_draft.name : value.name}});
    }
    if (m_draftIsNew)
        result.append(QVariantMap{{QStringLiteral("id"), m_draft.id},
                                  {QStringLiteral("name"), m_draft.name.trimmed().isEmpty()
                                       ? tr("New meeting type") : m_draft.name}});
    return result;
}

void MeetingTemplateEditorModel::setDraftName(const QString &value) { if (m_draft.name != value) { m_draft.name = value; emit changed(); } }
void MeetingTemplateEditorModel::setDraftSubject(const QString &value) { if (m_draft.subject != value) { m_draft.subject = value; emit changed(); } }
void MeetingTemplateEditorModel::setDraftRichAgenda(const QString &value) { if (m_draft.agenda != value) { m_draft.agenda = value; emit changed(); } }

void MeetingTemplateEditorModel::setDraftDurationMinutes(int value)
{
    if (value <= 0 || m_draft.durationMinutes == value) return;
    m_draft.durationMinutes = value;
    emit changed();
}

void MeetingTemplateEditorModel::setDatabaseKey(QString databaseKey)
{
    if (m_databaseKey == databaseKey) return;
    m_databaseKey = std::move(databaseKey);
    // Meeting type ids from the prior database mean nothing in the new one.
    m_draftIsNew = false;
    m_selectedId.clear();
    reload();
}

void MeetingTemplateEditorModel::reload(const QString &preferredId)
{
    ValidationResult validation;
    m_templates = MeetingTemplateStore().templates(&validation);
    const QString wanted = preferredId.isEmpty() ? m_selectedId : preferredId;
    auto found = std::find_if(m_templates.cbegin(), m_templates.cend(),
                              [&wanted](const MeetingTemplate &value) { return value.id == wanted; });
    if (found == m_templates.cend()) found = m_templates.cbegin();
    if (found != m_templates.cend()) {
        m_draft = *found;
        m_selectedId = found->id;
    } else {
        m_draft = {};
        m_selectedId.clear();
    }
    m_draftIsNew = false;
    m_diagnostic.clear();
    // The store is unavailable before a database opens; that is not an error
    // worth showing.  A corrupt stored value is.
    for (const ValidationIssue &issue : validation.issues)
        if (issue.code == QStringLiteral("template-store-corrupt")) { m_diagnostic = issueText(issue); break; }
    emit changed();
}

bool MeetingTemplateEditorModel::selectTemplate(const QString &id)
{
    const auto found = std::find_if(m_templates.cbegin(), m_templates.cend(),
                                    [&id](const MeetingTemplate &value) { return value.id == id; });
    if (found == m_templates.cend()) return false;
    m_draft = *found;
    m_selectedId = found->id;
    m_draftIsNew = false;
    m_diagnostic.clear();
    emit changed();
    return true;
}

void MeetingTemplateEditorModel::beginNew()
{
    m_draft = {};
    m_draft.id = QStringLiteral("meeting-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_selectedId = m_draft.id;
    m_draftIsNew = true;
    m_diagnostic.clear();
    emit changed();
}

bool MeetingTemplateEditorModel::duplicateSelected()
{
    if (m_selectedId.isEmpty() || m_draftIsNew) return false;
    MeetingTemplate copy = m_draft;
    copy.id = QStringLiteral("meeting-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    copy.name = tr("%1 (copy)").arg(m_draft.name);
    const ValidationResult result = MeetingTemplateStore().save(copy);
    setDiagnostic(result);
    if (!result.ok()) return false;
    reload(copy.id);
    return true;
}

bool MeetingTemplateEditorModel::deleteSelected()
{
    if (m_selectedId.isEmpty()) return false;
    if (m_draftIsNew) {
        m_draftIsNew = false;
        m_selectedId.clear();
        reload();
        return true;
    }
    const ValidationResult result = MeetingTemplateStore().remove(m_selectedId);
    setDiagnostic(result);
    if (!result.ok()) return false;
    m_selectedId.clear();
    reload();
    return true;
}

bool MeetingTemplateEditorModel::draftIsBlank() const
{
    return m_draft.name.trimmed().isEmpty() && m_draft.subject.trimmed().isEmpty()
        && richTextIsBlank(m_draft.agenda);
}

ValidationResult MeetingTemplateEditorModel::validate() const
{
    ValidationResult result;
    if (m_selectedId.isEmpty()) return result;
    if (m_draft.name.trimmed().isEmpty()) result.addError("template-invalid", "name");
    result.issues += validateTemplateSubject(m_draft.subject).issues;
    const TemplateContext context = meetingExampleContext();
    result.issues += renderTemplateFields(m_draft.subject, context).validation.issues;
    result.issues += renderTemplateFields(m_draft.agenda, context, TemplateRenderMode::Html).validation.issues;
    return result;
}

void MeetingTemplateEditorModel::setDiagnostic(const ValidationResult &result)
{
    m_diagnostic = result.issues.isEmpty() ? QString() : issueText(result.issues.constFirst());
    emit changed();
}

bool MeetingTemplateEditorModel::validateDraft()
{
    const ValidationResult result = validate();
    setDiagnostic(result);
    return result.ok();
}

bool MeetingTemplateEditorModel::save()
{
    if (m_selectedId.isEmpty()) return true;
    if (m_draftIsNew && draftIsBlank()) {
        // Nothing was entered after New; drop it rather than store a nameless row.
        m_draftIsNew = false;
        m_selectedId.clear();
        reload();
        return true;
    }
    // Store incomplete drafts so nothing typed is lost.  Preview problems stay
    // visible in the diagnostic, as with email templates.
    const ValidationResult draftValidation = validate();
    const ValidationResult result = MeetingTemplateStore().save(m_draft);
    setDiagnostic(result);
    if (!result.ok()) return false;
    reload(m_draft.id);
    if (!draftValidation.ok())
        setDiagnostic(draftValidation);
    return true;
}

bool MeetingTemplateEditorModel::appendField(const QString &path, const QString &target)
{
    if (m_selectedId.isEmpty() || !meetingTemplateFieldPaths().contains(path)) return false;
    const QString token = QStringLiteral("{{ %1 }}").arg(path);
    if (target == QStringLiteral("subject")) m_draft.subject += token;
    else if (target == QStringLiteral("richAgenda")) m_draft.agenda += token;
    else return false;
    emit changed();
    return true;
}

std::optional<MeetingTemplate> MeetingTemplateEditorModel::templateById(const QString &id) const
{
    const auto found = std::find_if(m_templates.cbegin(), m_templates.cend(),
                                    [&id](const MeetingTemplate &value) { return value.id == id; });
    return found == m_templates.cend() ? std::nullopt : std::optional<MeetingTemplate>(*found);
}

} // namespace PN::Comm
