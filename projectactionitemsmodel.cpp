#include "projectactionitemsmodel.h"

ProjectActionItemsModel::ProjectActionItemsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectActionItemsModel");

    setBaseSql("SELECT item_id, item_number, item_type, item_name, identified_by, date_identified, description, assigned_to, priority, status, date_due, last_update, date_resolved, note_id, project_id, internal_item , (select GROUP_CONCAT(update_note, ',') from item_tracker_updates where item_tracker.item_id=item_tracker_updates.item_id ) comments, (select project_status from projects p where p.project_id=item_tracker.project_id) project_status, (select client_id from projects p where p.project_id=item_tracker.project_id) client_id FROM item_tracker ");

    setTableName("item_tracker", "Project Action Items");

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

    addColumn(13, tr("Meeting"), DB_STRING, true, false, true, false);// actionitemprojectnotes, wxT("meeting"), wxT("note_id"));
    addColumn(14, tr("Project"), DB_STRING, true, false, true, false); //, true, projects, wxT("project_number"), wxT("project_id") );
    addColumn(15, tr("Internal"), DB_BOOL, true, false, true, false);
    addColumn(16, tr("Comments"), DB_STRING, true, false, false, false);
    addColumn(17, tr("Project Status"), DB_STRING, true, false, true, false);
    addColumn(18, tr("Client"), DB_STRING,  true, false, true, false);

    addRelatedTable("item_tracker_updates", "item_id", "Tracker Updates");

    setOrderBy("item_number");
}


bool ProjectActionItemsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value1);
    Q_UNUSED(t_fk_value2);

    QSqlRecord qr = emptyrecord();

    qr.setValue("project_id", *t_fk_value1);

    return addRecord(qr);
}
