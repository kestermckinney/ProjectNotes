// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "EmailTypes.h"

namespace PN::Comm {

ReportOptions defaultReportOptions(Workflow workflow, const QDate &today)
{
    ReportOptions options;
    options.workflow = workflow;
    options.reportingDate = today;
    if (workflow == Workflow::TrackerItemsReport)
        options.tracker.emplace();
    return options;
}

ValidationResult validate(const SourceContext &context)
{
    ValidationResult result;
    if (context.databaseKey.trimmed().isEmpty())
        result.addError(QStringLiteral("database-key-required"), QStringLiteral("source.databaseKey"));
    if (context.projectId.trimmed().isEmpty() && context.workflow != Workflow::ContactEmail)
        result.addError(QStringLiteral("project-id-required"), QStringLiteral("source.projectId"));
    if (context.workflow == Workflow::SendMeetingNotes && context.noteIds.size() != 1)
        result.addError(QStringLiteral("single-note-required"), QStringLiteral("source.noteIds"));
    if (context.workflow == Workflow::ShareLocation && context.locationId.trimmed().isEmpty())
        result.addError(QStringLiteral("location-id-required"), QStringLiteral("source.locationId"));
    if (!context.locationId.isEmpty() && context.workflow != Workflow::ShareLocation)
        result.addError(QStringLiteral("location-context-not-applicable"), QStringLiteral("source.locationId"));
    if (context.workflow == Workflow::ContactEmail) {
        if (context.contactId.trimmed().isEmpty())
            result.addError(QStringLiteral("contact-id-required"), QStringLiteral("source.contactId"));
        if (!context.projectId.isEmpty() || !context.noteIds.isEmpty())
            result.addError(QStringLiteral("project-context-not-applicable"), QStringLiteral("source.projectId"));
    }
    if (!context.contactId.isEmpty() && context.workflow != Workflow::ProjectEmail && context.workflow != Workflow::ContactEmail)
        result.addError(QStringLiteral("contact-context-not-applicable"), QStringLiteral("source.contactId"));
    return result;
}

ValidationResult validate(const ReportOptions &options)
{
    ValidationResult result;
    if (!options.reportingDate.isValid())
        result.addError(QStringLiteral("reporting-date-invalid"), QStringLiteral("reportingDate"));
    if (options.workflow == Workflow::SendMeetingNotes || options.workflow == Workflow::ProjectEmail
        || options.workflow == Workflow::ShareLocation || options.workflow == Workflow::ContactEmail)
        result.addError(QStringLiteral("report-options-not-supported"), QStringLiteral("workflow"));
    if (options.workflow == Workflow::TrackerItemsReport) {
        if (!options.tracker.has_value())
            result.addError(QStringLiteral("tracker-filters-required"), QStringLiteral("tracker"));
        else {
            if (options.tracker->itemTypes.isEmpty())
                result.addError(QStringLiteral("tracker-item-types-required"), QStringLiteral("tracker.itemTypes"));
            if (options.tracker->statuses.isEmpty())
                result.addError(QStringLiteral("tracker-statuses-required"), QStringLiteral("tracker.statuses"));
        }
    }
    if (options.workflow != Workflow::TrackerItemsReport && options.tracker.has_value())
        result.addError(QStringLiteral("tracker-filters-not-applicable"), QStringLiteral("tracker"));
    return result;
}

} // namespace PN::Comm
