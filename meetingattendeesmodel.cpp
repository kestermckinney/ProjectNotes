#include "meetingattendeesmodel.h"

MeetingAttendeesModel::MeetingAttendeesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("MeetingAttendeesModel");

    setBaseSql("SELECT meeting_attendees.attendee_id, meeting_attendees.note_id, meeting_attendees.person_id, name FROM meeting_attendees join people on people.people_id=meeting_attendees.person_id");

    setTableName("meeting_attendees", "Attendees");

    addColumn(0, tr("Attendee ID"), DB_STRING, false, true, true, true);
    addColumn(1, tr("Note ID"), DB_STRING, false, true, true, false);
    addColumn(2, tr("Attendee"), DB_STRING, false, true, true, false); //, true, teamlist, wxT("name"), wxT("people_id"), true );
    addColumn(3, tr("Attendee Name"), DB_STRING, false, true, true, false);

    //addRelatedTable("item_tracker", "assigned_to", "Assigned Item");

    setOrderBy("people.name");
}
