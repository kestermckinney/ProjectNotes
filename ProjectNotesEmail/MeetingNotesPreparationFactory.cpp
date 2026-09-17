// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MeetingNotesPreparationFactory.h"

#include "ProjectNotesIntegrations/TemplateApplication.h"

#include <algorithm>

namespace PN::Comm {

std::optional<MeetingNotesReview> MeetingNotesPreparationFactory::create(
        const CommunicationSnapshot &snapshot, SourceContext source,
        BackendId backend, EmailMode mode, const CommunicationTemplate *contentTemplate,
        ValidationResult *failureValidation)
{
    MeetingNotesReview review;
    source.workflow = Workflow::SendMeetingNotes;
    review.validation = validate(source);
    if (snapshot.projectId != source.projectId)
        review.validation.addError(QStringLiteral("snapshot-project-mismatch"),
                                   QStringLiteral("source.projectId"));
    if (snapshot.databaseGeneration != source.databaseGeneration)
        review.validation.addError(QStringLiteral("snapshot-database-generation-mismatch"),
                                   QStringLiteral("source.databaseGeneration"));
    if (!review.validation.ok()) {
        if (failureValidation) *failureValidation = review.validation;
        return std::nullopt;
    }

    ValidationResult reportValidation;
    auto document = MeetingNotesEmailBuilder::build({snapshot, source.noteIds.constFirst(), {}},
                                                     &reportValidation);
    review.validation.issues += reportValidation.issues;
    if (!document || !review.validation.ok()) {
        if (failureValidation) *failureValidation = review.validation;
        return std::nullopt;
    }

    if (contentTemplate) {
        const auto note = std::find_if(snapshot.notes.cbegin(), snapshot.notes.cend(), [&source](const SnapshotNote &value) {
            return value.id == source.noteIds.constFirst();
        });
        TemplateContext context;
        context.workflow = Workflow::SendMeetingNotes;
        context.values.insert(QStringLiteral("project.number"), snapshot.projectNumber);
        context.values.insert(QStringLiteral("project.name"), snapshot.projectName);
        const auto manager = std::find_if(snapshot.people.cbegin(), snapshot.people.cend(), [&snapshot](const SnapshotPerson &person) {
            return person.id == snapshot.projectManagerId;
        });
        if (manager != snapshot.people.cend() && !manager->name.trimmed().isEmpty())
            context.values.insert(QStringLiteral("preferences.managerName"), manager->name);
        context.values.insert(QStringLiteral("meeting.title"), note == snapshot.notes.cend() ? QString() : note->title);
        context.values.insert(QStringLiteral("meeting.date"), note == snapshot.notes.cend() || !note->date.isValid()
                              ? QString() : note->date.date().toString(QStringLiteral("MM/dd/yyyy")));
        const TemplateApplicationResult applied = TemplateApplication::apply(
            *contentTemplate, context, document->emailFragment, document->plainText);
        review.validation.issues += applied.validation.issues;
        if (!review.validation.ok()) {
            if (failureValidation) *failureValidation = review.validation;
            return std::nullopt;
        }
        document->htmlDocument.replace(document->emailFragment, applied.html);
        document->emailFragment = applied.html;
        document->plainText = applied.plainText;
        document->defaultSubject = applied.subject;
    }

    // This is the legacy Send Meeting Notes default: the entire project team.
    // RecipientSelectionModel retains this resolution for explicit review and
    // any user-selected To/Cc/Bcc overrides.
    AudienceRule defaultAudience;
    defaultAudience.source = PeopleSource::ProjectTeam;
    defaultAudience.companyFilter = CompanyFilter::All;
    defaultAudience.excludeProjectManager = false;
    review.audience = resolveAudience(snapshot, defaultAudience);
    review.validation.issues += review.audience.validation.issues;

    const RecipientResolution recipients = applyRecipientOverrides(review.audience, {});
    review.validation.issues += recipients.validation.issues;
    if (!review.validation.ok())
        return std::nullopt;

    review.preparation.source = std::move(source);
    review.preparation.backend = backend;
    review.preparation.mode = mode;
    review.preparation.document = *document;
    review.preparation.recipients = recipients.recipients;
    return review;
}

} // namespace PN::Comm
