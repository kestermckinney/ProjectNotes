#include "projectactionitemsmodel.h"

ProjectActionItemsModel::ProjectActionItemsModel(QObject* parent): PNSqlQueryModel(parent)
{
    setObjectName("ProjectActionItemsModel");

    setBaseSql("SELECT item_id, item_number, item_type, item_name, identified_by, date_identified, description, assigned_to, priority, status, date_due, last_update, date_resolved, note_id, project_id, internal_item , (select GROUP_CONCAT(update_note, ',') from item_tracker_updates where item_tracker.item_id=item_tracker_updates.item_id ) comments, (select project_status from projects p where p.project_id=item_tracker.project_id) project_status, (select client_id from projects p where p.project_id=item_tracker.project_id) client_id FROM item_tracker ");

    setTableName("item_tracker", "Project Action Items");

    AddColumn(0, tr("Item ID"), DB_STRING, false, true, true, true);
    AddColumn(1, tr("Item"), DB_STRING, true, true, true, false);
    AddColumn(2, tr("Type"), DB_STRING, true, true, true, false); //item_type, 2);
    AddColumn(3, tr("Item Name"), DB_STRING, true, false, true, false);
    AddColumn(4, tr("Identified By"), DB_STRING, true, false, true, false); // teamlist, tr("name"), tr("people_id"), false ); // made not required because it broke when action item detail changed project numbers
    AddColumn(5, tr("Date Identified"), DB_DATE, true, false, true, false);
    AddColumn(6, tr("Description"), DB_STRING, true, false, true, false);
    AddColumn(7, tr("Assigned To"), DB_STRING, true, false, true, false); // teamlist, tr("name"), tr("people_id") );
    AddColumn(8, tr("Priority"), DB_STRING, true, true, true, false);// item_priority, 3);
    AddColumn(9, tr("Status"), DB_STRING, true, true, true, false);//, true, item_status, 5);
    AddColumn(10, tr("Date Due"), DB_DATE, true, false, true, false);
    AddColumn(11, tr("Updated"), DB_DATE, true, true, true, false);
    AddColumn(12, tr("Date Resolved"), DB_DATE, true, false, true, false);

    AddColumn(13, tr("Meeting"), DB_STRING, true, false, true, false);// actionitemprojectnotes, wxT("meeting"), wxT("note_id"));
    AddColumn(14, tr("Project"), DB_STRING, true, false, true, false); //, true, projects, wxT("project_number"), wxT("project_id") );
    AddColumn(15, tr("Internal"), DB_BOOL, true, false, true, false);
    AddColumn(16, tr("Comments"), DB_STRING, true, false, false, false);
    AddColumn(17, tr("Project Status"), DB_STRING, true, false, true, false);
    AddColumn(18, tr("Client"), DB_STRING,  true, false, true, false);

    AddRelatedTable("item_tracker_updates", "item_id", "Tracker Updates");

    SetOrderBy("item_number");
}
