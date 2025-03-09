#include "statusreportitemsmodel.h"
#include "pndatabaseobjects.h"

StatusReportItemsModel::StatusReportItemsModel(PNDatabaseObjects* t_dbo): PNSqlQueryModel(t_dbo)
{
    setObjectName("StatusReportItemsModel");
    setOrderKey(40);

    setBaseSql("SELECT status_item_id, project_id, task_category, task_description FROM status_report_items");

    setTableName("status_report_items", "Status Report Items");

    addColumn("status_item_id", tr("Status Item ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn("task_category",  tr("Category"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &PNDatabaseObjects::status_item_status);
    addColumn("task_description", tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable);

    QStringList key1 = {"project_id", "task_description"};

    addUniqueKeys(key1, "Description");

    setOrderBy("status_item_id");
}

const QModelIndex StatusReportItemsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value1);
    Q_UNUSED(t_fk_value2);

    QVector<QVariant> qr = emptyrecord();

    qr[getColumnNumber("project_id")] = *t_fk_value1;
    qr[getColumnNumber("task_category")] = "In Progress";
    qr[getColumnNumber("task_description")] = "[New Status Item]";

    return addRecord(qr);
}
