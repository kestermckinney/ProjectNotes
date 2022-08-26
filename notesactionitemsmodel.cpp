#include "notesactionitemsmodel.h"

NotesActionItemsModel::NotesActionItemsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("NotesActionItemsModel");

    setBaseSql("SELECT item_id, item_number, item_type, item_name, identified_by, date_identified, description, assigned_to, priority, status, date_due, last_update, date_resolved, note_id, project_id, internal_item FROM item_tracker");

    setTableName("item_tracker", "Notes Action Items");

    addColumn(0, tr("Item ID"), DBString, DBSearchable, DBRequired, DBReadOnly);
    addColumn(1, tr("Item"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique);
    addColumn(2, tr("t_type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(3, tr("Item Name"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(4, tr("Identified By"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(5, tr("Date Identified"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(6, tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(7, tr("Assigned To"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(8, tr("Priority"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(9, tr("Status"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(10, tr("Date Due"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(11, tr("Updated"), DBDate, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(12, tr("Date Resolved"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(13, tr("Note Date"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(14, tr("Project ID"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(15, tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addRelatedTable("item_tracker_updates", "item_id", "Tracker Updates");

    setOrderBy("item_number");
}
