#include "projectnotesmodel.h"
#include "pndatabaseobjects.h"

ProjectNotesModel::ProjectNotesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectNotesModel");

    setBaseSql("SELECT note_id, project_id, note_title, note_date, note, internal_item FROM project_notes");

    setTableName("project_notes", "Project Notes");

    addColumn(0, tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBSearchable, DBRequired, DBReadOnly);
    addColumn(2,  tr("Title"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(3, tr("Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn(4, tr("Note"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(5, tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable);


    addRelatedTable("item_tracker", "note_id", "Action Item");
    addRelatedTable("meeting_attendees", "note_id", "Meeting Attendee");


    setOrderBy("note_date desc");
}

bool ProjectNotesModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    QSqlRecord qr = emptyrecord();

    // determine the max note_id from the database, then determine the max number from the record cache in case new unsaved records were added
    QString note_id_string = global_DBObjects.execute(QString("select max(note_id) from project_locations where project_id = '%1'").arg(t_fk_value1->toString()));
    int note_id_int = note_id_string.toInt();

    for ( int i = 0; i < rowCount(QModelIndex()); i++ )
    {
        int testnumber = data(this->index(i, 0)).toInt();
        if (testnumber > note_id_int)
            note_id_int = testnumber;
    }

    note_id_int++;  // set one above the max
    //TODO:  the way i increment note id could be problme when i go to implemeny sycing

    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();
    QVariant notetitle = QString("[Meeting Notes for %1]").arg(QDateTime::currentDateTime().toString("MM/dd/yyyy"));

    qr.setValue(0, QString("%1").arg(note_id_int, 4, 10, QLatin1Char('0')));  // Need to make a counter that looks good for items
    qr.setValue(1, *t_fk_value1);
    qr.setValue(2, notetitle);
    qr.setValue(3, curdate);
    qr.setValue(4, QVariant());
    qr.setValue(5, 0);

    return addRecord(qr);
}

bool ProjectNotesModel::openRecord(QModelIndex t_index)
{
    QVariant location = data(index(t_index.row(), 4));
    QVariant location_type = data(index(t_index.row(), 2));

    //TODO: Implement open the meeting notes
/*
    if ( location_type == "Web Link" )
    {
        QDesktopServices::openUrl(QUrl(location.toString(), QUrl::TolerantMode));
    }
    else
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(location.toString()));
    }
*/
    return true;
}
