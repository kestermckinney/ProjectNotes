// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "TemplateEditorModel.h"

#include "TemplateApplication.h"
#include "TemplateParser.h"

#include <QRegularExpression>

#include <algorithm>

namespace PN::Comm {
namespace {

TemplateContext exampleContext(Workflow workflow)
{
    TemplateContext context;
    context.workflow = workflow;
    for (const QString &field : templateFieldPaths(workflow))
        context.values.insert(field, QStringLiteral("Example"));
    return context;
}

QList<CommunicationTemplate> bundledTemplates()
{
    // This is intentionally an exact native-content wrapper. It gives the
    // editor usable/resettable native baselines.  Each wrapper leaves rendered
    // report HTML in the protected content block rather than interpolating it.
    return {{QStringLiteral("send-meeting-notes-native-v1"),
             QObject::tr("Native meeting notes"), Workflow::SendMeetingNotes,
             QStringLiteral("{{ project.number }} {{ project.name }} - {{ meeting.date }} {{ meeting.title }} Notes"),
             QStringLiteral("{{ content.body }}")},
            {QStringLiteral("meeting-notes-report-native-v1"),
             QObject::tr("Native meeting notes report"), Workflow::MeetingNotesReport,
             QStringLiteral("{{ project.number }} {{ project.name }} - {{ meeting.date }}"),
             QStringLiteral("{{ content.body }}")},
            {QStringLiteral("status-report-native-v1"),
             QObject::tr("Native status report"), Workflow::StatusReport,
             QStringLiteral("{{ project.number }} {{ project.name }} - Status Report {{ report.date }}"),
             QStringLiteral("{{ content.body }}")},
            {QStringLiteral("tracker-items-report-native-v1"),
             QObject::tr("Native tracker items report"), Workflow::TrackerItemsReport,
             QStringLiteral("{{ project.number }} {{ project.name }} - Tracker Items {{ report.date }}"),
             QStringLiteral("{{ content.body }}")}};
}

QString nativeTemplateId(Workflow workflow)
{
    switch (workflow) {
    case Workflow::SendMeetingNotes: return QStringLiteral("send-meeting-notes-native-v1");
    case Workflow::MeetingNotesReport: return QStringLiteral("meeting-notes-report-native-v1");
    case Workflow::StatusReport: return QStringLiteral("status-report-native-v1");
    case Workflow::TrackerItemsReport: return QStringLiteral("tracker-items-report-native-v1");
    }
    return {};
}

} // namespace

TemplateEditorModel::TemplateEditorModel(QString organization, QString databaseKey, QObject *parent)
    : QObject(parent), m_settings(std::make_unique<QSettings>(std::move(organization), QStringLiteral("AppSettings"))),
      m_databaseKey(std::move(databaseKey))
{
    m_settings->setFallbacksEnabled(false);
    reload();
}

QString TemplateEditorModel::workflow() const { return toStableString(m_workflow); }

void TemplateEditorModel::setWorkflow(const QString &workflow)
{
    const auto parsed = workflowFromStableString(workflow);
    if (!parsed || *parsed == m_workflow) return;
    m_workflow = *parsed;
    // A template id is scoped to its workflow. Keeping the old selected id
    // makes reload() treat it as an explicit (but unavailable) selection and
    // leaves the editor draft empty instead of selecting the new workflow's
    // native baseline.
    m_selectedId.clear();
    reload();
}

QVariantList TemplateEditorModel::templates() const
{
    QVariantList result;
    for (const CommunicationTemplate &templateValue : m_templates) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), templateValue.id);
        row.insert(QStringLiteral("name"), templateValue.name);
        row.insert(QStringLiteral("bundled"), templateValue.bundled);
        result.append(row);
    }
    return result;
}

QStringList TemplateEditorModel::availableFields() const { return templateFieldPaths(m_workflow); }

void TemplateEditorModel::setDraftName(const QString &value) { if (m_draft.name != value) { m_draft.name = value; emit changed(); } }
void TemplateEditorModel::setDraftSubject(const QString &value) { if (m_draft.subject != value) { m_draft.subject = value; emit changed(); } }
void TemplateEditorModel::setDraftRichBody(const QString &value) { if (m_draft.body != value) { m_draft.body = value; emit changed(); } }
void TemplateEditorModel::setDraftPlainBody(const QString &value) { if (m_draftPlainBody != value) { m_draftPlainBody = value; emit changed(); } }

void TemplateEditorModel::setDatabaseKey(QString databaseKey)
{
    if (m_databaseKey == databaseKey) return;
    m_databaseKey = std::move(databaseKey);
    // A user-created id from the prior database is not meaningful in the new
    // settings scope.  Reload should select that scope's bundled baseline,
    // never retain an unavailable id and expose an empty editor draft.
    m_selectedId.clear();
    reload();
}

Workflow TemplateEditorModel::currentWorkflow() const { return m_workflow; }

void TemplateEditorModel::reload(const QString &preferredId)
{
    CommunicationTemplateStore store(*m_settings, m_databaseKey, bundledTemplates());
    // Native workflows are the four fixed replacements for the bundled Python
    // plug-ins.  A workflow has one editable wrapper, identified by its native
    // id; it is not a user-extensible list of workflows/templates.  Older
    // versions allowed extra rows here, which made the settings screen show
    // the same workflow repeatedly. Keep those saved rows intact for recovery,
    // but do not expose or apply them in the native UI.
    const QString nativeId = nativeTemplateId(currentWorkflow());
    const QList<CommunicationTemplate> stored = store.templates(currentWorkflow());
    const auto native = std::find_if(stored.cbegin(), stored.cend(), [&nativeId](const CommunicationTemplate &value) {
        return value.id == nativeId;
    });
    m_templates.clear();
    if (native != stored.cend())
        m_templates.append(*native);
    const QString wanted = preferredId.isEmpty() ? m_selectedId : preferredId;
    const auto found = std::find_if(m_templates.cbegin(), m_templates.cend(), [&wanted](const CommunicationTemplate &value) {
        return value.id == wanted;
    });
    if (found != m_templates.cend()) {
        m_draft = *found;
        m_draftPlainBody = found->plainBody;
        m_selectedId = found->id;
    } else if (wanted.isEmpty() && !m_templates.isEmpty()) {
        m_draft = m_templates.constFirst();
        m_draftPlainBody = m_draft.plainBody;
        m_selectedId = m_draft.id;
    } else {
        m_draft = {};
        m_draft.workflow = currentWorkflow();
        m_draftPlainBody.clear();
        m_selectedId.clear();
    }
    m_diagnostic.clear();
    emit changed();
}

bool TemplateEditorModel::selectTemplate(const QString &id)
{
    const auto found = std::find_if(m_templates.cbegin(), m_templates.cend(), [&id](const CommunicationTemplate &value) {
        return value.id == id;
    });
    if (found == m_templates.cend()) return false;
    m_draft = *found;
    m_draftPlainBody = found->plainBody;
    m_selectedId = found->id;
    m_diagnostic.clear();
    emit changed();
    return true;
}

void TemplateEditorModel::beginNew()
{
    // Retained as a harmless compatibility entry point for older QML, but a
    // native workflow cannot acquire a second template.
    reload(nativeTemplateId(currentWorkflow()));
}

bool TemplateEditorModel::duplicateSelected()
{
    // There is exactly one editable wrapper for each fixed workflow.
    m_diagnostic = tr("Native workflows cannot be duplicated.");
    emit changed();
    return false;
}

bool TemplateEditorModel::resetSelected()
{
    if (m_selectedId.isEmpty()) return false;
    CommunicationTemplateStore store(*m_settings, m_databaseKey);
    const ValidationResult result = store.reset(m_selectedId);
    setDiagnostic(result);
    if (!result.ok()) return false;
    reload(m_selectedId);
    return true;
}

ValidationResult TemplateEditorModel::validate() const
{
    ValidationResult result = validateTemplateSubject(m_draft.subject);
    const TemplateContext context = exampleContext(currentWorkflow());
    // Shield the protected native-content marker while validating ordinary
    // literal fields. TemplateApplication validates its count separately.
    QString richBody = m_draft.body;
    QString plainBody = m_draftPlainBody;
    const QRegularExpression protectedToken(QStringLiteral(R"(\{\{\s*content\.body\s*\}\})"));
    richBody.replace(protectedToken, QStringLiteral("\\{{ content.body }}"));
    plainBody.replace(protectedToken, QStringLiteral("\\{{ content.body }}"));
    result.issues += renderTemplateFields(m_draft.subject, context).validation.issues;
    result.issues += renderTemplateFields(richBody, context, TemplateRenderMode::Html).validation.issues;
    result.issues += renderTemplateFields(plainBody, context).validation.issues;
    if (m_draft.name.trimmed().isEmpty()) result.addError("template-invalid", "name");
    if (TemplateApplication::protectedContentBlockCount(m_draft.body) != 1)
        result.addError("template-content-block-required", "body");
    if (!m_draftPlainBody.isEmpty()
        && TemplateApplication::protectedContentBlockCount(m_draftPlainBody) != 1)
        result.addError("template-content-block-required", "plainBody");
    return result;
}

void TemplateEditorModel::setDiagnostic(const ValidationResult &result)
{
    m_diagnostic.clear();
    if (!result.issues.isEmpty()) {
        const ValidationIssue &issue = result.issues.constFirst();
        m_diagnostic = issue.displayText.isEmpty() ? issue.code : issue.displayText;
    }
    emit changed();
}

bool TemplateEditorModel::validateDraft()
{
    const ValidationResult result = validate();
    setDiagnostic(result);
    return result.ok();
}

bool TemplateEditorModel::save()
{
    // A template may be saved before its context/protected block is complete.
    // Keep those preview diagnostics visible, but defer their enforcement to
    // TemplateApplication immediately before review/handoff.
    const ValidationResult draftValidation = validate();
    // Always save as this workflow's native override. This also repairs a
    // draft left by an older UI which assigned it a generated id.
    m_draft.id = nativeTemplateId(currentWorkflow());
    m_draft.workflow = currentWorkflow();
    m_draft.plainBody = m_draftPlainBody;
    CommunicationTemplateStore store(*m_settings, m_databaseKey);
    const ValidationResult result = store.save(m_draft);
    setDiagnostic(result);
    if (!result.ok()) return false;
    reload(m_draft.id);
    if (!draftValidation.ok())
        setDiagnostic(draftValidation);
    return true;
}

bool TemplateEditorModel::appendField(const QString &path, const QString &target)
{
    if (!templateFieldIsApplicable(path, currentWorkflow())) return false;
    const QString token = QStringLiteral("{{ %1 }}").arg(path);
    if (target == QStringLiteral("subject")) m_draft.subject += token;
    else if (target == QStringLiteral("richBody")) m_draft.body += token;
    else if (target == QStringLiteral("plainBody")) m_draftPlainBody += token;
    else return false;
    emit changed();
    return true;
}

std::optional<CommunicationTemplate> TemplateEditorModel::templateById(const QString &id) const
{
    const auto found = std::find_if(m_templates.cbegin(), m_templates.cend(), [&id](const CommunicationTemplate &value) {
        return value.id == id;
    });
    return found == m_templates.cend() ? std::nullopt : std::optional<CommunicationTemplate>(*found);
}

} // namespace PN::Comm
