#include "statusreportitemsmodel.h"

StatusReportItemsModel::StatusReportItemsModel(QObject* parent): PNSqlQueryModel(parent)
{
    setObjectName("StatusReportItemsModel");

    setBaseSql("SELECT status_item_id, project_id, task_category, task_description FROM status_report_items");

    setTableName("status_report_items", "Status Report Items");

    AddColumn(0, tr("Status Item ID"), DB_STRING, false, true, false, true);
    AddColumn(1, tr("Project ID"), DB_STRING, true, true, true, false);
    AddColumn(2,  tr("Category"), DB_STRING, true, true, true, false);
    AddColumn(3, tr("Description"), DB_STRING, true, true, true, true);

    AddRelatedTable("projects", "project_id", "Projects");

    SetOrderBy("status_item_id");
}
