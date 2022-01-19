#include "actionitemprojectnotesmodel.h"

ActionItemProjectNotesModel::ActionItemProjectNotesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ActionItemProjectNotesModel");

    setBaseSql("SELECT note_id, project_id, (strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title) as meeting, internal_item FROM project_notes");

    setTableName("project_notes", "Project Notes");

    addColumn(0, tr("Note ID"), DB_STRING, false, true, false, false);
    addColumn(1, tr("Project ID"), DB_STRING, false, true, false, false);
    addColumn(2, tr("Meeting"), DB_STRING, false, true, false, false);
    addColumn(3, tr("Internal Item"), DB_BOOL, false, true, false, false);

    //addRelatedTable("item_tracker", "assigned_to", "Assigned Item");

    setOrderBy("note_date");
}
