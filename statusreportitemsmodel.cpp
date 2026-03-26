#include "statusreportitemsmodel.h"
#include "databaseobjects.h"

StatusReportItemsModel::StatusReportItemsModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("StatusReportItemsModel");

    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
    setBaseSql("SELECT id, project_id, task_category, task_description FROM status_report_items");

    setTableName("status_report_items", "Status Report Items");

    addColumn("id", tr("Status Item ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "id", "project_number");
    addColumn("task_category",  tr("Category"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &DatabaseObjects::status_item_status);
    addColumn("task_description", tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable);

    QStringList key1 = {"project_id", "task_description"};

    addUniqueKeys(key1, "Description");

    setOrderBy("project_id");
}

const QModelIndex StatusReportItemsModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue1);
    Q_UNUSED(fkValue2);

    QVector<QVariant> qr = emptyrecord();

    qr[getColumnNumber("project_id")] = *fkValue1;
    qr[getColumnNumber("task_category")] = "In Progress";
    qr[getColumnNumber("task_description")] = "[New Status Item]";

    return addRecord(qr);
}
