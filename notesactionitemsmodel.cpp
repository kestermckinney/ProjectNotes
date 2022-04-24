#include "notesactionitemsmodel.h"

NotesActionItemsModel::NotesActionItemsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("NotesActionItemsModel");

    setBaseSql("SELECT item_id, item_number, item_type, item_name, identified_by, date_identified, description, assigned_to, priority, status, date_due, last_update, date_resolved, note_id, project_id, internal_item FROM item_tracker");

    setTableName("item_tracker", "Notes Action Items");

    addColumn(0, tr("Item ID"), DB_STRING, false, true, true, true);
    addColumn(1, tr("Item"), DB_STRING, true, true, true, false);
    addColumn(2, tr("t_type"), DB_STRING, true, true, true, false); //item_type, 2);
    addColumn(3, tr("Item Name"), DB_STRING, true, false, true, false);
    addColumn(4, tr("Identified By"), DB_STRING, true, false, true, false); // teamlist, tr("name"), tr("people_id"), false ); // made not required because it broke when action item detail changed project numbers
    addColumn(5, tr("Date Identified"), DB_DATE, true, false, true, false);
    addColumn(6, tr("Description"), DB_STRING, true, false, true, false);
    addColumn(7, tr("Assigned To"), DB_STRING, true, false, true, false); // teamlist, tr("name"), tr("people_id") );
    addColumn(8, tr("Priority"), DB_STRING, true, true, true, false);// item_priority, 3);
    addColumn(9, tr("Status"), DB_STRING, true, true, true, false);//, true, item_status, 5);
    addColumn(10, tr("Date Due"), DB_DATE, true, false, true, false);
    addColumn(11, tr("Updated"), DB_DATE, true, true, true, false);
    addColumn(12, tr("Date Resolved"), DB_DATE, true, false, true, false);
    addColumn(13, tr("Note Date"), DB_STRING, true, false, true, false); // actionitemprojectnotes, tr("note_date"), tr("note_id"));
    addColumn(14, tr("Project ID"), DB_STRING, true, false, true, false);
    addColumn(15, tr("Internal"), DB_BOOL, true, false, true, false);

    addRelatedTable("item_tracker_updates", "item_id", "Tracker Updates");

    setOrderBy("item_number");
}
