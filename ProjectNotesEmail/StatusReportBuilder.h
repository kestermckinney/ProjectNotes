#pragma once
#include "ReportTypes.h"
#include "reports/ReportFormatters.h"
namespace PN::Comm {
struct StatusIssue { QString name,assignedTo,priority,dueDate,status; bool internal=false; };
struct StatusReportInput {
    QString projectNumber,projectName,managerName,reportingPeriod,budget,actual,bcwp,bcws,bac;
    QStringList stakeholders,activitiesInProgress,activitiesNextPeriod,activitiesCompleted;
    QList<StatusIssue> issues; QDate reportingDate; bool internalReport=false;
};
class StatusReportBuilder final { public: static std::optional<ReportDocument> build(const StatusReportInput &, ValidationResult * = nullptr); };
}
