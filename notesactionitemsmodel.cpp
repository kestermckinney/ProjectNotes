// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "notesactionitemsmodel.h"
#include "databaseobjects.h"

NotesActionItemsModel::NotesActionItemsModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("NotesActionItemsModel");

    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
    setBaseSql("SELECT id, item_number, item_type, item_name, identified_by, date_identified, description, assigned_to, priority, status, date_due, last_update, date_resolved, note_id, project_id, internal_item FROM item_tracker");

    setTableName("item_tracker", "Notes Action Items");

    addColumn("id", tr("Item ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    addColumn("item_number", tr("Item"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn("item_type", tr("Type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &DatabaseObjects::item_type);
    addColumn("item_name", tr("Item Name"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("identified_by", tr("Identified By"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "people", "id", "name");
    addColumn("date_identified", tr("Date Identified"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("description", tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("assigned_to", tr("Assigned To"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "people", "id", "name");
    addColumn("priority", tr("Priority"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &DatabaseObjects::item_priority);
    addColumn("status", tr("Status"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &DatabaseObjects::item_status);
    addColumn("date_due", tr("Date Due"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("last_update", tr("Updated"), DBDate, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn("date_resolved", tr("Date Resolved"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("note_id", tr("Note"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "project_notes", "id", "(strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title)");
    addColumn("project_id", tr("Project ID"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "projects", "id", "project_number");
    addColumn("internal_item", tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);

    QStringList key1 = {"project_id", "item_number"};

    addUniqueKeys(key1, "Item");

    QStringList key2 = {"project_id", "item_name"};

    addUniqueKeys(key2, "Item Name");

    addRelatedTable("item_tracker_updates", "item_id", "id", "Tracker Updates", DBExportable);

    setOrderBy("item_number");
}

const QModelIndex NotesActionItemsModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    QVector<QVariant> qr = emptyrecord();

    // determine the max item_number from the database, then determine the max number from the record cache in case new unsaved records were added
    QString itemnumber_string = getDBOs()->execute(QString("select max(CAST(item_number as integer)) from item_tracker where project_id = '%1' and deleted = 0").arg(fkValue2->toString()));
    int itemnumber_int = itemnumber_string.toInt();

    for ( int i = 0; i < rowCount(QModelIndex()); i++ )
    {
        int testnumber = data(this->index(i, 1)).toInt();
        if (testnumber > itemnumber_int)
            itemnumber_int = testnumber;
    }

    itemnumber_int++;  // set one above the max

    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    qr[13] = *fkValue1; // note id
    qr[14] = *fkValue2; // project id

    qr[1] = QString("%1").arg(itemnumber_int, 4, 10, QLatin1Char('0'));  // Need to make a counter that looks good for items
    qr[2] = "Action";
    qr[4] = getDBOs()->getProjectManager(); // default identified by to the pm
    qr[5] = curdate; // default to today
    qr[8] = "High"; // set a default priority
    qr[9] = "New"; // set a default status
    qr[11] = curdate; // date data as entered
    qr[15] = 0;

    return addRecord(qr);
}

bool NotesActionItemsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ( SqlQueryModel::setData(index, value, role) )
    {
        // if assigned_to is set and status is New, advance status to Assigned
        if (index.column() == 7 && !value.toString().isEmpty())
        {
            QModelIndex qmi_status = this->index(index.row(), 9);
            if (data(qmi_status).toString() == "New")
                SqlQueryModel::setData(qmi_status, "Assigned", role);
        }

        return true;
    }

    return false;
}

void NotesActionItemsModel::prepareCopiedRecord(QVector<QVariant>& newrecord, const QModelIndex& sourceIndex)
{
    QVariant project_id = data(this->index(sourceIndex.row(), 14));
    newrecord[3] = QString("Copy of %1").arg(newrecord[3].toString());
    newrecord[1] = getNextItemNumber(project_id);
}

QVariant NotesActionItemsModel::getNextItemNumber(const QVariant& projectId)
{
    // determine the max item_number from the database, then determine the max number from the record cache in case new unsaved records were added
    QString itemnumber_string = getDBOs()->execute(QString("select max(CAST(item_number as integer)) from item_tracker where project_id = '%1' and deleted = 0").arg(projectId.toString()));
    int itemnumber_int = itemnumber_string.toInt();

    for ( int i = 0; i < rowCount(QModelIndex()); i++ )
    {
        int testnumber = data(this->index(i, 1)).toInt();
        if (testnumber > itemnumber_int)
            itemnumber_int = testnumber;
    }

    itemnumber_int++;  // set one above the max

    return QVariant(QString("%1").arg(itemnumber_int, 4, 10, QLatin1Char('0')));
}

