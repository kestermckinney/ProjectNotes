// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "meetingattendeesmodel.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


MeetingAttendeesModel::MeetingAttendeesModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("MeetingAttendeesModel");

    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
    setBaseSql("SELECT meeting_attendees.id, meeting_attendees.note_id, meeting_attendees.person_id, name, (select p.project_name from projects p where p.id=(select n.project_id from project_notes n where n.id=meeting_attendees.note_id)) project_id_name, email, client_name, (select n.project_id from project_notes n where n.id=meeting_attendees.note_id) project_id, (select p.project_number from projects p where p.id=(select n.project_id from project_notes n where n.id=meeting_attendees.note_id)) project_number FROM meeting_attendees join people on people.id=meeting_attendees.person_id left join clients on clients.id=people.client_id");

    setTableName("meeting_attendees", "Attendees");

    addColumn("id", tr("Attendee ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBUnique);
    addColumn("note_id", tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "project_notes", "id", "(strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title)");
    addColumn("person_id", tr("Attendee"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "people", "id", "name");
    addColumn("name", tr("Attendee Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_id_name", tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("email", tr("Email"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("client_name", tr("Client Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_number", tr("Project Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    QStringList key1 = {"note_id", "people_id"};

    addUniqueKeys(key1, "Name");

    setOrderBy("people.name");
}

const QModelIndex MeetingAttendeesModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue2);

    QVector<QVariant> qr = emptyrecord();

    qr[1] = *fkValue1;

    return addRecord(qr);
}

