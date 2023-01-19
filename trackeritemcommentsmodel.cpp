#include "trackeritemcommentsmodel.h"
#include "pndatabaseobjects.h"
#include <QDateTime>

TrackerItemCommentsModel::TrackerItemCommentsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("TrackerItemCommentsModel");

    setBaseSql("SELECT tracker_updated_id, item_id, lastupdated_date, update_note, updated_by FROM item_tracker_updates "), tr("tracker_updated_id");

    setTableName("item_tracker_updates", "Tracker Comments");

    addColumn(0, tr("Item Updated ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Item ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "item_tracker", "item_id", "item_number");
    addColumn(2, tr("Updated"), DBDate, DBSearchable, DBRequired);
    addColumn(3, tr("Comments"), DBString, DBSearchable, DBNotRequired);
    addColumn(4, tr("Updated By"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name"); // itemdetailteamlist, tr("name"), tr("people_id"), true );

    setOrderBy("lastupdated_date");
}

bool TrackerItemCommentsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    QSqlRecord qr = emptyrecord();

    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    qr.setValue(1, *t_fk_value1);
    qr.setValue(2, curdate); // default to today
    qr.setValue(3, QVariant());
    qr.setValue(4, global_DBObjects.getProjectManager()); // default updated by to the pm

    return addRecord(qr);
}


bool TrackerItemCommentsModel::setData(const QModelIndex &t_index, const QVariant &t_value, int t_role)
{
    QVariant curdate = QDateTime::currentDateTime().toString("MM/dd/yyyy");

    // set the date the record was updated
    if (t_index.column() != 2)
    {
        QVariant oldvalue = data(t_index, t_role);

        if (oldvalue != t_value) // don't change the update date if nothing changed
        {
            QModelIndex qmi = index(t_index.row(), 2);
            PNSqlQueryModel::setData(qmi, curdate, t_role);
        }
    }

    return PNSqlQueryModel::setData(t_index, t_value, t_role);
}
