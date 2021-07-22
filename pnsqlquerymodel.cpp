#include "pnsqlquerymodel.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMessageBox>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QUuid>
#include <QRegularExpressionMatch>
#include <QHashIterator>

PNSqlQueryModel::PNSqlQueryModel(QObject *parent) : QAbstractTableModel(parent)
{

}

int PNSqlQueryModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_SqlQuery.record().count();
    else
        return 0;
}

Qt::ItemFlags PNSqlQueryModel::flags(
        const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    if (m_ColumnIsEditable[index.column()])
        flags |= Qt::ItemIsEditable;

    return flags;
}

bool PNSqlQueryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
            //return QAbstractTableModel::setData(index, value, role);

        // nmake sure column is editable
        // exit if no update table defined
        if (!m_ColumnIsEditable[index.column()] || m_tablename.isEmpty())
            return false;

        if (m_ColumnRequired[index.column()] || index.column() == 0)
        {
            if (value.isNull() || value == "")
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update record"),
                   m_headers[index.column()][Qt::EditRole].toString() + QObject::tr(" is a required field."), QMessageBox::Ok);

                return true;
            }
        }

        if (m_ColumnIsUnique[index.column()])
        {
            if (!isUniqueValue(value, index))
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update record"),
                   m_headers[index.column()][Qt::EditRole].toString() + QObject::tr(" must be a unique value."), QMessageBox::Ok);

                ReloadRecord(index);

                return true;
            }
        }


        // validate the data
        if (m_LookupValues[index.column()] != NULL)
        {
            if (m_LookupValues[index.column()]->indexOf(value.toString()) == -1)
            {
                QMessageBox::warning(nullptr, QObject::tr("Invalid Field Value"),
                                     "\"" + value.toString() + "\" is not a valid value for " + m_headers[index.column()][Qt::EditRole].toString() + ".", QMessageBox::Ok);

                return true;
            }
        }

        // reformat the value to be stored in the database
        QVariant cleanvalue = value;

        SQLEscape(cleanvalue, m_ColumnType[index.column()]);

        // the record id is always column 0
        //QModelIndex primaryKeyIndex = m_cache[index.row()].value(0); // QAbstractTableModel::index(index.row(), 0);
        QString keycolumnname = m_cache[index.row()].fieldName(0);

        QString columnname = m_cache[index.row()].fieldName(index.column());
        QVariant keyvalue = m_cache[index.row()].value(0);
        QVariant oldvalue = m_cache[index.row()].value(index.column());

        QSqlQuery update;
        update.prepare("update " + m_tablename + " set " + columnname + " = ? where " + keycolumnname + " = ? and (" + columnname + " = ? or " + columnname + " is NULL)");
        update.addBindValue(cleanvalue);
        update.addBindValue(keyvalue);
        update.addBindValue(oldvalue);

        if(update.exec())
        {
            if (update.numRowsAffected() == 0)
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update value"),
                   QObject::tr("Field was already updated by another process."), QMessageBox::Ok);

                ReloadRecord(index);

                return true;
            }
            else
            {
                m_cache[index.row()].setValue(index.column(), cleanvalue);

                return true;
            }
        }
        else
        {
            QMessageBox::critical(nullptr, QObject::tr("Cannot update value"),
               update.lastError().text() + "\n" + update.lastQuery(), QMessageBox::Ok);

            return false;
        }
    }

    return true;
}

void PNSqlQueryModel::Refresh()
{
    //beginResetModel();
    clear();

    m_SqlQuery = QSqlQuery(BaseSQL());

    // add a blank row for drop downs
    if (m_ShowBlank)
    {
        m_cache.append(emptyrecord());
    }

    while (m_SqlQuery.next())
    {
        m_cache.append(m_SqlQuery.record());
    }
    //endResetModel();
}

QVariant PNSqlQueryModel::data(const QModelIndex &index, int role) const
{
    QVariant retval;

    if (m_cache.size() > index.row() && (role == Qt::DisplayRole || role == Qt::EditRole))
    {
        retval = m_cache[index.row()].field(index.column()).value();
        ReformatValue(retval, m_ColumnType[index.column()]);
    }

    return retval;
}

void PNSqlQueryModel::clear()
{
    m_cache.clear();

    qDebug() << "PNSsqlQueryModel cache clearned.";
}

QDateTime PNSqlQueryModel::ParseDateTime(QString entrydate)
{
    QStringList elements = entrydate.split(QRegularExpression("[-/.: ]"), Qt::SkipEmptyParts);
    QString Mon, Day,Year, Hours, Min, Seconds, Mil;
    int add12hours = 0;
    int addcurrentyear = 0;

    if (!elements.isEmpty())
        Mon = elements.takeFirst();

    if (!elements.isEmpty())
        Day = elements.takeFirst();

    if (!elements.isEmpty())
        Year = elements.takeFirst();

    if (!elements.isEmpty())
        Hours = elements.takeFirst();

    if (!elements.isEmpty())
        Min = elements.takeFirst();

    if (!elements.isEmpty())
        Seconds = elements.takeFirst();

    if (!elements.isEmpty())
        Mil = elements.takeFirst();

    if (Hours.contains("p", Qt::CaseInsensitive))
        add12hours = 12;

    if (Year.length() <= 2)
    {
        addcurrentyear = trunc(QDate::currentDate().year() / 100) * 100;
    }

    QDate qd(Year.toInt()+addcurrentyear, Mon.toInt(),Day.toInt());
    QTime qt(Hours.toInt()+add12hours,Min.toInt(),Seconds.toInt(),Mil.toInt());

    return QDateTime(qd,qt);
}

void PNSqlQueryModel::SQLEscape(QVariant& ColumnValue, DBColumnType ColumnType)
{
    // don't store blank values
    if ( ColumnValue.isNull() )
    {
        return;
    }

    // make blank entries null
    if ( ColumnValue.toString().isEmpty())
    {
        ColumnValue.clear();
        return;
    }

    switch (ColumnType) {
        case DB_DATE:
        {
            //QDateTime datecol = QDateTime::fromString(ColumnValue.toString(), "MM/dd/yyyy");
            QDateTime datecol = ParseDateTime(ColumnValue.toString());

            if ( datecol.isValid() )
            {
                /*
                if (datecol.date().year() < 1000)
                {
                    qint32 adjustment = QDate::currentDate().year();
                    adjustment = trunc((float)adjustment / 100.0) * 100;

                    datecol = datecol.addYears(adjustment);
                }
*/

                ColumnValue = datecol.toSecsSinceEpoch();
            }
            else
                ColumnValue.clear();

            break;
        }
        case DB_DATETIME:
        {
            //QDateTime datecol = QDateTime::fromString(ColumnValue.toString(),"MM/dd/yyyy hh:mm:ss");
            QDateTime datecol = ParseDateTime(ColumnValue.toString());
            if ( datecol.isValid() )
            {
                if (datecol.date().year() < 1000)
                {
                    qint32 adjustment = QDate::currentDate().year();
                    adjustment = trunc((float)adjustment / 100.0) * 100;

                    datecol = datecol.addYears(adjustment);
                }

                ColumnValue = datecol.toSecsSinceEpoch();
            }
            else
                ColumnValue.clear();

            break;
        }
        case DB_PERCENT:
        case DB_REAL:
        case DB_INTEGER:
        case DB_BOOL:
        case DB_USD:
        {
            ColumnValue.setValue(ColumnValue.toString().toLower());
            if (ColumnValue == "true")
                ColumnValue = "1";
            if (ColumnValue == "false")
                ColumnValue = "0";

            ColumnValue.setValue(ColumnValue.toString().replace("$",""));
            ColumnValue.setValue(ColumnValue.toString().replace("%%",""));
            ColumnValue.setValue(ColumnValue.toString().replace(",",""));

            break;
        }
        case DB_STRING:
        default:
        {
            break;
        }
    }
}

QVariant PNSqlQueryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        QVariant val = m_headers.value(section).value(role);

        if (role == Qt::DisplayRole && !val.isValid())
            val = m_headers.value(section).value(Qt::EditRole);

        if (val.isValid())
            return val;

        if (role == Qt::DisplayRole && m_SqlQuery.record().count() > section)
            return m_SqlQuery.record().fieldName(section);
    }


    return QAbstractTableModel::headerData(section, orientation, role);
}

bool PNSqlQueryModel::setHeaderData(int section, Qt::Orientation orientation,
                                   const QVariant &value, int role)
{
    if (orientation != Qt::Horizontal || section < 0 ) //|| columnCount() <= section)
        return false;

    if (m_headers.size() <= section)
        m_headers.resize(qMax(section + 1, 16));

    m_headers[section][role] = value;

    emit headerDataChanged(orientation, section, section);

    return true;
}

void PNSqlQueryModel::ReformatValue(QVariant& ColumnValue, DBColumnType ColumnType) const
{
    // don't reformat empty values
    if (ColumnValue.isNull())
        return;

    switch (ColumnType) {
        case DB_STRING:
        {
            // leave strings alone
            break;
        }
        case DB_DATE:
        {
            QDateTime datecol;
            datecol.setSecsSinceEpoch(ColumnValue.toLongLong());

            if ( datecol.isValid() )
            {
                ColumnValue = datecol.toString("MM/dd/yyyy");
            }
            else
                ColumnValue.clear();

            break;
        }
        case DB_DATETIME:
        {
            QDateTime datecol;
            datecol.setSecsSinceEpoch(ColumnValue.toLongLong());

            if ( datecol.isValid() )
            {
                ColumnValue = datecol.toString("MM/dd/yyyy hh:mm:ss");
            }
            else
                ColumnValue.clear();

            break;
        }
        case DB_REAL:
        {
            ColumnValue.setValue(ColumnValue.toString().replace("$",""));
            ColumnValue.setValue(ColumnValue.toString().replace("%%",""));
            ColumnValue.setValue(ColumnValue.toString().replace(",",""));
            break;
        }
        case DB_INTEGER:
        case DB_BOOL:
        {
            ColumnValue.setValue(ColumnValue.toString().replace("$",""));
            ColumnValue.setValue(ColumnValue.toString().replace("%%",""));
            ColumnValue.setValue(ColumnValue.toString().replace(",",""));
            break;
        }
        case DB_USD:
        {
            ColumnValue.setValue(ColumnValue.toString().replace("$",""));
            ColumnValue.setValue(ColumnValue.toString().replace("%%",""));
            ColumnValue.setValue(ColumnValue.toString().replace(",",""));

            QLocale lc;
            ColumnValue = lc.toCurrencyString(ColumnValue.toDouble());

            break;
        }
        case DB_PERCENT:
        {
            ColumnValue.setValue(ColumnValue.toString().replace("$",""));
            ColumnValue.setValue(ColumnValue.toString().replace("%%",""));
            ColumnValue.setValue(ColumnValue.toString().replace(",",""));

            ColumnValue = QString::asprintf("%.2f%%",ColumnValue.toDouble());
            break;
        }
        default:
            break;
    }
}

void PNSqlQueryModel::AddColumn(int ColumnNumber, const QString& DisplayName, DBColumnType Type, bool Searchable, bool Required, bool Editable, bool Unique)
{
    setHeaderData(ColumnNumber, Qt::Horizontal, DisplayName);

    m_ColumnType[ColumnNumber] = Type;
    m_ColumnRequired[ColumnNumber] = Required;
    m_ColumnSearchable[ColumnNumber] = Searchable;
    m_ColumnIsEditable[ColumnNumber] = Editable;
    m_LookupValues[ColumnNumber] = NULL;
    m_ColumnIsUnique[ColumnNumber] = Unique;

    m_IsFiltered[ColumnNumber] = false;
    m_FilterValue[ColumnNumber] = QString();

    m_IsUserFiltered[ColumnNumber] = false;
    m_UserFilterValues[ColumnNumber] = QStringList();
    m_UserSearchString[ColumnNumber] = QString();

    m_IsUserRangeFiltered[ColumnNumber] = false;
    m_RangeSearchStart[ColumnNumber] = QString();
    m_RangeSearchEnd[ColumnNumber] = QString();

    m_LookupView[ColumnNumber] = NULL;
    m_LookupValue[ColumnNumber] = -1;
    m_LookupFK[ColumnNumber] = -1;
}

void PNSqlQueryModel::AssociateLookupValues(int ColumnNumber, QStringList* LookupValues)
{
    m_LookupValues[ColumnNumber] = LookupValues;
}

int PNSqlQueryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_cache.size();
}

bool PNSqlQueryModel::AddRecord(QSqlRecord& newrecord)
{
    int i = 0;
    // the record id is always column 0
    newrecord.setValue(0, QUuid::createUuid().toString());

    for (i = 0; i < m_SqlQuery.record().count(); i++)
    {
        if (m_ColumnRequired[i])
        {
            if (newrecord.field(i).value().isNull() || newrecord.field(i).value() == "")
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot insert record"),
                   m_headers[i][Qt::EditRole].toString() + QObject::tr(" is a required field."), QMessageBox::Ok);

                return false;
            }
        }
    }

    for (i = 0; i < m_SqlQuery.record().count(); i++)
    {

        if (m_ColumnIsUnique[i])
        {
            QModelIndex qmi = index(0, i);
            if (!isUniqueValue(newrecord.field(i).value(), qmi))
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update record"),
                   m_headers[i][Qt::EditRole].toString() + QObject::tr(" must be a unique value."), QMessageBox::Ok);

                return true;
            }
        }
    }


    QString fields;
    QString values;

    for (i = 0; i < m_SqlQuery.record().count(); i++)
    {
        if (m_ColumnIsEditable[i] || i == 0)
        {
            if (!fields.isEmpty())
                fields += ", ";

            if (!values.isEmpty())
                values += ", ";

            fields += m_SqlQuery.record().fieldName(i);

            values += " ? ";
        }
    }

    QSqlQuery insert;
    insert.prepare("insert into " + m_tablename + " ( " + fields + " ) values ( " + values + " )");

    for (i = 0; i < m_SqlQuery.record().count(); i++)
    {
        if (m_ColumnIsEditable[i] || i == 0)
            insert.bindValue(i, newrecord.field(i).value());
    }

    if(insert.exec())
    {
        QModelIndex qmi = QModelIndex();
        int row = rowCount((qmi));

        beginInsertRows(qmi, row, row);
        m_cache.append(newrecord);

        endInsertRows();

        return true;
    }

    QMessageBox::critical(nullptr, QObject::tr("Cannot insert record"),
       insert.lastError().text() + "\n" + insert.lastQuery(), QMessageBox::Ok);

    return false;
}

bool PNSqlQueryModel::DeleteRecord(QModelIndex index)
{
    if (!DeleteCheck(index))
        return false;

    QSqlQuery delrow("delete from " + m_tablename + " where " + m_SqlQuery.record().fieldName(0) + " = ? ");
    delrow.bindValue(0, m_cache[index.row()].field(0).value());

    if(delrow.exec())
    {
        beginRemoveRows(QModelIndex(), index.row(), index.row());

        m_cache.remove(index.row());

        endRemoveRows();

        QModelIndex qil = createIndex(index.row(), 0);
        QModelIndex qir = createIndex(index.row(), columnCount() - 1);

        emit dataChanged(qil, qir);

        return true;
    }


    QMessageBox::critical(nullptr, QObject::tr("Cannot delete record"),
       delrow.lastError().text() + "\n" + delrow.lastQuery(), QMessageBox::Ok);

    return false;
}

QSqlRecord PNSqlQueryModel::emptyrecord()
{
    QSqlRecord qr = m_SqlQuery.record();
    qr.clearValues();

    return qr;
}

bool PNSqlQueryModel::isUniqueValue(const QVariant &NewValue, const QModelIndex &index)
{
    QString keycolumnname = m_cache[index.row()].fieldName(0);

    QString columnname = m_cache[index.row()].fieldName(index.column());
    QVariant keyvalue = m_cache[index.row()].value(0).toString();

    QSqlQuery select;
    select.prepare("select count(*) from " + m_tablename + " where " + keycolumnname + " <> ? and " + columnname + " = ?");
    select.bindValue(0, keyvalue);
    select.bindValue(1, NewValue);

    select.exec();
    if (select.next())
        if (select.value(0).toInt() > 0)
            return false;

    return true;
}

void PNSqlQueryModel::AddRelatedTable(const QString& TableName, const QString& ColumnName, const QString& Title)
{
    m_RelatedTable.append(TableName);
    m_RelatedColumn.append(ColumnName);
    m_RelationTitle.append(Title);
}

bool PNSqlQueryModel::DeleteCheck(const QModelIndex &index)
{
    // TODO: lookup records that use this row in other tables
    int reference_count = 0;
    QString message;

    for (int i = 0; i < m_RelatedTable.size(); ++i)
    {
        int relatedcount = 0;

        QSqlQuery select;
        select.prepare("select count(*) from " + m_RelatedTable.at(i) + " where " + m_RelatedColumn.at(i) + " = ?");
        select.bindValue(0, m_cache[index.row()].value(0));
        select.exec();

        if (select.next())
            relatedcount = select.value(0).toInt();

        if (relatedcount > 0)
        {
            reference_count += relatedcount;

            message += select.value(0).toString() + " " + m_RelationTitle.at(i) + " record(s)\n";
        }
    }

    if (reference_count > 0)
    {
        message = QObject::tr("The ") + m_DisplayName + QObject::tr(" record is referenced in the following records:\n\n") + message +
                 QObject::tr("\nYou cannot delete the ") + m_DisplayName + QObject::tr(" record until the assocated records are deleted. Would you like to run a search for all related records?");

        if ( QMessageBox::question(nullptr, QObject::tr("Cannot delete record"),
           message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes )
        {
            //TODO: call search function here
        }


        return false;
    }
    else
    {
        if ( QMessageBox::question(nullptr, QObject::tr("Cannot delete record"),
            QObject::tr("Are you sure you want to delete this ") + m_DisplayName + QObject::tr(" record?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes )
            return true;
        else
            return false;
    }
}

const QVariant PNSqlQueryModel::FindValue(QVariant& LookupValue, int SearchColumn, int ReturnColumn)
{
    for ( QVector<QSqlRecord>::Iterator itrow = m_cache.begin(); itrow != m_cache.end(); ++itrow )
    {
        if ( itrow->value(SearchColumn) == LookupValue )
        {
            return itrow->value(ReturnColumn); // key is always at 0
        }
    }

    return QVariant();
}

bool PNSqlQueryModel::ReloadRecord(const QModelIndex& index)
{
    QSqlQuery select(BaseSQL() + " where " + m_SqlQuery.record().fieldName(0) + " = ? ");
    select.bindValue(0, m_cache[index.row()].field(0).value());

    if (select.exec())
    {
        if (select.next())
        {
            m_cache[index.row()] = select.record();

            emit dataChanged(index.model()->index(index.row(), 0), index.model()->index(index.row(), select.record().count()));
            return true;
        }
    }

    return false;
}

QString PNSqlQueryModel::ConstructWhereClause()
{
    QString valuelist;
    QVariant ColumnValue;

    // build the column list for the where statement
    QHashIterator<int, bool> hashit(m_IsFiltered);
    while (hashit.hasNext())
    {
        hashit.next();

        if (hashit.value()) // has a filter
        {
            if (!valuelist.isEmpty())
                valuelist += tr(" AND ");

            ColumnValue = m_FilterValue[hashit.key()];

            SQLEscape(ColumnValue, m_ColumnType[hashit.key()]);

            if (m_ColumnType[hashit.key()] == DB_BOOL && ColumnValue == tr("'0'"))
            {
                valuelist += QString(" (%1 = %2").arg(m_SqlQuery.record().fieldName(hashit.key()), ColumnValue.toString() );
                valuelist += QString(" OR %1 IS NULL) ").arg( m_SqlQuery.record().fieldName(hashit.key()) );
            }
            else
                valuelist += QString("%1 = %2").arg( m_SqlQuery.record().fieldName(hashit.key()), ColumnValue.toString() );
        }
    }

    if (m_UserFilterActive)
    {
        bool checkfornull;

        // build the column list for user filter where statement

        QHashIterator<int, QString> hashitsrch(m_UserSearchString);
        while (hashitsrch.hasNext())
        {
            hashitsrch.next();

            checkfornull = false;

            if (!hashitsrch.value().isEmpty())
            {
                if (!valuelist.isEmpty())
                    valuelist += tr(" AND ");

                ColumnValue = tr("%%") + hashitsrch.value() + tr("%%");
                SQLEscape(&ColumnValue, m_ColumnType[hashitsrch.key()]);

                valuelist += QString(" %1 LIKE %2 ").arg(m_SqlQuery.record().fieldName(hashitsrch.key()), ColumnValue);
            }

            if (m_IsUserFiltered[hashitsrch.key()])
            {
                if (!valuelist.isEmpty())
                    valuelist += tr(" AND ");

                QStringList& ColumnValues = m_UserFilterValues[hashitsrch.key()];
                QString instring;

                for (QStringList::iterator itfilter = ColumnValues.begin(); itfilter != ColumnValues.end(); ++itfilter)
                {
                    if (!instring.isEmpty())
                        instring += tr(", ");

                    if (m_LookupView[hashitsrch.key()])
                        ColumnValue = m_LookupView[hashitsrch.key()].FindValue(*itfilter, m_LookupFK[hashitsrch.key()], m_LookupValue[hashitsrch.key()]);
                    else
                        ColumnValue = *itfilter;

                    SQLEscape(&ColumnValue, m_ColumnType[hashitsrch.key()]);

                    instring += ColumnValue.toString();

                    if (m_ColumnType[hashitsrch.key()] == DB_BOOL && ColumnValue == tr("'0'"))
                        checkfornull = true;

                    // the database doesn't store blanks, they are converted to null
                    if (ColumnValue == tr("NULL"))
                        checkfornull = true;
                }

                if (checkfornull)
                {
                    valuelist += QString(" ( %1 IN (%2)").arg(m_SqlQuery.record().fieldName(hashitsrch.key()), instring);
                    valuelist += QString(" OR %s IS NULL) ").arg(m_SqlQuery.record().fieldName(hashitsrch.key()));
                }
                else
                    valuelist += QString(" %1 IN (%2) ").arg(m_SqlQuery.record().fieldName(hashitsrch.key()), instring);
            }

            if (m_IsUserRangeFiltered[hashitsrch.key()])
            {
                QString RangeStart = m_RangeSearchStart[hashitsrch.key()];
                QString RangeEnd = m_RangeSearchEnd[hashitsrch.key()];

                SQLEscape(&RangeStart, m_ColumnType[hashitsrch.key()]);
                SQLEscape(&RangeEnd, m_ColumnType[hashitsrch.key()]);

                if (RangeStart != tr("NULL") && RangeStart != tr("''"))
                {
                    if (!valuelist.isEmpty())
                        valuelist += tr(" AND ");

                    valuelist += QString("%1 >= %2", m_SqlQuery.record().fieldName(hashitsrch.key()), RangeStart);
                }

                if (RangeEnd != tr("NULL") && RangeEnd != tr("''"))
                {
                    if (!valuelist.isEmpty())
                        valuelist += tr(" AND ");

                    valuelist += QString("%1 <= %2").arg(m_SqlQuery.record().fieldName(hashitsrch.key()), RangeEnd);
                }
            }
        }
    }

    if (!valuelist.isEmpty())
        valuelist = QString(" WHERE %1 COLLATE NOCASE ").arg(valuelist);

    return valuelist;
}

void PNSqlQueryModel::SetFilter(int ColumnNumber, const QString& FilterValue)
{
    m_FilterValue[ColumnNumber] = FilterValue;
    m_IsFiltered[ColumnNumber] = true;
}

void PNSqlQueryModel::ClearAllFilters()
{
    QHashIterator<int, bool> hashit(m_IsFiltered);

    while (hashit.hasNext())
    {
        hashit.next();
        ClearFilter(hashit.key());
    }
}

void PNSqlQueryModel::ClearFilter(int ColumnNumber)
{
    m_IsFiltered[ColumnNumber] = false;
    m_FilterValue[ColumnNumber].clear();
}

void PNSqlQueryModel::SetUserFilter(int ColumnNumber, const QStringList& FilterValues)
{
    m_IsFiltered[ColumnNumber] = true;
    m_UserFilterValues[ColumnNumber] = FilterValues;
}

void PNSqlQueryModel::SetUserSearchString(int ColumnNumber, const QString& SearchValue)
{
    m_UserSearchString[ColumnNumber] = SearchValue;
}

void PNSqlQueryModel::SetUserSearchRange(int ColumnNumber, const QString& SearchBeginValue, const QString& SearchEndValue )
{
    m_IsUserRangeFiltered[ColumnNumber] = true;
    m_RangeSearchStart[ColumnNumber] = SearchBeginValue;
    m_RangeSearchEnd[ColumnNumber] = SearchEndValue;
}

void PNSqlQueryModel::GetUserSearchRange(int ColumnNumber, QString& SearchBeginValue, QString& SearchEndValue )
{
    SearchBeginValue = m_RangeSearchStart[ColumnNumber];
    SearchEndValue = m_RangeSearchEnd[ColumnNumber];
}

void PNSqlQueryModel::ClearAllUserSearches()
{
    QHashIterator<int, bool> hashit(m_IsFiltered);

    while (hashit.hasNext())
    {
        ClearUserFilter(hashit.key());
        ClearUserSearchString(hashit.key());
        ClearUserSearchRange(hashit.key());
    }
}

void PNSqlQueryModel::ClearUserFilter(int ColumnNumber)
{
    m_IsUserFiltered[ColumnNumber] = false;
    m_UserFilterValues[ColumnNumber].clear();
}

void PNSqlQueryModel::ClearUserSearchString(int ColumnNumber)
{
    m_UserSearchString[ColumnNumber].clear();
}

void PNSqlQueryModel::ClearUserSearchRange(int ColumnNumber)
{
    m_IsUserRangeFiltered[ColumnNumber] = false;
    m_RangeSearchStart[ColumnNumber].clear();
    m_RangeSearchEnd[ColumnNumber].clear();
}

bool PNSqlQueryModel::HasUserFilters(int ColumnNumber)
{
    if (m_IsUserRangeFiltered[ColumnNumber] || m_IsUserFiltered[ColumnNumber] || !m_UserSearchString[ColumnNumber].isEmpty()  )
        return true;
    else
        return false;
}

bool PNSqlQueryModel::HasUserFilters()
{
    QHashIterator<int, bool> hashit(m_IsFiltered);

    while (hashit.hasNext())
    {
        if ( HasUserFilters(hashit.key()) )
            return true;
    }
}

void PNSqlQueryModel::ActivateUserFilter(QString FilterName)
{
  // TODO: Continue from here
}

void DeactivateUserFilter(QString FilterName);
void LoadLastUserFilterState(QString FilterName);

void SaveUserFilter( QString FilterName);
void LoadUserFilter( QString FilterName);

// TODO: Setup filter
// TODO: Setup global filter
// TODO: Setup refresh signalling between models

