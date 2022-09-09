#include "trackeritemcommentsmodel.h"

TrackerItemCommentsModel::TrackerItemCommentsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("TrackerItemCommentsModel");

    setBaseSql("SELECT tracker_updated_id, item_id, lastupdated_date, update_note, updated_by FROM item_tracker_updates "), tr("tracker_updated_id");

    setTableName("item_tracker_updates", "Tracker Comments");

    addColumn(0, tr("Item Updated ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Item ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    addColumn(2, tr("Updated"), DBDate, DBSearchable, DBRequired);
    addColumn(3, tr("Comments"), DBString, DBSearchable, DBNotRequired);
    addColumn(4, tr("Updated By"), DBString, DBSearchable, DBRequired); // itemdetailteamlist, tr("name"), tr("people_id"), true );

    setOrderBy("lastupdated_date");
}

// TODO: Add new record implmentation
// TODO: Add overrident update to change the updated date
