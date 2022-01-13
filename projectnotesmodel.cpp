#include "projectnotesmodel.h"

ProjectNotesModel::ProjectNotesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectNotesModel");

    setBaseSql("SELECT note_id, project_id, note_title, note_date, note, internal_item FROM project_notes");

    setTableName("project_notes", "Project Notes");

    AddColumn(0, tr("Note ID"), DB_STRING, false, true, false, true);
    AddColumn(1, tr("Project ID"), DB_STRING, true, true, false);
    AddColumn(2,  tr("Title"), DB_STRING, true, false, true, false);
    AddColumn(3, tr("Date"), DB_DATE, true, false, true, false);
    AddColumn(4, tr("Note"), DB_STRING, true, false, true, false);
    AddColumn(5, tr("Internal"), DB_BOOL, true, false, true, false);


    AddRelatedTable("item_tracker", "note_id", "Action Item");
    AddRelatedTable("meeting_attendees", "note_id", "Meeting Attendee");


    SetOrderBy("note_date desc");
}
