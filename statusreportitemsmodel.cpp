#include "statusreportitemsmodel.h"

StatusReportItemsModel::StatusReportItemsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("StatusReportItemsModel");

    setBaseSql("SELECT status_item_id, project_id, task_category, task_description FROM status_report_items");

    setTableName("status_report_items", "Status Report Items");

    addColumn(0, tr("Status Item ID"), DB_STRING, false, true, false, true);
    addColumn(1, tr("Project ID"), DB_STRING, true, true, true, false);
    addColumn(2,  tr("Category"), DB_STRING, true, true, true, false);
    addColumn(3, tr("Description"), DB_STRING, true, true, true, true);

    addRelatedTable("projects", "project_id", "Projects");

    setOrderBy("status_item_id");
}
