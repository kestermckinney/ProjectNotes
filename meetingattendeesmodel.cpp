#include "meetingattendeesmodel.h"

MeetingAttendeesModel::MeetingAttendeesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("MeetingAttendeesModel");

    setBaseSql("SELECT meeting_attendees.attendee_id, meeting_attendees.note_id, meeting_attendees.person_id, name FROM meeting_attendees join people on people.people_id=meeting_attendees.person_id");

    setTableName("meeting_attendees", "Attendees");

    addColumn(0, tr("Attendee ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBUnique);
    addColumn(1, tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "project_notes", "note_id", "(strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title)");
    addColumn(2, tr("Attendee"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name");
    addColumn(3, tr("Attendee Name"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBNotUnique);

    QStringList key1 = {"note_id", "people_id"};

    addUniqueKeys(key1, "Name");

    setOrderBy("people.name");
}

bool MeetingAttendeesModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    QSqlRecord qr = emptyrecord();

    qr.setValue(1, *t_fk_value1);

    return addRecord(qr);
}

