#include "trackeritemcommentsmodel.h"
#include "databaseobjects.h"
#include <QDateTime>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


TrackerItemCommentsModel::TrackerItemCommentsModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("TrackerItemCommentsModel");
    setOrderKey(35);

    setBaseSql("SELECT id, item_id, lastupdated_date, update_note, updated_by, (select i.item_name from item_tracker i where i.id=u.item_id) item_name, (select i.item_number from item_tracker i where i.id=u.item_id) item_number, (select i.description from item_tracker i where i.id=u.item_id) description, (select p.project_name from projects p where p.id=(select i.project_id from item_tracker i where i.id=u.item_id)) project_name, (select p.project_number from projects p where p.id=(select i.project_id from item_tracker i where i.id=u.item_id)) project_number FROM item_tracker_updates u");

    setTableName("item_tracker_updates", "Tracker Comments");

    addColumn("id", tr("Item Updated ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("item_id", tr("Item ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "item_tracker", "id", "item_number");
    addColumn("lastupdated_date", tr("Updated"), DBDate, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn("update_note", tr("Comments"), DBString, DBSearchable, DBNotRequired);
    addColumn("updated_by", tr("Updated By"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "people", "id", "name"); // itemdetailteamlist, tr("name"), tr("people_id"), true );
    addColumn("item_name", tr("Item Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("item_number", tr("Item Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("description", tr("Item Description"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_name", tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_number", tr("Project Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    setOrderBy("lastupdated_date");
}

const QModelIndex TrackerItemCommentsModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue2);

    QVector<QVariant> qr = emptyrecord();

    QVariant curdate = QDateTime::currentSecsSinceEpoch();

    qr[1] = *fkValue1;
    qr[2] = curdate; // default to today
    qr[4] = getDBOs()->getProjectManager(); // default updated by to the pm

    return addRecord(qr);
}


bool TrackerItemCommentsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QVariant curdate = QDateTime::currentDateTime().toString("MM/dd/yyyy");
    QVariant oldvalue = data(index, role);

    //qDebug() << " for column " << index.column() << " got value " << value;

    if(SqlQueryModel::setData(index, value, role))
    {
        // set the date the record was updated
        if (index.column() != 2)
        {
            if (oldvalue != value) // don't change the update date if nothing changed
            {
                QModelIndex qmi = this->index(index.row(), 2);
                SqlQueryModel::setData(qmi, curdate, role);

                //qDebug() << "setting auto date " << curdate;
            }
        }

        return true;
    }

    return false;
}
