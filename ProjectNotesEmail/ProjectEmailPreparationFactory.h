// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "EmailContentBuilder.h"
#include "RecipientAudienceResolver.h"
#include "ProjectNotesIntegrations/CommunicationTemplateStore.h"
#include <QUrl>

namespace PN::Comm {
struct ProjectEmailReview {
    EmailPreparation preparation;
    AudienceResolution audience;
    AudienceRule rule;
    ValidationResult validation;
    QUrl attachmentUrl;
};

// shareContactCard (ContactEmail only) prepares a message about the contact
// rather than to them: no recipients, address-later, and a "Contact: <name>"
// subject. The caller attaches the vCard.
class ProjectEmailPreparationFactory final {
public:
    static ProjectEmailReview create(const CommunicationSnapshot &snapshot, SourceContext source,
                                    BackendId backend, const CommunicationTemplate &contentTemplate,
                                    bool attachLocation = false, bool shareContactCard = false);
};
}
