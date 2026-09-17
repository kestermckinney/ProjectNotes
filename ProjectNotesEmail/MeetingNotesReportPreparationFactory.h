// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "EmailContentBuilder.h"
#include "MeetingNotesReportBuilder.h"
#include "ProjectNotesIntegrations/CommunicationTemplateStore.h"

namespace PN::Comm {
class MeetingNotesReportPreparationFactory final {
public:
    static std::optional<EmailPreparation> create(const CommunicationSnapshot &snapshot, SourceContext source,
                                                   QDate reportingDate, bool internalReport,
                                                   BackendId backend = BackendId::Mailto,
                                                   EmailMode mode = EmailMode::InlineHtml,
                                                   bool retainHtml = false,
                                                   ValidationResult *failureValidation = nullptr,
                                                   const CommunicationTemplate *contentTemplate = nullptr);
};
} // namespace PN::Comm
