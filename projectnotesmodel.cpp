// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectnotesmodel.h"
#include "databaseobjects.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

#include <QSqlQuery>
#include <QUuid>

using namespace QLogger;


ProjectNotesModel::ProjectNotesModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("ProjectNotesModel");

    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
    setBaseSql("SELECT project_notes.id, project_id, note_title, note_date, note, internal_item, (select project_name from projects p where p.id=project_notes.project_id) project_name, (select project_number from projects p where p.id=project_notes.project_id) project__number FROM project_notes ");

    setTableName("project_notes", "Project Notes");

    addColumn("id", tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "id", "project_number");
    addColumn("note_title",  tr("Title"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn("note_date", tr("Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn("note", tr("Note"), DBHtml, DBSearchable, DBNotRequired, DBEditable);
    addColumn("internal_item", tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable);
    addColumn("project_name", tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);
    addColumn("project_number", tr("Project Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);

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
    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    qr[1] = data(this->index(index.row(), 1));
    qr[2] = data(this->index(index.row(), 2));
    qr[3] = curdate;
    qr[5] = 0;

    // addRecord stages the row in the cache; insertCacheRow assigns a GUID and
    // INSERTs it into the database (with its own DB_LOCK).  Using insertCacheRow
    // directly also avoids triggering addDefaultPMToMeeting via setData, which
    // must not run during a copy because the attendees are copied explicitly below.
    QModelIndex qi = addRecord(qr);
    insertCacheRow(qi.row());

    QVariant oldid = data(this->index(index.row(), 0));
    QVariant newid = data(this->index(qi.row(), 0));

    // Copy meeting_attendees with new GUIDs for each row
    DB_LOCK;
    QSqlQuery select(getDBOs()->getDb());
    select.prepare("SELECT person_id FROM meeting_attendees "
                   "WHERE note_id = ? AND deleted = 0 "
                   "AND person_id NOT IN (SELECT person_id FROM meeting_attendees WHERE note_id = ? AND deleted = 0)");
    select.addBindValue(oldid);
    select.addBindValue(newid);

    if (select.exec())
    {
        QSqlQuery insert(getDBOs()->getDb());
        insert.prepare("INSERT INTO meeting_attendees (id, note_id, person_id) VALUES (?, ?, ?)");

        while (select.next())
        {
            QString aid = QUuid::createUuid().toString();

            insert.addBindValue(aid);
            insert.addBindValue(newid);
            insert.addBindValue(select.value(0));
            insert.exec();

            getDBOs()->pushRowChange("meeting_attendees", aid, KeyColumnChange::Insert);
        }
    }
    DB_UNLOCK;

    getDBOs()->updateDisplayData();

    return qi;
}
