#include "pnsqlquerymodel.h"
#include "pndatabaseobjects.h"

#include <QString>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMessageBox>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QUuid>
#include <QRegularExpressionMatch>
#include <QHashIterator>
#include <QDomDocument>
#include <QDomNode>


PNSqlQueryModel::PNSqlQueryModel(QObject *t_parent) : QAbstractTableModel(t_parent)
{
}

int PNSqlQueryModel::columnCount(const QModelIndex &t_parent) const
{
    if (!t_parent.isValid())
        return m_sql_query.record().count();
    else
        return 0;
}

Qt::ItemFlags PNSqlQueryModel::flags(
        const QModelIndex &t_index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(t_index);

    if (m_column_is_editable[t_index.column()])
        flags |= Qt::ItemIsEditable;

    return flags;
}

bool PNSqlQueryModel::setData(const QModelIndex &t_index, const QVariant &t_value, int t_role)
{
    if (t_role == Qt::EditRole)
    {
            //return QAbstractTableModel::setData(t_index, t_value, t_role);

        // nmake sure column is edit_table
        // exit if no update t_table defined
        if (!m_column_is_editable[t_index.column()] || m_tablename.isEmpty())
            return false;

        if (m_column_is_required[t_index.column()] || t_index.column() == 0)
        {
            if (t_value.isNull() || t_value == "")
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update record"),
                   m_headers[t_index.column()][Qt::EditRole].toString() + QObject::tr(" is a required field."), QMessageBox::Ok);

                return true;
            }
        }

        if (m_column_is_unique[t_index.column()])
        {
            if (!isUniqueValue(t_value, t_index))
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update record"),
                   m_headers[t_index.column()][Qt::EditRole].toString() + QObject::tr(" must be a unique t_value."), QMessageBox::Ok);

                reloadRecord(t_index);

                return true;
            }
        }


        // validate the data
        if (m_lookup_values[t_index.column()] != nullptr)
        {
            if (m_lookup_values[t_index.column()]->indexOf(t_value.toString()) == -1)
            {
                QMessageBox::warning(nullptr, QObject::tr("Invalid Field Value"),
                                     "\"" + t_value.toString() + "\" is not a valid t_value for " + m_headers[t_index.column()][Qt::EditRole].toString() + ".", QMessageBox::Ok);

                return true;
            }
        }

        // reformat the t_value to be stored in the database
        QVariant cleanvalue = t_value;

        sqlEscape(cleanvalue, m_column_type[t_index.column()]);

        // the record id is always column 0
        //QModelIndex primaryKeyIndex = m_cache[t_index.row()].value(0); // QAbstractTableModel::t_index(t_index.row(), 0);
        QString keycolumnname = m_cache[t_index.row()].fieldName(0);

        QString columnname = m_cache[t_index.row()].fieldName(t_index.column());
        QVariant keyvalue = m_cache[t_index.row()].value(0);
        QVariant oldvalue = m_cache[t_index.row()].value(t_index.column());

        QSqlQuery update;
        update.prepare("update " + m_tablename + " set " + columnname + " = ? where " + keycolumnname + " = ? and (" + columnname + " = ? or " + columnname + " is NULL)");
        update.addBindValue(cleanvalue);
        update.addBindValue(keyvalue);
        update.addBindValue(oldvalue);

        if(update.exec())
        {
            if (update.numRowsAffected() == 0)
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update t_value"),
                   QObject::tr("Field was already updated by another process."), QMessageBox::Ok);

                reloadRecord(t_index);

                return true;
            }
            else
            {
                m_cache[t_index.row()].setValue(t_index.column(), cleanvalue);

                return true;
            }
        }
        else
        {
            QMessageBox::critical(nullptr, QObject::tr("Cannot update t_value"),
               update.lastError().text() + "\n" + update.lastQuery(), QMessageBox::Ok);

            return false;
        }
    }

    return true;
}

void PNSqlQueryModel::refresh()
{
    QString orderby;
    QString fullsql;

    beginResetModel();
    clear();

    m_sql_query = QSqlQuery( BaseSQL() ); // always build query to get the column names for where clause generation

    if (!m_order_by.isEmpty() )
        orderby = " order by " + m_order_by;

    fullsql = QString("%1 %2 %3").arg( BaseSQL(), constructWhereClause(), orderby);

    m_sql_query = QSqlQuery( fullsql );

    qDebug() << "Refreshing: ";
    qDebug() << fullsql;

    // add a blank row for drop downs
    if (m_show_blank)
    {
        m_cache.append(emptyrecord());
    }

    while (m_sql_query.next())
    {
        m_cache.append(m_sql_query.record());
    }
    endResetModel();
}

QVariant PNSqlQueryModel::data(const QModelIndex &t_index, int t_role) const
{
    QVariant retval;

    if (m_cache.size() > t_index.row() && (t_role == Qt::DisplayRole || t_role == Qt::EditRole) && t_index.row() >= 0)
    {
        retval = m_cache[t_index.row()].field(t_index.column()).value();
        reformatValue(retval, m_column_type[t_index.column()]);
    }

    // make a light gray backround when not edit_table
    if (m_cache.size() > t_index.row() && t_role == Qt::BackgroundColorRole && t_index.row() >= 0)
    {
        if (!m_column_is_editable[t_index.column()])
        {
            retval = QVariant(QColor("lightgray"));
        }
    }

    return retval;
}


void PNSqlQueryModel::clear()
{
    m_cache.clear();

    //qDebug() << "PNSsqlQueryModel cache clearned.";
}

QDateTime PNSqlQueryModel::parseDateTime(QString t_entrydate)
{
    QStringList elements = t_entrydate.split(QRegularExpression("[-/.: ]"), Qt::SkipEmptyParts);
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

void PNSqlQueryModel::sqlEscape(QVariant& t_column_value, DBColumnType t_column_type) const
{
    // don't store blank values
    if ( t_column_value.isNull() )
    {
        return;
    }

    // make blank entries nullptr
    if ( t_column_value.toString().isEmpty())
    {
        t_column_value.clear();
        return;
    }

    switch (t_column_type) {
        case DB_DATE:
        {
            QDateTime datecol = parseDateTime(t_column_value.toString());

            if ( datecol.isValid() )
            {
                t_column_value = datecol.toSecsSinceEpoch();
            }
            else
                t_column_value.clear();

            break;
        }
        case DB_DATETIME:
        {
            QDateTime datecol = parseDateTime(t_column_value.toString());
            if ( datecol.isValid() )
            {
                if (datecol.date().year() < 1000)
                {
                    qint32 adjustment = QDate::currentDate().year();
                    adjustment = trunc((float)adjustment / 100.0) * 100;

                    datecol = datecol.addYears(adjustment);
                }

                t_column_value = datecol.toSecsSinceEpoch();
            }
            else
                t_column_value.clear();

            break;
        }
        case DB_PERCENT:
        case DB_REAL:
        case DB_INTEGER:
        case DB_BOOL:
        case DB_USD:
        {
            t_column_value.setValue(t_column_value.toString().toLower());
            if (t_column_value == "true")
                t_column_value = "1";
            if (t_column_value == "false")
                t_column_value = "0";

            t_column_value.setValue(t_column_value.toString().replace("$",""));
            t_column_value.setValue(t_column_value.toString().replace("%",""));
            t_column_value.setValue(t_column_value.toString().replace(",",""));

            break;
        }
        case DB_STRING:
        default:
        {
            break;
        }
    }
}

QVariant PNSqlQueryModel::headerData(int t_section, Qt::Orientation t_orientation, int t_role) const
{
    if (t_orientation == Qt::Horizontal) {
        if ( t_role == Qt::ForegroundRole )
        {
            if (hasUserFilters(t_section))
                return QColor(Qt::darkBlue);
        }
        else if ( t_role == Qt::DisplayRole )
        {
            QVariant val = m_headers.value(t_section).value(t_role);

            if (t_role == Qt::DisplayRole && !val.isValid())
                val = m_headers.value(t_section).value(Qt::EditRole);

            if (val.isValid())
                return val;

            if (t_role == Qt::DisplayRole && m_sql_query.record().count() > t_section)
                return m_sql_query.record().fieldName(t_section);
        }
    }

    return QAbstractTableModel::headerData(t_section, t_orientation, t_role);
}

bool PNSqlQueryModel::setHeaderData(int t_section, Qt::Orientation t_orientation,
                                   const QVariant &t_value, int t_role)
{
    if (t_orientation != Qt::Horizontal || t_section < 0 ) //|| columnCount() <= t_section)
        return false;

    if (m_headers.size() <= t_section)
        m_headers.resize(qMax(t_section + 1, 16));

    m_headers[t_section][t_role] = t_value;

    emit headerDataChanged(t_orientation, t_section, t_section);

    return true;
}

void PNSqlQueryModel::reformatValue(QVariant& t_column_value, DBColumnType t_column_type) const
{
    // don't reformat empty values
    if (t_column_value.isNull())
        return;

    switch (t_column_type) {
        case DB_STRING:
        {
            // leave strings alone
            break;
        }
        case DB_DATE:
        {
            QDateTime datecol;
            datecol.setSecsSinceEpoch(t_column_value.toLongLong());

            if ( datecol.isValid() )
            {
                t_column_value = datecol.toString("MM/dd/yyyy");
            }
            else
                t_column_value.clear();

            break;
        }
        case DB_DATETIME:
        {
            QDateTime datecol;
            datecol.setSecsSinceEpoch(t_column_value.toLongLong());

            if ( datecol.isValid() )
            {
                t_column_value = datecol.toString("MM/dd/yyyy hh:mm:ss");
            }
            else
                t_column_value.clear();

            break;
        }
        case DB_REAL:
        {
            t_column_value.setValue(t_column_value.toString().replace("$",""));
            t_column_value.setValue(t_column_value.toString().replace("%",""));
            t_column_value.setValue(t_column_value.toString().replace(",",""));
            break;
        }
        case DB_INTEGER:
        case DB_BOOL:
        {
            t_column_value.setValue(t_column_value.toString().replace("$",""));
            t_column_value.setValue(t_column_value.toString().replace("%",""));
            t_column_value.setValue(t_column_value.toString().replace(",",""));
            break;
        }
        case DB_USD:
        {
            t_column_value.setValue(t_column_value.toString().replace("$",""));
            t_column_value.setValue(t_column_value.toString().replace("%",""));
            t_column_value.setValue(t_column_value.toString().replace(",",""));

            QLocale lc;
            t_column_value = lc.toCurrencyString(t_column_value.toDouble());

            break;
        }
        case DB_PERCENT:
        {
            t_column_value.setValue(t_column_value.toString().replace("$",""));
            t_column_value.setValue(t_column_value.toString().replace("%",""));
            t_column_value.setValue(t_column_value.toString().replace(",",""));

            t_column_value = QString::asprintf("%.2f%%",t_column_value.toDouble());
            break;
        }
        default:
            break;
    }
}

void PNSqlQueryModel::addColumn(int t_column_number, const QString& t_display_name, DBColumnType t_type, bool t_searchable, bool t_required, bool t_editable, bool t_uniquie)
{
    setHeaderData(t_column_number, Qt::Horizontal, t_display_name);

    m_column_type[t_column_number] = t_type;
    m_column_is_required[t_column_number] = t_required;
    m_column_is_searchable[t_column_number] = t_searchable;
    m_column_is_editable[t_column_number] = t_editable;
    m_lookup_values[t_column_number] = nullptr;
    m_column_is_unique[t_column_number] = t_uniquie;

    m_column_is_filtered[t_column_number] = false;
    m_filter_value[t_column_number] = QString();

    m_is_user_filtered[t_column_number] = false;
    m_user_filter_values[t_column_number] = QVariantList();
    m_user_search_string[t_column_number] = QVariant();

    m_is_user_range_filtered[t_column_number] = false;
    m_range_search_start[t_column_number] = QVariant();
    m_range_search_end[t_column_number] = QVariant();

    m_lookup_view[t_column_number] = nullptr;
    m_lookup_value_column[t_column_number] = -1;
    m_lookup_fk_column[t_column_number] = -1;
}

void PNSqlQueryModel::associateLookupValues(int t_column_number, QStringList* LookupValues)
{
    m_lookup_values[t_column_number] = LookupValues;
}

int PNSqlQueryModel::rowCount(const QModelIndex &t_parent) const
{
    if (t_parent.isValid())
        return 0;

    return m_cache.size();
}

bool PNSqlQueryModel::addRecord(QSqlRecord& t_newrecord)
{
    int i = 0;
    // the record id is always column 0
    t_newrecord.setValue(0, QUuid::createUuid().toString());

    for (i = 0; i < m_sql_query.record().count(); i++)
    {
        if (m_column_is_required[i])
        {
            if (t_newrecord.field(i).value().isNull() || t_newrecord.field(i).value() == "")
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot insert record"),
                   m_headers[i][Qt::EditRole].toString() + QObject::tr(" is a required field."), QMessageBox::Ok);

                return false;
            }
        }
    }

    for (i = 0; i < m_sql_query.record().count(); i++)
    {

        if (m_column_is_unique[i])
        {
            QModelIndex qmi = index(0, i);
            if (!isUniqueValue(t_newrecord.field(i).value(), qmi))
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update record"),
                   m_headers[i][Qt::EditRole].toString() + QObject::tr(" must be a unique t_value."), QMessageBox::Ok);

                return true;
            }
        }
    }


    QString fields;
    QString values;

    for (i = 0; i < m_sql_query.record().count(); i++)
    {
        if (m_column_is_editable[i] || i == 0)
        {
            if (!fields.isEmpty())
                fields += ", ";

            if (!values.isEmpty())
                values += ", ";

            fields += m_sql_query.record().fieldName(i);

            values += " ? ";
        }
    }

    QSqlQuery insert;
    insert.prepare("insert into " + m_tablename + " ( " + fields + " ) values ( " + values + " )");
    qDebug() << "insert into " << m_tablename << " ( " << fields << " ) values ( " << values << " )";

    int bindcount = 0;
    for (i = 0; i < m_sql_query.record().count(); i++)
    {
        if (m_column_is_editable[i] || i == 0)
        {
            insert.bindValue(bindcount, t_newrecord.field(i).value());
            bindcount++;
        }
    }

    if(insert.exec())
    {
        QModelIndex qmi = QModelIndex();
        int row = rowCount((qmi));

        beginInsertRows(qmi, row, row);
        m_cache.append(t_newrecord);

        endInsertRows();

        return true;
    }

    QMessageBox::critical(nullptr, QObject::tr("Cannot insert record"),
       insert.lastError().text() + "\n" + insert.lastQuery(), QMessageBox::Ok);

    return false;
}

bool PNSqlQueryModel::newRecord()
{
    QSqlRecord qr = emptyrecord();
    return addRecord(qr);
}

bool PNSqlQueryModel::deleteRecord(QModelIndex t_index)
{
    if (!deleteCheck(t_index))
        return false;

    QSqlQuery delrow("delete from " + m_tablename + " where " + m_sql_query.record().fieldName(0) + " = ? ");
    delrow.bindValue(0, m_cache[t_index.row()].field(0).value());

    if(delrow.exec())
    {
        beginRemoveRows(QModelIndex(), t_index.row(), t_index.row());

        m_cache.remove(t_index.row());

        endRemoveRows();

        QModelIndex qil = createIndex(t_index.row(), 0);
        QModelIndex qir = createIndex(t_index.row(), columnCount() - 1);

        emit dataChanged(qil, qir);

        return true;
    }


    QMessageBox::critical(nullptr, QObject::tr("Cannot delete record"),
       delrow.lastError().text() + "\n" + delrow.lastQuery(), QMessageBox::Ok);

    return false;
}

QSqlRecord PNSqlQueryModel::emptyrecord()
{
    QSqlRecord qr = m_sql_query.record();
    qr.clearValues();

    return qr;
}

bool PNSqlQueryModel::isUniqueValue(const QVariant &t_new_value, const QModelIndex &t_index)
{
    QString keycolumnname = m_cache[t_index.row()].fieldName(0);

    QString columnname = m_cache[t_index.row()].fieldName(t_index.column());
    QVariant keyvalue = m_cache[t_index.row()].value(0).toString();

    QSqlQuery select;
    select.prepare("select count(*) from " + m_tablename + " where " + keycolumnname + " <> ? and " + columnname + " = ?");
    select.bindValue(0, keyvalue);
    select.bindValue(1, t_new_value);

    select.exec();
    if (select.next())
        if (select.value(0).toInt() > 0)
            return false;

    return true;
}

void PNSqlQueryModel::addRelatedTable(const QString& TableName, const QString& ColumnName, const QString& Title)
{
    m_related_table.append(TableName);
    m_related_column.append(ColumnName);
    m_relation_title.append(Title);
}

bool PNSqlQueryModel::deleteCheck(const QModelIndex &t_index)
{
    int reference_count = 0;
    QString message;

    for (int i = 0; i < m_related_table.size(); ++i)
    {
        int relatedcount = 0;

        QSqlQuery select;
        select.prepare("select count(*) from " + m_related_table.at(i) + " where " + m_related_column.at(i) + " = ?");
        select.bindValue(0, m_cache[t_index.row()].value(0));
        select.exec();

        if (select.next())
            relatedcount = select.value(0).toInt();

        if (relatedcount > 0)
        {
            reference_count += relatedcount;

            message += select.value(0).toString() + " " + m_relation_title.at(i) + " record(s)\n";
        }
    }

    if (reference_count > 0)
    {
        message = QObject::tr("The ") + m_display_name + QObject::tr(" record is referenced in the following records:\n\n") + message +
                 QObject::tr("\nYou cannot delete the ") + m_display_name + QObject::tr(" record until the assocated records are deleted. Would you like to run a search for all related records?");

        if ( QMessageBox::question(nullptr, QObject::tr("Cannot delete record"),
           message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes )
        {
            //TODO: call search function here
        }


        return false;
    }
    else
    {
        if ( QMessageBox::question(nullptr, QObject::tr("Delete record?"),
            QObject::tr("Are you sure you want to delete this ") + m_display_name + QObject::tr(" record?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes )
            return true;
        else
            return false;
    }
}

const QVariant PNSqlQueryModel::findValue(QVariant& t_lookup_value, int t_search_column, int t_return_column)
{
    for ( QVector<QSqlRecord>::Iterator itrow = m_cache.begin(); itrow != m_cache.end(); ++itrow )
    {
        if ( itrow->value(t_search_column) == t_lookup_value )
        {
            return itrow->value(t_return_column); // key is always at 0
        }
    }

    return QVariant();
}

bool PNSqlQueryModel::reloadRecord(const QModelIndex& t_index)
{
    QSqlQuery select(BaseSQL() + " where " + m_sql_query.record().fieldName(0) + " = ? ");
    select.bindValue(0, m_cache[t_index.row()].field(0).value());

    if (select.exec())
    {
        if (select.next())
        {
            m_cache[t_index.row()] = select.record();

            emit dataChanged(t_index.model()->index(t_index.row(), 0), t_index.model()->index(t_index.row(), select.record().count()));
            return true;
        }
    }

    return false;
}

QString PNSqlQueryModel::constructWhereClause(bool t_include_user_filter)
{
    QString valuelist;
    QVariant column_value;

    QHashIterator<int, bool> hashit(m_column_is_filtered);

    // build out the filtered value portion of the where clause
    // filtered values don't consider the value of foreign keys
    // they aren't a filter the user sets in the filter tool
    while (hashit.hasNext())
    {
        hashit.next();

        if (hashit.value()) // has a filter
        {

            column_value = m_filter_value[hashit.key()];

            if (!column_value.isNull())
            {
                if (!valuelist.isEmpty())
                    valuelist += tr(" AND ");

                sqlEscape(column_value, m_column_type[hashit.key()]);

                if ( m_column_type[hashit.key()] != DB_STRING )
                {
                    if (m_column_type[hashit.key()] == DB_BOOL && column_value == tr("0"))
                    {
                        valuelist += QString(" ( %1 = %2").arg(m_sql_query.record().fieldName(hashit.key()), column_value.toString() );
                        valuelist += QString(" OR %1 IS NULL) ").arg( m_sql_query.record().fieldName(hashit.key()) );
                    }
                    else
                        valuelist += QString("%1 = %2").arg( m_sql_query.record().fieldName(hashit.key()), column_value.toString() );
                }
                else
                    valuelist += QString("%1 = '%2'").arg( m_sql_query.record().fieldName(hashit.key()), column_value.toString() );
            }
        }
    }

    // if the user filter is set to active and the function caller wan't to include user filters specified
    // in the filter tool then add them to the where clause
    if (m_user_filter_active && t_include_user_filter)
    {
        // iterate through all the fields and apply the user filters if they have them
        // user search strings will search a foreign key value and not the foreign key id
        QHashIterator<int, bool> hashitsrch(m_is_user_filtered);
        while (hashitsrch.hasNext())
        {
            hashitsrch.next();

            int colnum = hashitsrch.key();

            if ( m_is_user_filtered[colnum] || m_is_user_range_filtered[colnum] ) // if has a user filter then add the various kinds
            {

                // if the column has a sub string search them apply it
                if ( !m_user_search_string[colnum].isNull() )
                {
                    column_value = m_user_search_string[colnum];

                    sqlEscape(column_value, m_column_type[colnum]);

                    if (!valuelist.isEmpty())
                        valuelist += tr(" AND ");

                    // search foreign key value if exists otherwise search for the value in the field
                    if (m_lookup_view[colnum])
                    {
                        int colnumval = m_lookup_value_column[colnum];
                        int colnumkey = m_lookup_fk_column[colnum];

                        QString keycolumn =  m_lookup_view[colnum]->getColumnName(colnumkey);
                        QString valuecolumn = m_lookup_view[colnum]->getColumnName(colnumval);

                        valuelist += QString(" ( %1 IN (select %2 from %3 where %4 LIKE '%%5%') ) ").arg(
                                    m_sql_query.record().fieldName(colnum),
                                    keycolumn,
                                    m_lookup_view[colnum]->tablename(),
                                    valuecolumn,
                                    column_value.toString()
                                );

                        //qDebug() << "Looking for columns in " << m_lookup_view[colnum]->tablename() << ":  Column Number " << colnumkey << " is " << keycolumn<< ", Column Number " << colnumval << " is " << valuecolumn;
                        //qDebug() << "View State: " << m_lookup_view[colnum]->rowCount(m_lookup_view[colnum]->index(0,0)) << "   " << m_lookup_view[colnum]->BaseSQL();
                    }
                    else
                    {
                        valuelist += QString(" %1 LIKE '%%2%' ").arg(m_sql_query.record().fieldName(colnum), column_value.toString());
                    }
                }

                // if column has a list of values to filter add them to the where clause
                QVariantList& ColumnValues = m_user_filter_values[colnum];
                QString instring;
                bool checkfornullptr = false;

                for ( const auto& colval : ColumnValues)
                {
                    if (!instring.isEmpty())
                        instring += tr(", ");

                    column_value = colval;

                    sqlEscape(column_value, m_column_type[colnum]);

                    instring += QString("'%1'").arg(column_value.toString());

                    if (m_column_type[colnum] == DB_BOOL && column_value == tr("'0'"))
                        checkfornullptr = true;

                    // the database doesn't store blanks, they are converted to nullptr
                    if (column_value.isNull())
                        checkfornullptr = true;
                }

                // if we found any items in the list of values add them to the where clause
                if (!instring.isEmpty())
                {
                    if (!valuelist.isEmpty())
                        valuelist += tr(" AND ");

                    // if there is a null value to search for we need to add it to the where clause
                    // since you can put blanks in the IN clause
                    if (checkfornullptr)
                    {
                        valuelist += QString(" ( %1 IN (%2)").arg(m_sql_query.record().fieldName(hashitsrch.key()), instring);
                        valuelist += QString(" OR %1 IS NULL) ").arg(m_sql_query.record().fieldName(hashitsrch.key()));
                    }
                    else
                        valuelist += QString(" %1 IN (%2) ").arg(m_sql_query.record().fieldName(hashitsrch.key()), instring);
                }

                // and any range filters that have been included into the where clause
                if (m_is_user_range_filtered[colnum])
                {
                    QVariant RangeStart(m_range_search_start[colnum]);
                    QVariant RangeEnd(m_range_search_end[colnum]);

                    sqlEscape(RangeStart, m_column_type[colnum]);
                    sqlEscape(RangeEnd, m_column_type[colnum]);


                    // if the range is searching accross a foreign key value then
                    // search and return a list of foreign key ids that apply
                    // search foreign key value if exists otherwise search for the value in the field
                    if (m_lookup_view[colnum])
                    {
                        QString fk_valuelist;

                        int colnumval = m_lookup_value_column[colnum];
                        int colnumkey = m_lookup_fk_column[colnum];

                        QString keycolumn =  m_lookup_view[colnum]->getColumnName(colnumkey);
                        QString valuecolumn = m_lookup_view[colnum]->getColumnName(colnumval);

                        if (!RangeStart.isNull() && RangeStart != tr("''"))
                        {
                            //if (!fk_valuelist.isEmpty())
                            //    fk_valuelist += tr(" AND ");

                            if ( m_column_type[colnum] != DB_STRING )
                            {
                                fk_valuelist += QString("%1 >= %2").arg(valuecolumn, RangeStart.toString());
                            }
                            else
                            {
                                fk_valuelist += QString("%1 >= '%2'").arg(valuecolumn, RangeStart.toString());
                            }
                        }

                        if (!RangeEnd.isNull() && RangeEnd != tr("''"))
                        {
                            if (!valuelist.isEmpty())
                                fk_valuelist += tr(" AND ");


                            if ( m_column_type[colnum] != DB_STRING )
                            {
                                fk_valuelist += QString("%1 <= %2").arg(valuecolumn, RangeEnd.toString());
                            }
                            else
                            {
                                fk_valuelist += QString("%1 <= '%2'").arg(valuecolumn, RangeEnd.toString());
                            }
                        }

                        if (!valuelist.isEmpty())
                            valuelist += tr(" AND ");

                        valuelist += QString(" ( %1 IN (select %2 from %3 where %4) ) ").arg(
                                    m_sql_query.record().fieldName(colnum),
                                    keycolumn,
                                    m_lookup_view[colnum]->tablename(),
                                    fk_valuelist
                                );

                        //qDebug() << "Looking for columns in " << m_lookup_view[colnum]->tablename() << ":  Column Number " << colnumkey << " is " << keycolumn<< ", Column Number " << colnumval << " is " << valuecolumn;
                        //qDebug() << "View State: " << m_lookup_view[colnum]->rowCount(m_lookup_view[colnum]->index(0,0)) << "   " << m_lookup_view[colnum]->BaseSQL();
                    }
                    else
                    {
                        // if the column doesn't have a lookup value in a foreign key
                        // you can do the range search on the value
                        if (!RangeStart.isNull() && RangeStart != tr("''"))
                        {
                            if (!valuelist.isEmpty())
                                valuelist += tr(" AND ");

                            if ( m_column_type[colnum] != DB_STRING )
                            {
                                valuelist += QString("%1 >= %2").arg(m_sql_query.record().fieldName(colnum), RangeStart.toString());
                            }
                            else
                            {
                                valuelist += QString("%1 >= '%2'").arg(m_sql_query.record().fieldName(colnum), RangeStart.toString());
                            }
                        }

                        if (!RangeEnd.isNull() && RangeEnd != tr("''"))
                        {
                            if (!valuelist.isEmpty())
                                valuelist += tr(" AND ");


                            if ( m_column_type[colnum] != DB_STRING )
                            {
                                valuelist += QString("%1 <= %2").arg(m_sql_query.record().fieldName(colnum), RangeEnd.toString());
                            }
                            else
                            {
                                valuelist += QString("%1 <= '%2'").arg(m_sql_query.record().fieldName(colnum), RangeEnd.toString());
                            }
                        }
                    }
                }
            }
        }
    }

    if (!valuelist.isEmpty())
        valuelist = QString(" WHERE %1 COLLATE NOCASE ").arg(valuelist);

    return valuelist;
}

void PNSqlQueryModel::setFilter(int t_column_number, const QString& t_filter_value)
{
    m_filter_value[t_column_number] = t_filter_value;
    m_column_is_filtered[t_column_number] = true;
}

void PNSqlQueryModel::clearAllFilters()
{
    QHashIterator<int, bool> hashit(m_column_is_filtered);

    while (hashit.hasNext())
    {
        hashit.next();
        clearFilter(hashit.key());
    }
}

void PNSqlQueryModel::clearFilter(int t_column_number)
{
    m_column_is_filtered[t_column_number] = false;
    m_filter_value[t_column_number].clear();
}

void PNSqlQueryModel::setUserFilter(int t_column_number, const QVariantList& t_filter_values)
{
    m_is_user_filtered[t_column_number] = true;
    m_user_filter_values[t_column_number] = t_filter_values;
}

void PNSqlQueryModel::setUserSearchString(int t_column_number, const QVariant& t_search_value)
{
    m_is_user_filtered[t_column_number] = true;
    m_user_search_string[t_column_number] = t_search_value;
}

void PNSqlQueryModel::setUserSearchRange(int t_column_number, const QVariant& t_search_begin_value, const QVariant& t_search_end_value )
{
    m_is_user_range_filtered[t_column_number] = true;
    m_range_search_start[t_column_number] = t_search_begin_value;
    m_range_search_end[t_column_number] = t_search_end_value;
}

void PNSqlQueryModel::getUserSearchRange(int t_column_number, QVariant& t_search_begin_value, QVariant& t_search_end_value )
{
    if (m_range_search_start.contains(t_column_number))
    {
        t_search_begin_value = m_range_search_start[t_column_number];
        t_search_end_value = m_range_search_end[t_column_number];
    }
}

void PNSqlQueryModel::clearAllUserSearches()
{
    QHashIterator<int, bool> hashit(m_column_is_filtered);

    while (hashit.hasNext())
    {
        hashit.next();
        clearUserFilter(hashit.key());
        clearUserSearchString(hashit.key());
        clearUserSearchRange(hashit.key());
    }
}

void PNSqlQueryModel::clearUserFilter(int t_column_number)
{
    if (m_is_user_filtered.contains(t_column_number))
    {
        m_is_user_filtered[t_column_number] = false;
        m_user_filter_values[t_column_number].clear();
    }
}

void PNSqlQueryModel::clearUserSearchString(int t_column_number)
{
    if (m_user_search_string.contains(t_column_number))
        m_user_search_string[t_column_number].clear();
}

void PNSqlQueryModel::clearUserSearchRange(int t_column_number)
{
    if (m_is_user_range_filtered.contains(t_column_number))
    {
        m_is_user_range_filtered[t_column_number] = false;
        m_range_search_start[t_column_number].clear();
        m_range_search_end[t_column_number].clear();
    }
}

bool PNSqlQueryModel::hasUserFilters(int t_column_number) const
{
    if (m_is_user_range_filtered[t_column_number] || m_is_user_filtered[t_column_number] )
        return true;
    else
        return false;
}

bool PNSqlQueryModel::hasUserFilters() const
{
    QHashIterator<int, bool> hashit(m_column_is_filtered);

    while (hashit.hasNext())
    {
        hashit.next();
        if ( hasUserFilters(hashit.key()) )
            return true;
    }

    return false;
}

void PNSqlQueryModel::activateUserFilter(QString t_filter_name)
{
    m_user_filter_active = true;
    refresh();

    if (!t_filter_name.isEmpty())
    {
        QString filter_name = t_filter_name;
        filter_name.replace(" ", "_", Qt::CaseSensitive);

        QString parmname;
        QString val = "true";

        parmname = QString("UserFilter:%1:IsActive").arg(filter_name);

        global_DBObjects.saveParameter(parmname, val);
    }
}

void PNSqlQueryModel::deactivateUserFilter(QString t_filter_name)
{
    m_user_filter_active = false;
    refresh();

    if (!t_filter_name.isEmpty())
    {
        QString filter_name = t_filter_name;
        filter_name.replace(" ", "_", Qt::CaseSensitive);

        QString parmname;
        QString val = "false";

        parmname = QString("UserFilter:%1:IsActive").arg(filter_name);

        global_DBObjects.saveParameter(parmname, val);
    }
}

void PNSqlQueryModel::loadLastUserFilterState(QString t_filter_name)
{
    QString filter_name = t_filter_name;
    filter_name.replace(" ", "_", Qt::CaseSensitive);

    QString parmname;
    QString val;

    parmname = QString("UserFilter:%1:IsActive").arg(filter_name);
    parmname.replace(" ", "_", Qt::CaseSensitive);

    val = global_DBObjects.loadParameter(parmname);

    if (val == "true")
        m_user_filter_active = true;
    else
        m_user_filter_active = false;
}

void PNSqlQueryModel::saveUserFilter( QString t_filter_name)
{
    QString filter_name = t_filter_name;
    filter_name.replace(" ", "_", Qt::CaseSensitive);

    QDomDocument doc;
    QDomElement root = doc.createElement(filter_name);
    doc.appendChild(root);

    QDomElement child;
    QDomElement columnvalue;

    root.setAttribute("ObjectType", "UserFilter");

    for (auto it = m_column_type.cbegin(); it != m_column_type.cend(); ++it)
    {
        child = doc.createElement(m_sql_query.record().fieldName(it.key()));
        child.setAttribute("ObjectType", "Field");
        child.setAttribute("RangeSearchStart", m_range_search_start[it.key()].toString());
        child.setAttribute("RangeSearchEnd", m_range_search_end[it.key()].toString());
        child.setAttribute("UserSearchString", m_user_search_string[it.key()].toString());
        child.setAttribute("FieldNumber", it.key());

        root.appendChild(child);

        int i = 0;

        for (auto ait = m_user_filter_values[it.key()].cbegin(); ait != m_user_filter_values[it.key()].cend(); ++ait)
        {
            columnvalue = doc.createElement(QString("value_%1").arg(i));
            i++;

            columnvalue.setAttribute("SearchValue", ait->toString());

            child.appendChild(columnvalue);
        }
    }

    // Write the output to a QString.
    QString xml = doc.toString();

    QString parmname = QString("UserFilter:%1").arg(filter_name);

    global_DBObjects.saveParameter( parmname, xml );
}

void PNSqlQueryModel::loadUserFilter( QString t_filter_name)
{
    QString filter_name = t_filter_name;
    filter_name.replace(" ", "_", Qt::CaseSensitive);

    QString parmname;

    parmname = QString("UserFilter:%1").arg(filter_name);
    parmname.replace(" ", "_", Qt::CaseSensitive);

    QString xml = global_DBObjects.loadParameter(parmname);

    if (xml.isEmpty())
    {
        clearAllUserSearches();
        return;
    }

    QDomDocument doc;
    doc.setContent(xml);

    QDomElement root = doc.documentElement();
    QDomNode child;

    if (!root.isNull())
        child = root.firstChild();

    QDomNode subchild;
    while (!child.isNull())
    {
        int field = child.toElement().attribute("FieldNumber").toInt();

        m_range_search_start[field] = child.toElement().attribute("RangeSearchStart");
        m_range_search_end[field] = child.toElement().attribute("RangeSearchEnd");
        m_user_search_string[field] = child.toElement().attribute("UserSearchString");

        if (!m_range_search_start[field].toString().isEmpty() || !m_range_search_end[field].toString().isEmpty() )
            m_is_user_range_filtered[field] = true;
        else
            m_is_user_range_filtered[field] = false;

        m_user_filter_values[field].clear();

        if (m_user_search_string[field].toString().isEmpty())
            m_is_user_filtered[field] = false;
        else
            m_is_user_filtered[field] = true;

        subchild = child.firstChild();

        while (!subchild.isNull())
        {
            m_user_filter_values[field].append(subchild.toElement().attribute("SearchValue"));
            m_is_user_filtered[field] = true;

            subchild = subchild.nextSibling();
        }

        child = child.nextSibling();
    }
}

void PNSqlQueryModel::setLookup(int Column, PNSqlQueryModel* t_lookup, int t_lookup_fk_column, int t_lookup_value_column)
{
    m_lookup_view[Column] = t_lookup;
    m_lookup_fk_column[Column] = t_lookup_fk_column;
    m_lookup_value_column[Column] = t_lookup_value_column;
}

void PNSqlQueryModel::setLookup(int Column, QStringList* t_lookup)
{
    m_lookup_values[Column] = t_lookup;
}

QVariant PNSqlQueryModel::getLookupValue(const QModelIndex& t_index)
{
    if ( m_lookup_view[t_index.column()] != nullptr)
    {
        QVariant val = data(t_index);
        QVariant find = m_lookup_view[t_index.column()]->findValue(val, m_lookup_fk_column[t_index.column()], m_lookup_value_column[t_index.column()]);
        return find;
    }

    return QVariant();
}

QString PNSqlQueryModel::getColumnName( QString& t_display_name )
{
    for ( int i = 0; i < m_headers.size(); i++ )
    {
        if ( m_headers[i][Qt::EditRole].toString() == t_display_name )
            return m_sql_query.record().fieldName(i);
    }

    return QString();
}

int PNSqlQueryModel::getColumnNumber(QString &t_field_name)
{
    for (int i = 0; i < m_sql_query.record().count(); i++)
        if (m_sql_query.record().fieldName(i) == t_field_name)
            return i;

    return -1;
}

// TODO: Setup refresh signalling between models

