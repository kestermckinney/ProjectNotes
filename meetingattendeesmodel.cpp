// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "meetingattendeesmodel.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


MeetingAttendeesModel::MeetingAttendeesModel(PNDatabaseObjects* t_dbo): PNSqlQueryModel(t_dbo)
{
    setObjectName("MeetingAttendeesModel");
    setOrderKey(40);

    setBaseSql("SELECT m.attendee_id, m.note_id, m.person_id, name, (select p.project_name from projects p where p.project_id=(select n.project_id from project_notes n where n.note_id=m.note_id)) project_id_name, email, client_name, (select n.project_id from project_notes n where n.note_id=m.note_id) project_id, (select p.project_number from projects p where p.project_id=(select n.project_id from project_notes n where n.note_id=m.note_id)) project_number FROM meeting_attendees m join people on people.people_id=m.person_id left join clients on clients.client_id=people.client_id");

    setTableName("meeting_attendees", "Attendees");

    addColumn("attendee_id", tr("Attendee ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBUnique);
    addColumn("note_id", tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "project_notes", "note_id", "(strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title)");
    addColumn("person_id", tr("Attendee"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name");
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

const QModelIndex MeetingAttendeesModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    QVector<QVariant> qr = emptyrecord();

    //qDebug() << "note id key " << *t_fk_value1;

    qr[1] = *t_fk_value1;

    return addRecord(qr);
}

