// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#include "MeetingNotesReportPreparationFactory.h"

#include "ProjectNotesIntegrations/TemplateApplication.h"

namespace PN::Comm {
std::optional<EmailPreparation> MeetingNotesReportPreparationFactory::create(
    const CommunicationSnapshot &snapshot, SourceContext source, QDate reportingDate, bool internalReport,
    BackendId backend, EmailMode mode, bool retainHtml, ValidationResult *failureValidation,
    const CommunicationTemplate *contentTemplate)
{
    source.workflow = Workflow::MeetingNotesReport;
    ValidationResult validation = validate(source);
    if (snapshot.projectId != source.projectId) validation.addError("snapshot-project-mismatch", "source.projectId");
    if (snapshot.databaseGeneration != source.databaseGeneration) validation.addError("snapshot-database-generation-mismatch", "source.databaseGeneration");
    auto document = MeetingNotesReportBuilder::build({snapshot, reportingDate, internalReport}, &validation);
    if (!document || !validation.ok()) { if (failureValidation) *failureValidation = validation; return std::nullopt; }
    if (contentTemplate) {
        TemplateContext context;
        context.workflow = Workflow::MeetingNotesReport;
        context.values.insert(QStringLiteral("project.number"), snapshot.projectNumber);
        context.values.insert(QStringLiteral("project.name"), snapshot.projectName);
        context.values.insert(QStringLiteral("meeting.date"), reportingDate.toString(QStringLiteral("MM/dd/yyyy")));
        const TemplateApplicationResult applied = TemplateApplication::apply(
            *contentTemplate, context, document->emailFragment, document->plainText);
        validation.issues += applied.validation.issues;
        if (!validation.ok()) { if (failureValidation) *failureValidation = validation; return std::nullopt; }
        document->htmlDocument.replace(document->emailFragment, applied.html);
        document->emailFragment = applied.html;
        document->plainText = applied.plainText;
        document->defaultSubject = applied.subject;
    }
    EmailPreparation preparation; preparation.operationId = QUuid::createUuid(); preparation.source = std::move(source);
    preparation.backend = backend; preparation.mode = mode; preparation.retainHtml = retainHtml;
    preparation.projectFolderPath = snapshot.projectFolderPath;
    preparation.document = std::move(*document);
    return preparation;
}
} // namespace PN::Comm
