#include "trackeritemsmodel.h"
#include "databaseobjects.h"

#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


TrackerItemsModel::TrackerItemsModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("TrackerItemsModel");
    setOrderKey(40);

    setBaseSql("select * from item_tracker_view");
    setDeletedFilterInView(true);  // view filters deleted rows internally

    setTableName("item_tracker", "Project Action Items");

    addColumn("id", tr("Item ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("item_number", tr("Item"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn("item_type", tr("Type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
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

    addColumn("note_id", tr("Meeting"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
               "project_notes", "id", "(strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title)");
    addColumn("project_id", tr("Project"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "projects", "id", "project_number");
    addColumn("internal_item", tr("Internal"), DBBool, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("comments", tr("Comments"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_status", tr("Project Status"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("client_id", tr("Client"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_id_name", tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    QStringList key1 = {"project_id", "item_number"};

    addUniqueKeys(key1, "Item");

    addRelatedTable("item_tracker_updates", "item_id", "id", "Tracker Updates", DBExportable);

    setOrderBy("item_number");
}

QVariant TrackerItemsModel::getNextItemNumber(const QVariant& projectId)
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

const QModelIndex TrackerItemsModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue2);

    //qDebug() << "Adding new tracker item with fk1: " << fkValue1->toString() << " and fk2: " << fkValue2;

    QVector<QVariant> qr = emptyrecord();
    QVariant next_item_number = getNextItemNumber(*fkValue1);
    QVariant curdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    //qDebug() << "Using project manager id: " << m_dbo->getProjectManager();

    qr[getColumnNumber("project_id")] = *fkValue1;
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

bool TrackerItemsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ( SqlQueryModel::setData(index, value, role) )
    {
        QVariant curdate = QDateTime::currentDateTime().toString("MM/dd/yyyy");

        // if the issue was changed to resolved them change the resolved date
        if (index.column() == 9 && value.toString() == "Resolved")
        {
            QModelIndex qmi_resolved = this->index(index.row(), 12);
            SqlQueryModel::setData(qmi_resolved, curdate, role);
        }

        // set the date the record was updated
        if (index.column() != 11)
        {
            QModelIndex qmi = this->index(index.row(), 11);
            SqlQueryModel::setData(qmi, curdate, role);
        }

        return true;
    }

    return false;
}

QVariant TrackerItemsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ForegroundRole)
    {
        if (index.column() == 8) // priority
        {
            QString value = data(index).toString();

            if (value.compare("High") == 0)
            {
                 return QVariant(QCOLOR_RED);
            }
            else if (value.compare("Medium") == 0)
            {
                return QVariant(QCOLOR_YELLOW);
            }
        }
        else if (index.column() == 10) // due date
        {
            QVariant value = data(index);

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

    return SqlQueryModel::data(index, role);
}

void TrackerItemsModel::prepareCopiedRecord(QVector<QVariant>& newrecord, const QModelIndex& sourceIndex)
{
    QVariant project_id = data(this->index(sourceIndex.row(), 14));
    newrecord[3] = QString("Copy of %1").arg(newrecord[3].toString());
    newrecord[1] = getNextItemNumber(project_id);
}
