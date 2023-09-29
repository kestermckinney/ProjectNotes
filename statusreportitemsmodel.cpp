#include "statusreportitemsmodel.h"
#include "pndatabaseobjects.h"

StatusReportItemsModel::StatusReportItemsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("StatusReportItemsModel");
    setOrderKey(40);

    setBaseSql("SELECT status_item_id, project_id, task_category, task_description FROM status_report_items");

    setTableName("status_report_items", "Status Report Items");

    addColumn(0, tr("Status Item ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn(2,  tr("Category"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &PNDatabaseObjects::status_item_status);
    addColumn(3, tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable);

    QStringList key1 = {"project_id", "task_description"};

    addUniqueKeys(key1, "Description");

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
