// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#include "ProjectReportPreparationFactory.h"

#include "CommunicationTemplateContext.h"
#include "ProjectNotesIntegrations/TemplateApplication.h"

namespace PN::Comm {
namespace {

bool validSnapshot(const CommunicationSnapshot &snapshot, const SourceContext &source, ValidationResult &validation)
{
    validation = validate(source);
    if (snapshot.projectId != source.projectId)
        validation.addError(QStringLiteral("snapshot-project-mismatch"), QStringLiteral("source.projectId"));
    if (snapshot.databaseGeneration != source.databaseGeneration)
        validation.addError(QStringLiteral("snapshot-database-generation-mismatch"), QStringLiteral("source.databaseGeneration"));
    return validation.ok();
}

ProjectReportReview finish(ReportDocument document, SourceContext source, BackendId backend,
                           EmailMode mode, bool retainHtml, bool displayPdf, QString projectFolderPath,
                           AudienceResolution audience, ValidationResult validation)
{
    ProjectReportReview review;
    review.validation = std::move(validation);
    review.audience = std::move(audience);
    review.validation.issues += review.audience.validation.issues;
    review.preparation.operationId = QUuid::createUuid();
    review.preparation.source = std::move(source);
    review.preparation.backend = backend;
    review.preparation.mode = mode;
    review.preparation.retainHtml = retainHtml;
    review.preparation.displayPdf = displayPdf;
    review.preparation.projectFolderPath = std::move(projectFolderPath);
    review.preparation.document = std::move(document);
    const RecipientResolution recipients = applyRecipientOverrides(review.audience,
                                                                     review.audience.initialOverrides);
    review.validation.issues += recipients.validation.issues;
    review.preparation.recipients = recipients.recipients;
    review.preparation.addressLater = recipients.addressLaterExplicitlyChosen;
    return review;
}

QStringList activity(const CommunicationSnapshot &snapshot, const QString &category)
{
    QStringList values;
    for (const SnapshotStatusItem &item : snapshot.statusItems)
        if (item.category.compare(category, Qt::CaseInsensitive) == 0 && !item.description.trimmed().isEmpty())
            values.append(item.description);
    return values;
}

bool applyContentTemplate(ReportDocument *document, const CommunicationSnapshot &snapshot,
                          Workflow workflow, const ReportOptions &options,
                          const CommunicationTemplate *contentTemplate,
                          ValidationResult *validation)
{
    if (!contentTemplate)
        return true;
    if (contentTemplate->workflow != workflow) {
        validation->addError(QStringLiteral("template-workflow-mismatch"), QStringLiteral("template"));
        return false;
    }
    TemplateContext context = communicationTemplateContext(snapshot, workflow);
    context.values.insert(QStringLiteral("report.date"), options.reportingDate.toString(QStringLiteral("MM/dd/yyyy")));
    context.values.insert(QStringLiteral("report.internal"), templateBoolean(options.internalReport));
    const TemplateApplicationResult applied = TemplateApplication::apply(
        *contentTemplate, context, document->emailFragment, document->plainText);
    validation->issues += applied.validation.issues;
    if (!validation->ok())
        return false;
    document->htmlDocument.replace(document->emailFragment, applied.html);
    document->emailFragment = applied.html;
    document->plainText = applied.plainText;
    document->defaultSubject = applied.subject;
    return true;
}

} // namespace

std::optional<ProjectReportReview> ProjectReportPreparationFactory::createStatus(
    const CommunicationSnapshot &snapshot, SourceContext source, const ReportOptions &options, BackendId backend,
    const CommunicationTemplate *contentTemplate)
{
    source.workflow = Workflow::StatusReport;
    ValidationResult validation;
    if (!validSnapshot(snapshot, source, validation)) return std::nullopt;
    validation.issues += validate(options).issues;
    if (options.workflow != Workflow::StatusReport)
        validation.addError(QStringLiteral("report-workflow-mismatch"), QStringLiteral("workflow"));
    if (!validation.ok()) return std::nullopt;

    StatusReportInput input;
    input.projectNumber = snapshot.projectNumber; input.projectName = snapshot.projectName;
    input.reportingPeriod = snapshot.statusReportPeriod; input.reportingDate = options.reportingDate;
    input.internalReport = options.internalReport;
    input.budget = snapshot.budget; input.actual = snapshot.actual; input.bcwp = snapshot.bcwp;
    input.bcws = snapshot.bcws; input.bac = snapshot.bac;
    input.activitiesInProgress = activity(snapshot, QStringLiteral("In Progress"));
    input.activitiesNextPeriod = activity(snapshot, QStringLiteral("Next Period"));
    input.activitiesCompleted = activity(snapshot, QStringLiteral("Completed"));
    for (const SnapshotPerson &person : snapshot.people) {
        if (person.id == snapshot.projectManagerId) input.managerName = person.name;
        if (person.receivesStatus && !person.name.trimmed().isEmpty()) input.stakeholders.append(person.name);
    }
    for (const SnapshotTrackerItem &item : snapshot.trackerItems) {
        if (item.itemType.compare(QStringLiteral("Tracker"), Qt::CaseInsensitive) == 0
            && (item.status == QStringLiteral("New") || item.status == QStringLiteral("Assigned")))
            input.issues.append({item.name, item.assignedTo, item.priority, item.dueDate, item.status,
                                 item.internal});
    }
    auto document = StatusReportBuilder::build(input, &validation);
    if (!document || !validation.ok()) return std::nullopt;
    if (!applyContentTemplate(&*document, snapshot, Workflow::StatusReport, options, contentTemplate, &validation))
        return std::nullopt;
    // The report dialog may offer every member of this project, but starts
    // with only members who opted in to status reports selected.
    AudienceRule audienceRule; audienceRule.source = PeopleSource::ProjectTeam;
    audienceRule.companyFilter = CompanyFilter::All; audienceRule.excludeProjectManager = true;
    AudienceResolution audience = resolveAudience(snapshot, audienceRule);
    for (const SnapshotPerson &person : audience.people)
        if (!person.receivesStatus)
            audience.initialOverrides.append({person.id, false, RecipientRole::To});
    return finish(*document, std::move(source), backend, options.emailMode, options.retainHtml, options.displayPdf, snapshot.projectFolderPath,
                  std::move(audience), std::move(validation));
}

std::optional<ProjectReportReview> ProjectReportPreparationFactory::createTracker(
    const CommunicationSnapshot &snapshot, SourceContext source, const ReportOptions &options, BackendId backend,
    const CommunicationTemplate *contentTemplate)
{
    source.workflow = Workflow::TrackerItemsReport;
    ValidationResult validation;
    if (!validSnapshot(snapshot, source, validation)) return std::nullopt;
    validation.issues += validate(options).issues;
    if (options.workflow != Workflow::TrackerItemsReport)
        validation.addError(QStringLiteral("report-workflow-mismatch"), QStringLiteral("workflow"));
    if (!validation.ok()) return std::nullopt;

    TrackerItemsReportInput input; input.projectNumber = snapshot.projectNumber;
    input.projectName = snapshot.projectName; input.options = options;
    for (const SnapshotTrackerItem &item : snapshot.trackerItems)
        input.items.append({item.number,item.name,item.identifiedBy,item.dateIdentified,item.description,item.assignedTo,
                            item.priority,item.status,item.dueDate,item.lastUpdate,item.dateResolved,item.comments,
                            item.itemType,item.internal});
    auto document = TrackerItemsReportBuilder::build(input, &validation);
    if (!document || !validation.ok()) return std::nullopt;
    if (!applyContentTemplate(&*document, snapshot, Workflow::TrackerItemsReport, options, contentTemplate, &validation))
        return std::nullopt;
    AudienceRule audienceRule; audienceRule.source = PeopleSource::ProjectTeam;
    audienceRule.companyFilter = CompanyFilter::All; audienceRule.excludeProjectManager = true;
    AudienceResolution audience = resolveAudience(snapshot, audienceRule);
    for (const SnapshotPerson &person : audience.people)
        if (!person.receivesStatus)
            audience.initialOverrides.append({person.id, false, RecipientRole::To});
    return finish(*document, std::move(source), backend, options.emailMode, options.retainHtml, options.displayPdf, snapshot.projectFolderPath,
                  std::move(audience), std::move(validation));
}

} // namespace PN::Comm
