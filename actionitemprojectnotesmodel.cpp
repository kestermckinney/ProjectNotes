#include "actionitemprojectnotesmodel.h"

ActionItemProjectNotesModel::ActionItemProjectNotesModel(QObject* parent): PNSqlQueryModel(parent)
{
    setBaseSql("SELECT note_id, project_id, (strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title) as meeting, internal_item FROM project_notes");

    setTableName("project_notes", "Project Notes");

    AddColumn(0, tr("Note ID"), DB_STRING, false, true, false, false);
    AddColumn(1, tr("Project ID"), DB_STRING, false, true, false, false);
    AddColumn(2, tr("Meeting"), DB_STRING, false, true, false, false);
    AddColumn(3, tr("Internal Item"), DB_BOOL, false, true, false, false);

    //AddRelatedTable("item_tracker", "assigned_to", "Assigned Item");

    SetOrderBy("note_date");
}
