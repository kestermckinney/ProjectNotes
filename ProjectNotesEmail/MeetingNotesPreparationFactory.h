// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailContentBuilder.h"
#include "MeetingNotesEmailBuilder.h"
#include "RecipientAudienceResolver.h"
#include "ProjectNotesIntegrations/CommunicationTemplateStore.h"

#include <optional>

namespace PN::Comm {

// Pure orchestration for the one-note review flow.  It deliberately stops at
// preparation: the desktop action owns snapshot lifetime and the review UI owns
// recipient overrides before any backend is called.
struct MeetingNotesReview {
    EmailPreparation preparation;
    AudienceResolution audience;
    ValidationResult validation;
};

class MeetingNotesPreparationFactory final {
public:
    static std::optional<MeetingNotesReview> create(const CommunicationSnapshot &snapshot,
                                                     SourceContext source,
                                                     BackendId backend = BackendId::Mailto,
                                                     EmailMode mode = EmailMode::InlineHtml,
                                                     const CommunicationTemplate *contentTemplate = nullptr,
                                                     ValidationResult *failureValidation = nullptr);
};

} // namespace PN::Comm
