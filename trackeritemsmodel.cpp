#include "trackeritemsmodel.h"
#include "pndatabaseobjects.h"

//#include <QDebug>

TrackerItemsModel::TrackerItemsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("TrackerItemsModel");
    setOrderKey(40);

    setBaseSql("select * from item_tracker_view");

    setTableName("item_tracker", "Project Action Items");

    addColumn(0, tr("Item ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Item"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(2, tr("Type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
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

    addColumn(13, tr("Meeting"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
               "project_notes", "note_id", "(strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title)");
    addColumn(14, tr("Project"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn(15, tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(16, tr("Comments"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(17, tr("Project Status"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(18, tr("Client"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(19, tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    QStringList key1 = {"project_id", "item_number"};

    addUniqueKeys(key1, "Item");

    addRelatedTable("item_tracker_updates", "item_id", "item_id", "Tracker Updates", DBExportable);

    setOrderBy("item_number");
}

QVariant TrackerItemsModel::getNextItemNumber(const QVariant& t_project_id)
{
    // determine the max item_number from the database, then determine the max number from the record cache in case new unsaved records were added
    QString itemnumber_string = global_DBObjects.execute(QString("select max(CAST(item_number as integer)) from item_tracker where project_id = '%1'").arg(t_project_id.toString()));
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

bool TrackerItemsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    //qDebug() << "Adding new tracker item with fk1: " << t_fk_value1->toString() << " and fk2: " << t_fk_value2;

    QSqlRecord qr = emptyrecord();
    QVariant next_item_number = getNextItemNumber(*t_fk_value1);
    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    //qDebug() << "Using project manager id: " << global_DBObjects.getProjectManager();

    qr.setValue("project_id", *t_fk_value1);
    qr.setValue(1, next_item_number);  // Need to make a counter that looks good for items
    qr.setValue(2, "Tracker");
    qr.setValue(4, global_DBObjects.getProjectManager()); // default identified by to the pm
    qr.setValue(5, curdate); // date identified
    qr.setValue(8, "High"); // set a default priority
    qr.setValue(9, "New"); // set a default status
    qr.setValue(10, QVariant());
    qr.setValue(11, curdate); // date data as updated
    qr.setValue(12, QVariant()); // date resolved
    qr.setValue(15, 0);

    return addRecord(qr);
}

bool TrackerItemsModel::setData(const QModelIndex &t_index, const QVariant &t_value, int t_role)
{
    if ( PNSqlQueryModel::setData(t_index, t_value, t_role) )
    {
        QVariant curdate = QDateTime::currentDateTime().toString("MM/dd/yyyy");

        // if the issue was changed to resolved them change the resolved date
        if (t_index.column() == 9 && t_value.toString() == "Resolved")
        {
            QModelIndex qmi_resolved = index(t_index.row(), 12);
            PNSqlQueryModel::setData(qmi_resolved, curdate, t_role);
        }

        // set the date the record was updated
        if (t_index.column() != 11)
        {
            QVariant oldvalue = data(t_index, t_role);
            QModelIndex qmi = index(t_index.row(), 11);
            PNSqlQueryModel::setData(qmi, curdate, t_role);
        }

        return true;
    }

    return false;
}

QVariant TrackerItemsModel::data(const QModelIndex &t_index, int t_role) const
{
    if (t_role == Qt::ForegroundRole)
    {
        if (t_index.column() == 8) // statu
        {
            QVariant value = data(t_index);

            if (value == "High")
            {
                 return QVariant(QCOLOR_RED);
            }
            else if (value == "Medium")
            {
                return QVariant(QCOLOR_YELLOW);
            }
        }
        else if (t_index.column() == 10) // due date
        {
            QVariant value = data(t_index);

            QDateTime datecol = parseDateTime(value.toString());

            qint64 dif = datecol.daysTo(QDateTime::currentDateTime());

            if (dif == 0)
            {
                return QVariant(QCOLOR_YELLOW);
            }
            else if (dif > 0)
            {
                return QVariant(QCOLOR_RED);
            }
        }
    }

    return PNSqlQueryModel::data(t_index, t_role);
}

bool TrackerItemsModel::openRecord(QModelIndex t_index)
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

bool TrackerItemsModel::copyRecord(QModelIndex t_index)
{
    if (PNSqlQueryModel::copyRecord(t_index))
    {
        QVariant project_id = data(index(t_index.row(), 14));
        QVariant next_item_number = getNextItemNumber(project_id);

        int count = rowCount(QModelIndex()) - 1;

        setCacheData(index(count, 1), next_item_number);

        return true;
    }

    return false;
}
