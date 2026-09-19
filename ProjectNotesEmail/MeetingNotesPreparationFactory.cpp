// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MeetingNotesPreparationFactory.h"

#include "CommunicationTemplateContext.h"

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
        TemplateContext context = communicationTemplateContext(snapshot, Workflow::SendMeetingNotes);
        // The manager may not be on this project's team, so prefer the name
        // resolved at snapshot time and fall back to the team roster.
        if (context.values.value(QStringLiteral("preferences.managerName")).isEmpty()) {
            const auto manager = std::find_if(snapshot.people.cbegin(), snapshot.people.cend(), [&snapshot](const SnapshotPerson &person) {
                return person.id == snapshot.projectManagerId;
            });
            if (manager != snapshot.people.cend())
                context.values.insert(QStringLiteral("preferences.managerName"), manager->name);
        }
        context.values.insert(QStringLiteral("meeting.title"), note == snapshot.notes.cend() ? QString() : note->title);
        context.values.insert(QStringLiteral("report.internal"),
                              templateBoolean(note != snapshot.notes.cend() && note->internal));
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

    // List this note's attendees together with the remaining project team.
    // Only attendees start selected; absent team members default to CC.
    AudienceRule defaultAudience;
    defaultAudience.source = PeopleSource::ChosenPeople;
    defaultAudience.companyFilter = CompanyFilter::All;
    defaultAudience.excludeProjectManager = true;
    const auto selectedNote = std::find_if(snapshot.notes.cbegin(), snapshot.notes.cend(), [&source](const SnapshotNote &note) {
        return note.id == source.noteIds.constFirst();
    });
    if (selectedNote != snapshot.notes.cend())
        defaultAudience.chosenPersonIds = selectedNote->attendeeIds;
    for (const SnapshotPerson &person : snapshot.people)
        if (person.projectTeamMember && !defaultAudience.chosenPersonIds.contains(person.id))
            defaultAudience.chosenPersonIds.append(person.id);
    review.audience = resolveAudience(snapshot, defaultAudience);
    for (SnapshotPerson &person : review.audience.people) {
        person.meetingAttendee = selectedNote != snapshot.notes.cend()
            && selectedNote->attendeeIds.contains(person.id);
        review.audience.initialOverrides.append({person.id, person.meetingAttendee,
                                                person.meetingAttendee ? RecipientRole::To : RecipientRole::Cc});
    }
    review.validation.issues += review.audience.validation.issues;

    const RecipientResolution recipients = applyRecipientOverrides(review.audience, review.audience.initialOverrides);
    review.validation.issues += recipients.validation.issues;
    if (!review.validation.ok())
        return std::nullopt;

    review.preparation.operationId = QUuid::createUuid();
    review.preparation.source = std::move(source);
    review.preparation.backend = backend;
    review.preparation.mode = mode;
    review.preparation.document = *document;
    review.preparation.recipients = recipients.recipients;
    return review;
}

} // namespace PN::Comm
