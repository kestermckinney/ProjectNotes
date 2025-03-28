#include "trackeritemsmodel.h"
#include "pndatabaseobjects.h"

#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


TrackerItemsModel::TrackerItemsModel(PNDatabaseObjects* t_dbo): PNSqlQueryModel(t_dbo)
{
    setObjectName("TrackerItemsModel");
    setOrderKey(40);

    setBaseSql("select * from item_tracker_view");

    setTableName("item_tracker", "Project Action Items");

    addColumn("item_id", tr("Item ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("item_number", tr("Item"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn("item_type", tr("Type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn("item_name", tr("Item Name"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("identified_by", tr("Identified By"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name");

    addColumn("date_identified", tr("Date Identified"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("description", tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("assigned_to", tr("Assigned To"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name");

    addColumn("priority", tr("Priority"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &PNDatabaseObjects::item_priority);
    addColumn("status", tr("Status"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &PNDatabaseObjects::item_status);
    addColumn("date_due", tr("Date Due"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("last_update", tr("Updated"), DBDate, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn("date_resolved", tr("Date Resolved"), DBDate, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);

    addColumn("note_id", tr("Meeting"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
               "project_notes", "note_id", "(strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title)");
    addColumn("project_id", tr("Project"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn("internal_item", tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("comments", tr("Comments"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("projet_status", tr("Project Status"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("client_id", tr("Client"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_id_name", tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    QStringList key1 = {"project_id", "item_number"};

    addUniqueKeys(key1, "Item");

    addRelatedTable("item_tracker_updates", "item_id", "item_id", "Tracker Updates", DBExportable);

    setOrderBy("item_number");
}

QVariant TrackerItemsModel::getNextItemNumber(const QVariant& t_project_id)
{
    // determine the max item_number from the database, then determine the max number from the record cache in case new unsaved records were added
    QString itemnumber_string = getDBOs()->execute(QString("select max(CAST(item_number as integer)) from item_tracker where project_id = '%1'").arg(t_project_id.toString()));
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

const QModelIndex TrackerItemsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    //qDebug() << "Adding new tracker item with fk1: " << t_fk_value1->toString() << " and fk2: " << t_fk_value2;

    QVector<QVariant> qr = emptyrecord();
    QVariant next_item_number = getNextItemNumber(*t_fk_value1);
    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    //qDebug() << "Using project manager id: " << m_dbo->getProjectManager();

    qr[getColumnNumber("project_id")] = *t_fk_value1;
    qr[1] = next_item_number;  // Need to make a counter that looks good for items
    qr[2] = "Tracker";
    qr[4] = getDBOs()->getProjectManager(); // default identified by to the pm
    qr[5] = curdate; // date identified
    qr[8] = "High"; // set a default priority
    qr[9] = "New"; // set a default status
    qr[11] = curdate; // date data as updated
    qr[15] = 0;

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
        if (t_index.column() == 8) // priority
        {
            QString value = data(t_index).toString();

            if (value.compare("High") == 0)
            {
                 return QVariant(QCOLOR_RED);
            }
            else if (value.compare("Medium") == 0)
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

const QModelIndex TrackerItemsModel::copyRecord(QModelIndex t_index)
{
    QModelIndex qi = PNSqlQueryModel::copyRecord(t_index);

    if(qi.isValid())
    {
        QVariant project_id = data(index(t_index.row(), 14));
        QVariant next_item_number = getNextItemNumber(project_id);

        int count = rowCount(QModelIndex()) - 1;

        setCacheData(index(count, 1), next_item_number);
    }

    return qi;
}
