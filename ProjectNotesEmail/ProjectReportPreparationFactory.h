// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "EmailContentBuilder.h"
#include "RecipientAudienceResolver.h"
#include "StatusReportBuilder.h"
#include "TrackerItemsReportBuilder.h"

#include <optional>

namespace PN::Comm {

struct CommunicationTemplate;

// Pure snapshot-to-review preparation for the two project-level reports.  The
// desktop composition root retains the snapshot and owns all later recipient
// overrides/backends; this class never reads live models or starts a handoff.
struct ProjectReportReview {
    EmailPreparation preparation;
    AudienceResolution audience;
    ValidationResult validation;
};

class ProjectReportPreparationFactory final {
public:
    static std::optional<ProjectReportReview> createStatus(const CommunicationSnapshot &snapshot,
                                                            SourceContext source,
                                                            const ReportOptions &options,
                                                            BackendId backend = BackendId::Mailto,
                                                            const CommunicationTemplate *contentTemplate = nullptr);
    static std::optional<ProjectReportReview> createTracker(const CommunicationSnapshot &snapshot,
                                                             SourceContext source,
                                                             const ReportOptions &options,
                                                             BackendId backend = BackendId::Mailto,
                                                             const CommunicationTemplate *contentTemplate = nullptr);
};

} // namespace PN::Comm
