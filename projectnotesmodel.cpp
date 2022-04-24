#include "projectnotesmodel.h"

ProjectNotesModel::ProjectNotesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectNotesModel");

    setBaseSql("SELECT note_id, project_id, note_title, note_date, note, internal_item FROM project_notes");

    setTableName("project_notes", "Project Notes");

    addColumn(0, tr("Note ID"), DB_STRING, false, true, false, true);
    addColumn(1, tr("Project ID"), DB_STRING, true, true, false);
    addColumn(2,  tr("Title"), DB_STRING, true, false, true, false);
    addColumn(3, tr("Date"), DB_DATE, true, false, true, false);
    addColumn(4, tr("Note"), DB_STRING, true, false, true, false);
    addColumn(5, tr("Internal"), DB_BOOL, true, false, true, false);


    addRelatedTable("item_tracker", "note_id", "Action Item");
    addRelatedTable("meeting_attendees", "note_id", "Meeting Attendee");


    setOrderBy("note_date desc");
}
