#include "ReportService.h"
namespace PN::Comm {
std::optional<ReportDocument> ReportService::build(const ReportBuildInput &input,ValidationResult *validation){return std::visit([validation](const auto &value)->std::optional<ReportDocument>{using T=std::decay_t<decltype(value)>;if constexpr(std::is_same_v<T,MeetingNotesBuildInput>)return MeetingNotesEmailBuilder::build(value,validation);else if constexpr(std::is_same_v<T,MeetingNotesReportInput>)return MeetingNotesReportBuilder::build(value,validation);else if constexpr(std::is_same_v<T,StatusReportInput>)return StatusReportBuilder::build(value,validation);else return TrackerItemsReportBuilder::build(value,validation);},input);}
}
