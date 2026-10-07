// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#include "ProjectEmailPreparationFactory.h"
#include "CommunicationTemplateContext.h"
#include "ProjectNotesIntegrations/TemplateApplication.h"
#include <QFileInfo>

namespace PN::Comm {
ProjectEmailReview ProjectEmailPreparationFactory::create(
    const CommunicationSnapshot &snapshot, SourceContext source, BackendId backend,
    const CommunicationTemplate &contentTemplate, bool attachLocation)
{
    ProjectEmailReview review;
    review.validation = validate(source);
    if (snapshot.projectId != source.projectId || snapshot.databaseGeneration != source.databaseGeneration)
        review.validation.addError("snapshot-source-mismatch", "source");
    const bool plainPerson = source.workflow == Workflow::PersonEmail;
    if (source.workflow != Workflow::ProjectEmail && source.workflow != Workflow::ShareLocation && !plainPerson)
        review.validation.addError("email-workflow-invalid", "source.workflow");
    review.rule.includeUnknownCompany = true;
    review.rule.excludeProjectManager = source.workflow == Workflow::ShareLocation;
    TemplateContext context = communicationTemplateContext(snapshot, source.workflow);
    context.values.insert("report.internal", templateBoolean(false));
    QString nativeHtml;
    if (source.workflow == Workflow::ProjectEmail || plainPerson) {
        context.values.insert("person.name", QString());
        context.values.insert("person.email", QString());
        if (!source.personId.isEmpty()) {
            review.rule.source = PeopleSource::ChosenPeople;
            review.rule.chosenPersonIds = {source.personId};
            // Keep the project's other recipients available to the shared
            // audience editor, but initially address only the clicked person.
            if (!plainPerson)
                for (const SnapshotPerson &person : snapshot.people)
                    if (person.projectTeamMember && !review.rule.chosenPersonIds.contains(person.id))
                        review.rule.chosenPersonIds.append(person.id);
            bool found = false;
            for (const SnapshotPerson &person : snapshot.people) {
                if (person.id != source.personId) continue;
                found = true;
                context.values.insert("person.name", person.name);
                context.values.insert("person.email", person.email);
            }
            if (!found) review.validation.addError(plainPerson ? "person-not-found" : "person-not-in-project", "source.personId");
        }
    } else if (!snapshot.location || snapshot.location->id != source.locationId) {
        review.validation.addError("location-not-in-project", "source.locationId");
    } else {
        const auto &location = *snapshot.location;
        const QString path = location.path.trimmed();
        QUrl url(path);
        if (url.scheme().isEmpty() || QFileInfo(path).isAbsolute())
            url = QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath());
        const bool web = url.scheme() == "http" || url.scheme() == "https";
        if (path.isEmpty() || !url.isValid() || (!web && !url.isLocalFile()))
            review.validation.addError("location-url-invalid", "location",
                                      QStringLiteral("Choose a local file or an HTTP/HTTPS URL."));
        if (web && url.host().isEmpty())
            review.validation.addError("location-url-invalid", "location");
        if (attachLocation && url.isLocalFile() && !QFileInfo(url.toLocalFile()).isFile())
            review.validation.addError("location-file-required", "location",
                                      QStringLiteral("The location must be an existing file to attach it."));
        const QString name = QFileInfo(url.path()).fileName().isEmpty()
            ? QStringLiteral("download") : QFileInfo(url.path()).fileName();
        const QString description = location.description.trimmed().isEmpty() ? name : location.description;
        context.values.insert("location.description", description);
        context.values.insert("location.url", url.toString(QUrl::FullyEncoded));
        context.values.insert("location.fileName", name);
        nativeHtml = attachLocation
            ? QStringLiteral("<p>Attachment: %1</p>").arg(description.toHtmlEscaped())
            : QStringLiteral("<p><a href=\"%1\">%2</a></p>")
                .arg(url.toString(QUrl::FullyEncoded).toHtmlEscaped(), description.toHtmlEscaped());
        if (attachLocation) review.attachmentUrl = url;
    }
    if (!review.validation.ok()) return review;
    const auto rendered = plainPerson
        ? TemplateApplicationResult{} : TemplateApplication::apply(contentTemplate, context, nativeHtml);
    review.validation.issues += rendered.validation.issues;
    if (!review.validation.ok()) return review;
    review.audience = resolveAudience(snapshot, review.rule);
    review.validation.issues += review.audience.validation.issues;
    if (!source.personId.isEmpty()) {
        bool clickedPersonAvailable = false;
        for (const SnapshotPerson &person : review.audience.people) {
            const bool selected = person.id == source.personId;
            review.audience.initialOverrides.append({person.id, selected, RecipientRole::To});
            clickedPersonAvailable |= selected;
        }
        if (!clickedPersonAvailable)
            review.validation.addError("recipient-address-invalid", "person.email",
                                      QStringLiteral("The selected person has no valid email address. Correct it on the People page."));
    }
    if (!review.validation.ok()) return review;
    auto &prepared = review.preparation;
    prepared.operationId = QUuid::createUuid();
    prepared.source = std::move(source);
    prepared.backend = backend;
    prepared.document.workflow = prepared.source.workflow;
    prepared.document.defaultSubject = rendered.subject;
    prepared.document.emailFragment = rendered.html;
    prepared.document.htmlDocument = QStringLiteral("<!DOCTYPE html><html><body>%1</body></html>").arg(rendered.html);
    prepared.recipients = applyRecipientOverrides(review.audience, review.audience.initialOverrides).recipients;
    return review;
}
}
