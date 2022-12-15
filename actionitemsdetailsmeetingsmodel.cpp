#include "actionitemsdetailsmeetingsmodel.h"

ActionItemsDetailsMeetingsModel::ActionItemsDetailsMeetingsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ActionItemsDetailsMeetingsModel");

    setBaseSql("SELECT note_id, project_id, (strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title) as meeting, internal_item FROM project_notes");

    setTableName("project_notes", "Notes");

    addColumn(0, tr("Note ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(1, tr("Project ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(2, tr("Meeting"), DBString, DBNotSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(3, tr("Internal Item"), DBBool, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    setOrderBy("note_date");
}
