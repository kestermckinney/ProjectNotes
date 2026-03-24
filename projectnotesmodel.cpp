// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectnotesmodel.h"
#include "databaseobjects.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


ProjectNotesModel::ProjectNotesModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("ProjectNotesModel");

    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
    setBaseSql("SELECT project_notes.id, project_id, note_title, note_date, note, internal_item, (select project_name from projects p where p.id=project_notes.project_id) project_id_name, (select project_number from projects p where p.id=project_notes.project_id) project_id_number FROM project_notes ");

    setTableName("project_notes", "Project Notes");

    addColumn("id", tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "id", "project_number");
    addColumn("note_title",  tr("Title"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn("note_date", tr("Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn("note", tr("Note"), DBHtml, DBSearchable, DBNotRequired, DBEditable);
    addColumn("internal_item", tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable);
    addColumn("project_id_name", tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);
    addColumn("project_id_number", tr("Project Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);

    addRelatedTable("item_tracker", "note_id", "id", "Action Item", DBExportable);
    addRelatedTable("meeting_attendees", "note_id", "id", "Meeting Attendee", DBExportable);

    setOrderBy("note_date desc");
}

const QModelIndex ProjectNotesModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue2);

    //qDebug() << "Adding a new note with fk1: " << *fkValue1;

    QVector<QVariant> qr = emptyrecord();

    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();
    QVariant notetitle = QString("[Meeting Notes for %1]").arg(QDateTime::currentDateTime().toString("MM/dd/yyyy"));

    qr[1] = *fkValue1;
    qr[2] = notetitle;
    qr[3] = curdate;
    qr[5] = 0;

    return addRecord(qr);
}

bool ProjectNotesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool wasnew = isNewRecord(index);
    bool result = SqlQueryModel::setData(index, value, role);

    if (wasnew && result)
    {
        QString note_id = data(this->index(index.row(), 0)).toString();
        getDBOs()->addDefaultPMToMeeting(note_id);
    }

    return result;
}

const QModelIndex ProjectNotesModel::copyRecord(QModelIndex index)
{
    QVector<QVariant> qr = emptyrecord();
    QString unique_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");
    //TODO: fix this to use GUID like other copy
    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    qr[1] = data(this->index(index.row(), 1));
    qr[2] = data(this->index(index.row(), 2));
    qr[3] = curdate;
    //qr[4, QVariant());
    qr[5] = 0;

    QModelIndex qi = addRecord(qr);
    setData( this->index(qi.row(), 4), QVariant(), Qt::EditRole);

    QVariant oldid = data(this->index(index.row(), 0));
    QVariant newid = data(this->index(qi.row(), 0));

    QString insert = "insert into meeting_attendees (id, note_id, person_id) select m.id || '-" + unique_stamp + "', '" + newid.toString() + "', m.person_id from meeting_attendees m where m.note_id ='" + oldid.toString() + "' and deleted = 0 and m.person_id not in (select e.person_id from meeting_attendees e where e.note_id='" + newid.toString() + "' and deleted = 0)";

    getDBOs()->execute(insert);
    getDBOs()->pushRowChange("meeting_attendees", newid, KeyColumnChange::Insert);
    getDBOs()->updateDisplayData();

    return qi;
}
