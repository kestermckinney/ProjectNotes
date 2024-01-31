#include "trackeritemcommentsmodel.h"
#include "pndatabaseobjects.h"
#include <QDateTime>
//#include <QDebug>

TrackerItemCommentsModel::TrackerItemCommentsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("TrackerItemCommentsModel");
    setOrderKey(35);

    setBaseSql("SELECT tracker_updated_id, item_id, lastupdated_date, update_note, updated_by, (select i.item_name from item_tracker i where i.item_id=u.item_id) item_name, (select i.item_number from item_tracker i where i.item_id=u.item_id) item_number, (select i.description from item_tracker i where i.item_id=u.item_id) description, (select p.project_name from projects p where p.project_id=(select i.project_id from item_tracker i where i.item_id=u.item_id)) project_name, (select p.project_number from projects p where p.project_id=(select i.project_id from item_tracker i where i.item_id=u.item_id)) project_number FROM item_tracker_updates u");

    setTableName("item_tracker_updates", "Tracker Comments");

    addColumn(0, tr("Item Updated ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Item ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "item_tracker", "item_id", "item_number");
    addColumn(2, tr("Updated"), DBDate, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(3, tr("Comments"), DBString, DBSearchable, DBNotRequired);
    addColumn(4, tr("Updated By"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name"); // itemdetailteamlist, tr("name"), tr("people_id"), true );
    addColumn(5, tr("Item Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(6, tr("Item Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(7, tr("Item Description"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(8, tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(9, tr("Project Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    setOrderBy("lastupdated_date");
}

const QModelIndex TrackerItemCommentsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
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
    QVariant oldvalue = data(t_index, t_role);

    //qDebug() << " for column " << t_index.column() << " got value " << t_value;

    if(PNSqlQueryModel::setData(t_index, t_value, t_role))
    {
        // set the date the record was updated
        if (t_index.column() != 2)
        {
            if (oldvalue != t_value) // don't change the update date if nothing changed
            {
                QModelIndex qmi = index(t_index.row(), 2);
                PNSqlQueryModel::setData(qmi, curdate, t_role);

                //qDebug() << "setting auto date " << curdate;
            }
        }

        global_DBObjects.trackeritemsmodel()->setDirty(); // the combined comments can't be set relatable

        return true;
    }

    return false;
}
