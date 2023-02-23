#include "projectnotesmodel.h"
#include "pndatabaseobjects.h"

ProjectNotesModel::ProjectNotesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectNotesModel");

    setBaseSql("SELECT note_id, project_id, note_title, note_date, note, internal_item FROM project_notes");

    setTableName("project_notes", "Project Notes");

    addColumn(0, tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn(2,  tr("Title"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(3, tr("Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn(4, tr("Note"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(5, tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable);

    addRelatedTable("item_tracker", "note_id", "Action Item", DBExportable);
    addRelatedTable("meeting_attendees", "note_id", "Meeting Attendee", DBExportable);

    setOrderBy("note_date desc");
}

bool ProjectNotesModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

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
