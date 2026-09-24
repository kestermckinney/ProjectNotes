// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "MeetingNotesEmailBuilder.h"

namespace PN::Comm {

struct MeetingNotesReportInput {
    CommunicationSnapshot snapshot;
    QDate reportingDate;
    bool internalReport = false;
};

class MeetingNotesReportBuilder final {
public:
    static std::optional<ReportDocument> build(const MeetingNotesReportInput &input,
                                               ValidationResult *validation = nullptr);
};

} // namespace PN::Comm
