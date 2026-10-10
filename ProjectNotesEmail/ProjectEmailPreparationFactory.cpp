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
    const bool plainContact = source.workflow == Workflow::ContactEmail;
    if (source.workflow != Workflow::ProjectEmail && source.workflow != Workflow::ShareLocation && !plainContact)
        review.validation.addError("email-workflow-invalid", "source.workflow");
    review.rule.includeUnknownCompany = true;
    review.rule.excludeProjectManager = source.workflow == Workflow::ShareLocation;
    TemplateContext context = communicationTemplateContext(snapshot, source.workflow);
    context.values.insert("report.internal", templateBoolean(false));
    QString nativeHtml;
    if (source.workflow == Workflow::ProjectEmail || plainContact) {
        context.values.insert("contact.name", QString());
        context.values.insert("contact.email", QString());
        if (!source.contactId.isEmpty()) {
            review.rule.source = ContactSource::ChosenContacts;
            review.rule.chosenContactIds = {source.contactId};
            // Keep the project's other recipients available to the shared
            // audience editor, but initially address only the clicked contact.
            if (!plainContact)
                for (const SnapshotContact &contact : snapshot.contacts)
                    if (contact.projectTeamMember && !review.rule.chosenContactIds.contains(contact.id))
                        review.rule.chosenContactIds.append(contact.id);
            bool found = false;
            for (const SnapshotContact &contact : snapshot.contacts) {
                if (contact.id != source.contactId) continue;
                found = true;
                context.values.insert("contact.name", contact.name);
                context.values.insert("contact.email", contact.email);
            }
            if (!found) review.validation.addError(plainContact ? "contact-not-found" : "contact-not-in-project", "source.contactId");
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
    const auto rendered = plainContact
        ? TemplateApplicationResult{} : TemplateApplication::apply(contentTemplate, context, nativeHtml);
    review.validation.issues += rendered.validation.issues;
    if (!review.validation.ok()) return review;
    review.audience = resolveAudience(snapshot, review.rule);
    review.validation.issues += review.audience.validation.issues;
    if (!source.contactId.isEmpty()) {
        bool clickedContactAvailable = false;
        for (const SnapshotContact &contact : review.audience.contacts) {
            const bool selected = contact.id == source.contactId;
            review.audience.initialOverrides.append({contact.id, selected, RecipientRole::To});
            clickedContactAvailable |= selected;
        }
        if (!clickedContactAvailable)
            review.validation.addError("recipient-address-invalid", "contact.email",
                                      QStringLiteral("The selected contact has no valid email address. Correct it on the Contacts page."));
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
