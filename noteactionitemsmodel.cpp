#include "noteactionitemsmodel.h"

NoteActionItemsModel::NoteActionItemsModel(QObject* parent): PNSqlQueryModel(parent)
{
    setBaseSql("SELECT item_id, item_number, item_type, item_name, identified_by, date_identified, description, assigned_to, priority, status, date_due, last_update, date_resolved, note_id, project_id, internal_item FROM item_tracker");

    setTableName("Action Items", "item_tracker");

    AddColumn(0, wxT("Item ID"), DB_STRING, false, true, true);
    AddColumn(1, wxT("Item"), DB_STRING, true, true, false, false);
    AddColumn(2, wxT("Type"), DB_STRING, true, true, true, false); // item_type, 2);
    AddColumn(3, wxT("Item Name"), DB_STRING, true, false, true, false);
    AddColumn(4, wxT("Identified By"), DB_STRING, true, false, true, false); // teamlist, wxT("name"), wxT("people_id"), false ); // made not required because it broke when action item detail changed project numbers
    AddColumn(5, wxT("Date Identified"), DB_DATE, true, false, true, false);
    AddColumn(6, wxT("Description"), DB_STRING, true, false, true, false);
    AddColumn(7, wxT("Assigned To"), DB_STRING, true, false, true, false); // teamlist, wxT("name"), wxT("people_id") );
    AddColumn(8, wxT("Priority"), DB_STRING, true, true, true, false); // item_priority, 3);
    AddColumn(9, wxT("Status"), DB_STRING, true, true, true, false); // item_status, 5);
    AddColumn(10, wxT("Date Due"), DB_DATE, true, false, true, false);
    AddColumn(11, wxT("Updated"), DB_DATE, false,);
    AddColumn(12, wxT("Date Resolved"), DB_DATE);
    AddLookupColumn(13, wxT("Note Date"), DB_STRING, true, actionitemprojectnotes, wxT("note_date"), wxT("note_id"));
    AddColumn(14, wxT("Project ID"), DB_STRING);
    AddColumn(15, wxT("Internal"), DB_BOOL);

    AddRelatedTable("item_tracker", "assigned_to", "Assigned Item");
    AddRelatedTable("item_tracker", "identified_by", "Identified By Item");
    AddRelatedTable("item_tracker_updates", "updated_by", "Item Updated By");
    AddRelatedTable("meeting_attendees", "person_id", "Meeting Attendee");
    AddRelatedTable("project_people", "people_id", "Project Team");
    AddRelatedTable("projects", "primary_contact", "Project Primary Contact");

    SetOrderBy("item_number");
}
