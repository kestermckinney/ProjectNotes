// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectnotesmodel.h"
#include "pndatabaseobjects.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


ProjectNotesModel::ProjectNotesModel(PNDatabaseObjects* t_dbo, bool t_gui): PNSqlQueryModel(t_dbo, t_gui)
{
    setObjectName("ProjectNotesModel");
    setOrderKey(50);

    setBaseSql("SELECT note_id, project_id, note_title, note_date, note, internal_item, (select project_name from projects p where p.project_id=n.project_id) project_id_name, (select project_number from projects p where p.project_id=n.project_id) project_id_number FROM project_notes n");

    setTableName("project_notes", "Project Notes");

    addColumn(0, tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn(2,  tr("Title"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(3, tr("Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn(4, tr("Note"), DBHtml, DBSearchable, DBNotRequired, DBEditable);
    addColumn(5, tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable);
    addColumn(6, tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);
    addColumn(7, tr("Project Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);

    addRelatedTable("item_tracker", "note_id", "note_id", "Action Item", DBExportable);
    addRelatedTable("meeting_attendees", "note_id", "note_id", "Meeting Attendee", DBExportable);

    setOrderBy("note_date desc");
}

const QModelIndex ProjectNotesModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    //qDebug() << "Adding a new note with fk1: " << *t_fk_value1;

    QSqlRecord qr = emptyrecord();

    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();
    QVariant notetitle = QString("[Meeting Notes for %1]").arg(QDateTime::currentDateTime().toString("MM/dd/yyyy"));

    qr.setValue(1, *t_fk_value1);
    qr.setValue(2, notetitle);
    qr.setValue(3, curdate);
    qr.setValue(4, QVariant());
    qr.setValue(5, 0);

    return addRecord(qr);
}

bool ProjectNotesModel::setData(const QModelIndex &t_index, const QVariant &t_value, int t_role)
{
    bool wasnew = isNewRecord(t_index);
    bool result = PNSqlQueryModel::setData(t_index, t_value, t_role);

    if (wasnew && result)
    {
        QString note_id = data(index(t_index.row(), 0)).toString();
        getDBOs()->addDefaultPMToMeeting(note_id);
    }

    return result;
}

const QModelIndex ProjectNotesModel::copyRecord(QModelIndex t_index)
{
    QSqlRecord qr = emptyrecord();
    QString unique_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");

    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    qr.setValue(1, data(index(t_index.row(), 1)));
    qr.setValue(2, data(index(t_index.row(), 2)));
    qr.setValue(3, curdate);
    //qr.setValue(4, QVariant());
    qr.setValue(5, 0);

    QModelIndex qi = addRecord(qr);
    setData( index(qi.row(), 4), QVariant(), Qt::EditRole);

    QVariant oldid = data(index(t_index.row(), 0));
    QVariant newid = data(index(qi.row(), 0));

    QString insert = "insert into meeting_attendees (attendee_id, note_id, person_id) select m.attendee_id || '-" + unique_stamp + "', '" + newid.toString() + "', m.person_id from meeting_attendees m where m.note_id ='" + oldid.toString() + "'  and m.person_id not in (select e.person_id from meeting_attendees e where e.note_id='" + newid.toString() + "')";

    getDBOs()->execute(insert);
    getDBOs()->meetingattendeesmodel()->setDirty();

    return qi;
}
