#include "projectnotesmodel.h"
#include "pndatabaseobjects.h"
#include <QDebug>

ProjectNotesModel::ProjectNotesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectNotesModel");

    setBaseSql("SELECT note_id, project_id, note_title, note_date, note, internal_item, (select project_name from projects p where p.project_id=n.project_id) project_id_name FROM project_notes n");

    setTableName("project_notes", "Project Notes");

    addColumn(0, tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn(2,  tr("Title"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(3, tr("Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn(4, tr("Note"), DBHtml, DBSearchable, DBNotRequired, DBEditable);
    addColumn(5, tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable);
    addColumn(6, tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);

    addRelatedTable("item_tracker", "note_id", "note_id", "Action Item", DBExportable);
    addRelatedTable("meeting_attendees", "note_id", "note_id", "Meeting Attendee", DBExportable);

    setOrderBy("note_date desc");
}

bool ProjectNotesModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    qDebug() << "Adding a new note with fk1: " << *t_fk_value1;

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

bool ProjectNotesModel::openRecord(QModelIndex t_index)
{
    QVariant note_id = data(index(t_index.row(), 0));

    global_DBObjects.projecteditingnotesmodel()->setFilter(0, note_id.toString());
    global_DBObjects.projecteditingnotesmodel()->refresh();

    // only select the records another event will be fired to open the window to show them
    global_DBObjects.meetingattendeesmodel()->setFilter(1, note_id.toString());
    global_DBObjects.meetingattendeesmodel()->refresh();

    global_DBObjects.notesactionitemsmodel()->setFilter(13, note_id.toString());
    global_DBObjects.notesactionitemsmodel()->refresh();

    return true;
}

bool ProjectNotesModel::setData(const QModelIndex &t_index, const QVariant &t_value, int t_role)
{
    bool wasnew = isNewRecord(t_index);
    bool result = PNSqlQueryModel::setData(t_index, t_value, t_role);

    if (wasnew && result)
    {
        QString note_id = data(index(t_index.row(), 0)).toString();
        global_DBObjects.addDefaultPMToMeeting(note_id);
    }

    return result;
}

bool ProjectNotesModel::copyRecord(QModelIndex t_index)
{
    QSqlRecord qr = emptyrecord();

    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    qr.setValue(1, data(index(t_index.row(), 1)));
    qr.setValue(2, data(index(t_index.row(), 2)));
    qr.setValue(3, curdate);
    //qr.setValue(4, QVariant());
    qr.setValue(5, 0);

    QModelIndex qi = addRecordIndex(qr);
    setData( index(qi.row(), 4), QVariant(), Qt::EditRole);

    QVariant oldid = data(index(t_index.row(), 0));
    QVariant newid = data(index(qi.row(), 0));

    QString insert = "insert into meeting_attendees (attendee_id, note_id, person_id) select m.attendee_id || '-cp', '" + newid.toString() + "', m.person_id from meeting_attendees m where m.note_id ='" + oldid.toString() + "'  and m.person_id not in (select e.person_id from meeting_attendees e where e.note_id='" + newid.toString() + "')";

    global_DBObjects.execute(insert);
    global_DBObjects.meetingattendeesmodel()->setDirty();

    return true;
}
