#include "projectnotesmodel.h"

ProjectNotesModel::ProjectNotesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectNotesModel");

    setBaseSql("SELECT note_id, project_id, note_title, note_date, note, internal_item FROM project_notes");

    setTableName("project_notes", "Project Notes");

    addColumn(0, tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBSearchable, DBRequired, DBReadOnly);
    addColumn(2,  tr("Title"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(3, tr("Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn(4, tr("Note"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(5, tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable);


    addRelatedTable("item_tracker", "note_id", "Action Item");
    addRelatedTable("meeting_attendees", "note_id", "Meeting Attendee");


    setOrderBy("note_date desc");
}
