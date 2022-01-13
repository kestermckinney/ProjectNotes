#include "trackeritemcommentsmodel.h"

TrackerItemCommentsModel::TrackerItemCommentsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("TrackerItemCommentsModel");

    setBaseSql("SELECT tracker_updated_id, item_id, lastupdated_date, update_note, updated_by FROM item_tracker_updates "), tr("tracker_updated_id");

    setTableName("item_tracker_updates", "Tracker Comments");

    AddColumn(0, tr("Item Updated ID"), DB_STRING, false, true, true, true);
    AddColumn(1, tr("Item ID"), DB_STRING, false, true, true, false);
    AddColumn(2, tr("Updated"), DB_DATE, true, true, true, false);
    AddColumn(3, tr("Comments"), DB_STRING, true, false, true, false);
    AddColumn(4, tr("Updated By"), DB_STRING, true, true, true, false); // itemdetailteamlist, tr("name"), tr("people_id"), true );

    SetOrderBy("lastupdated_date");
}
