// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "notesactionitemsmodel.h"
#include "pnsettings.h"
#include "pndatabaseobjects.h"

NotesActionItemsModel::NotesActionItemsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("NotesActionItemsModel");

    setBaseSql("SELECT item_id, item_number, item_type, item_name, identified_by, date_identified, description, assigned_to, priority, status, date_due, last_update, date_resolved, note_id, project_id, internal_item FROM item_tracker");

    setTableName("item_tracker", "Notes Action Items");

    addColumn(0, tr("Item ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    addColumn(1, tr("Item"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(2, tr("Type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &PNDatabaseObjects::item_type);
    addColumn(3, tr("Item Name"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(4, tr("Identified By"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name");
    addColumn(5, tr("Date Identified"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(6, tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(7, tr("Assigned To"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name");
    addColumn(8, tr("Priority"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &PNDatabaseObjects::item_priority);
    addColumn(9, tr("Status"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &PNDatabaseObjects::item_status);
    addColumn(10, tr("Date Due"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(11, tr("Updated"), DBDate, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(12, tr("Date Resolved"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(13, tr("Note"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "project_notes", "note_id", "(strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title)");
    addColumn(14, tr("Project ID"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn(15, tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);

    QStringList key1 = {"project_id", "item_number"};

    addUniqueKeys(key1, "Item");

    QStringList key2 = {"project_id", "item_name"};

    addUniqueKeys(key2, "Item Name");

    addRelatedTable("item_tracker_updates", "item_id", "item_id", "Tracker Updates", DBExportable);

    setOrderBy("item_number");
}

bool NotesActionItemsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    QSqlRecord qr = emptyrecord();

    // determine the max item_number from the database, then determine the max number from the record cache in case new unsaved records were added
    QString itemnumber_string = global_DBObjects.execute(QString("select max(item_number) from item_tracker where project_id = '%1'").arg(t_fk_value2->toString()));
    int itemnumber_int = itemnumber_string.toInt();

    for ( int i = 0; i < rowCount(QModelIndex()); i++ )
    {
        int testnumber = data(this->index(i, 1)).toInt();
        if (testnumber > itemnumber_int)
            itemnumber_int = testnumber;
    }

    itemnumber_int++;  // set one above the max

    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    qr.setValue(13, *t_fk_value1); // note id
    qr.setValue(14, *t_fk_value2); // project id

    qr.setValue(1, QString("%1").arg(itemnumber_int, 4, 10, QLatin1Char('0')));  // Need to make a counter that looks good for items
    qr.setValue(2, "Action");
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

bool NotesActionItemsModel::openRecord(QModelIndex t_index)
{
    QVariant record_id = data(index(t_index.row(), 0));
    QVariant project_id = data(index(t_index.row(), 14));

    // only select the records another event will be fired to open the window to show them
    global_DBObjects.actionitemsdetailsmodel()->setFilter(0, record_id.toString());
    global_DBObjects.actionitemsdetailsmodel()->refresh();

    global_DBObjects.actionitemsdetailsmeetingsmodel()->setFilter(1, project_id.toString());
    global_DBObjects.actionitemsdetailsmeetingsmodel()->refresh();

    global_DBObjects.trackeritemscommentsmodel()->setFilter(1, record_id.toString());
    global_DBObjects.trackeritemscommentsmodel()->refresh();

    return true;
}

