#include "projectactionitemsmodel.h"
#include "pndatabaseobjects.h"

ProjectActionItemsModel::ProjectActionItemsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectActionItemsModel");

    setBaseSql("SELECT item_id, item_number, item_type, item_name, identified_by, date_identified, description, assigned_to, priority, status, date_due, last_update, date_resolved, note_id, project_id, internal_item , (select GROUP_CONCAT(update_note, ',') from item_tracker_updates where item_tracker.item_id=item_tracker_updates.item_id ) comments, (select project_status from projects p where p.project_id=item_tracker.project_id) project_status, (select client_id from projects p where p.project_id=item_tracker.project_id) client_id FROM item_tracker ");

    setTableName("item_tracker", "Project Action Items");

    addColumn(0, tr("Item ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Item"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(2, tr("Type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique); //item_type, 2);
    addColumn(3, tr("Item Name"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(4, tr("Identified By"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique); // teamlist, tr("name"), tr("people_id"), false ); // made not required because it broke when action item detail changed project numbers
    addColumn(5, tr("Date Identified"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(6, tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(7, tr("Assigned To"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique); // teamlist, tr("name"), tr("people_id") );
    addColumn(8, tr("Priority"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);// item_priority, 3);
    addColumn(9, tr("Status"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);//, true, item_status, 5);
    addColumn(10, tr("Date Due"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(11, tr("Updated"), DBDate, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(12, tr("Date Resolved"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);

    addColumn(13, tr("Meeting"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);// actionitemprojectnotes, wxT("meeting"), wxT("note_id"));
    addColumn(14, tr("Project"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique); //, true, projects, wxT("project_number"), wxT("project_id") );
    addColumn(15, tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(16, tr("Comments"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(17, tr("Project Status"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(18, tr("Client"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    addRelatedTable("item_tracker_updates", "item_id", "Tracker Updates");

    setOrderBy("item_number");
}


bool ProjectActionItemsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    QSqlRecord qr = emptyrecord();

    // determine the max item_number from the database, then determine the max number from the record cache in case new unsaved records were added
    QString itemnumber_string = global_DBObjects.execute(QString("select max(item_number) from item_tracker where project_id = '%1'").arg(t_fk_value1->toString()));
    int itemnumber_int = itemnumber_string.toInt();

    for ( int i = 0; i < rowCount(QModelIndex()); i++ )
    {
        int testnumber = data(this->index(i, 1)).toInt();
        if (testnumber > itemnumber_int)
            itemnumber_int = testnumber;
    }

    itemnumber_int++;  // set one above the max


    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    qr.setValue("project_id", *t_fk_value1);
    qr.setValue(1, QString("%1").arg(itemnumber_int, 4, 10, QLatin1Char('0')));  // Need to make a counter that looks good for items
    qr.setValue(2, "Tracker");
    qr.setValue(4, global_DBObjects.getProjectManager()); // default identified by to the pm
    qr.setValue(5, curdate); // default to today
    qr.setValue(8, "High"); // set a default priority
    qr.setValue(9, "New"); // set a default status
    qr.setValue(10, QVariant());
    qr.setValue(11, curdate); // date data as entered
    qr.setValue(12, QVariant());
    qr.setValue(15, 0);

    return addRecord(qr);
}

bool ProjectActionItemsModel::setData(const QModelIndex &t_index, const QVariant &t_value, int t_role)
{


    QVariant curdate = QDateTime::currentDateTime().toString("MM/dd/yyyy");

    // set the date the record was updated
    if (t_index.column() != 11)
    {
        QVariant oldvalue = data(t_index, t_role);

        if (oldvalue != t_value) // don't change the update date if nothing changed
        {
            QModelIndex qmi = index(t_index.row(), 11);
            PNSqlQueryModel::setData(qmi, curdate, t_role);
        }
    }

    // if the issue was changed to resolved them change the resolved date
    if (t_index.column() == 9 && t_value.toString() == "Resolved")
    {
        QModelIndex qmi_resolved = index(t_index.row(), 12);
        PNSqlQueryModel::setData(qmi_resolved, curdate, t_role);
    }


    return PNSqlQueryModel::setData(t_index, t_value, t_role);
}


bool ProjectActionItemsModel::openRecord(QModelIndex t_index)
{
    QVariant record_id = data(index(t_index.row(), 0));

    // only select the records another event will be fired to open the window to show them
    global_DBObjects.actionitemsdetailsmodel()->setFilter(14, record_id.toString());
    global_DBObjects.actionitemsdetailsmodel()->refresh();

    return true;
}


//TODO: Pop up a tracker items detail screen
//TODO: Add notes list
//TODO: Pop up the note


