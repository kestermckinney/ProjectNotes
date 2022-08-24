#include "statusreportitemsmodel.h"

StatusReportItemsModel::StatusReportItemsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("StatusReportItemsModel");

    setBaseSql("SELECT status_item_id, project_id, task_category, task_description FROM status_report_items");

    setTableName("status_report_items", "Status Report Items");

    addColumn(0, tr("Status Item ID"), DB_STRING, false, true, true, true);
    addColumn(1, tr("Project ID"), DB_STRING, false, true, true, false);
    addColumn(2,  tr("Category"), DB_STRING, true, true, true, false);
    addColumn(3, tr("Description"), DB_STRING, true, false, true, false);

    addRelatedTable("projects", "project_id", "Projects");

    setOrderBy("status_item_id");
}

bool StatusReportItemsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value1);
    Q_UNUSED(t_fk_value2);

    QSqlRecord qr = emptyrecord();

    qr.setValue("project_id", *t_fk_value1);
    qr.setValue("task_category", "In Progress");
    qr.setValue("task_description", "[New Status Item]");

    return addRecord(qr);
}
