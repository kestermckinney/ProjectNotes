#pragma once
#include "MeetingNotesEmailBuilder.h"
#include "MeetingNotesReportBuilder.h"
#include "StatusReportBuilder.h"
#include "TrackerItemsReportBuilder.h"
#include <variant>
namespace PN::Comm {
using ReportBuildInput = std::variant<MeetingNotesBuildInput, MeetingNotesReportInput, StatusReportInput, TrackerItemsReportInput>;
class ReportService final { public: static std::optional<ReportDocument> build(const ReportBuildInput &, ValidationResult * = nullptr); };
}
