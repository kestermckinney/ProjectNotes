#include "actionitemprojectnotesmodel.h"

ActionItemProjectNotesModel::ActionItemProjectNotesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ActionItemProjectNotesModel");

    setBaseSql("SELECT note_id, project_id, (strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title) as meeting, internal_item FROM project_notes");

    setTableName("project_notes", "Project Notes");

    addColumn(0, tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(2, tr("Meeting"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBNotUnique);
    addColumn(3, tr("Internal Item"), DBBool, DBNotSearchable, DBRequired, DBReadOnly, DBNotUnique);

    //addRelatedTable("item_tracker", "assigned_to", "Assigned Item");

    setOrderBy("note_date");
}
