#pragma once
#include "ReportTypes.h"
namespace PN::Comm {
struct TrackerReportItem { QString number,name,identifiedBy,dateIdentified,description,assignedTo,priority,status,dueDate,lastUpdate,dateResolved,comments,itemType; bool internal=false; };
struct TrackerItemsReportInput { QString projectNumber,projectName; QList<TrackerReportItem> items; ReportOptions options; };
class TrackerItemsReportBuilder final { public: static std::optional<ReportDocument> build(const TrackerItemsReportInput &, ValidationResult * = nullptr); };
}
