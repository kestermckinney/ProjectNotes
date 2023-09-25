// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pnsqlquerymodel.h"
#include "pndatabaseobjects.h"

#include <QString>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMessageBox>
#include <QSqlError>
//#include <QDebug>
#include <QDateTime>
#include <QUuid>
#include <QRegularExpressionMatch>
#include <QHashIterator>
#include <QDomDocument>
#include <QDomNode>
#include <QList>
#include <QLocale>

QList<PNSqlQueryModel*> PNSqlQueryModel::m_open_recordsets;

PNSqlQueryModel::PNSqlQueryModel(QObject *t_parent) : QAbstractTableModel(t_parent)
{
    m_open_recordsets.append(this); // add to the list of open recordsets
}

PNSqlQueryModel::~PNSqlQueryModel()
{
    m_open_recordsets.removeAll(this);  // remove from the list of open recordsets
}

void PNSqlQueryModel::refreshImpactedRecordsets(QModelIndex t_index)
{
    // if no tables rely on this record then jump out of this test
    if (m_related_table.count() == 0)
        return;

    QVariant key_value = m_cache[t_index.row()].value(0);

    QListIterator<PNSqlQueryModel*> it_recordsets(m_open_recordsets);
    PNSqlQueryModel* recordset = nullptr;

    // look through all recordsets that are open
    while(it_recordsets.hasNext())
    {
        recordset = it_recordsets.next();

        //qDebug() << "Searching for Table " << recordset->tablename();

        // look through all related tables and uses of the same table to see if the recordset is match
        // don't check against yourself
        if ( recordset != this)
        {
            for (int i = 0; i < m_related_table.count(); i++)
            {
                // if this is a realated table look for related columns
                if ( recordset->tablename().compare( m_related_table[i] ) == 0 )
                {
                    //qDebug() << "Looking Into Table " << recordset->tablename() << " i is " << i;
                    //qDebug() << "Check if table " << recordset->tablename() << " has column " << m_related_columns[i];
                    // we found a table to check, look for the related columns
                    for (QString &c : m_related_columns[i])
                    {
                        int ck_col = recordset->m_sql_query.record().indexOf(c);

                        // if related column is being used then search
                        if (ck_col != -1)
                        {
                            recordset->setDirty();  // refresh when ne page active
                            //qDebug() << "Marking table: " << recordset->tablename() << " as dirty.";
                            break; // just need to identify one column
                        }
                    }
                }
            }

            // if it is the same table in a different recordset reload it
            if ( recordset->tablename().compare(tablename()) == 0 )
            {
                //qDebug() << "Found " << tablename() << " in another model";

                recordset->setDirty();
                //qDebug() << "Marking table: " << recordset->tablename() << " as dirty based on same table name.";
            }
        }
    }
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

    if (m_column_is_editable[t_index.column()] == DBEditable)
        flags |= Qt::ItemIsEditable;

    return flags;
}

bool PNSqlQueryModel::setData(const QModelIndex &t_index, const QVariant &t_value, int t_role)
{
    if (t_role == Qt::EditRole)
    {
        // nothing changed, so do nothing
        if (m_cache[t_index.row()].value(t_index.column()) == t_value)
            return false;

        // make sure column is edit_table
        // exit if no update t_table defined
        if ((m_column_is_editable[t_index.column()] == DBReadOnly) || m_tablename.isEmpty())
            return false;

        if ((m_column_is_required[t_index.column()] == DBRequired) || t_index.column() == 0)
        {
            if (t_value.isNull() || t_value == "")
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update record"),
                   m_headers[t_index.column()][Qt::EditRole].toString() + QObject::tr(" is a required field."), QMessageBox::Ok);

                return false;
            }
        }

        // check to see if you can change it
        if (!columnChangeCheck(t_index))
            return false;

        if (m_column_is_unique[t_index.column()] == DBUnique)
        {
            if (!isUniqueValue(t_value, t_index))
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update record"),
                   m_headers[t_index.column()][Qt::EditRole].toString() + QObject::tr(" must be a unique value."), QMessageBox::Ok);

                reloadRecord(t_index);

                return false;
            }
        }

        // make sure we aren't violating any unique key sets
        if (!checkUniqueKeys(t_index, t_value))
            return false;

        // new records will not have an id and need inserted
        if ( m_cache[t_index.row()].value(0).isNull() )
        {
            QString fields;
            QString values;

            // the record id is always column 0
            // new records will need a new key id set
            m_cache[t_index.row()].setValue(0, QUuid::createUuid().toString());

            QVariant value = t_value;
            sqlEscape(value, m_column_type[t_index.column()], true);

            // set the cached value
            m_cache[t_index.row()].setValue(t_index.column(), value );

            for (int i = 0; i < m_sql_query.record().count(); i++)
            {
                if ((m_column_is_editable[i] == DBEditable) || i == 0)
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
            //qDebug() << "insert into " << m_tablename << " ( " << fields << " ) values ( " << values << " )";

            int bindcount = 0;
            for (int i = 0; i < m_sql_query.record().count(); i++)
            {
                if ((m_column_is_editable[i] == DBEditable) || i == 0)
                {
                    insert.bindValue(bindcount, m_cache[t_index.row()].field(i).value());
                    //qDebug() << "Binding Value " << value << " for " << m_cache[t_index.row()].field(i).name() << " non-escaped val: " << m_cache[t_index.row()].field(i).value();
                    bindcount++;
                }

                // all required fields must be available, otherwise we get a primary key error
                if ( (m_column_is_required[i] == DBRequired) && m_cache[t_index.row()].field(i).value().isNull() && i != 0)
                {
                    // don't insert the record until the required fields are filled in
                    // make the record a new record again
                    m_cache[t_index.row()].setValue(0, QVariant());
                    //qDebug() << "Can't save row column " << i << " is null see -> "  << m_cache[t_index.row()].field(i).value();
                    return false;
                }
            }

            if(insert.exec())
            {
                QModelIndex qil = createIndex(t_index.row(), 0);
                QModelIndex qir = createIndex(t_index.row(), columnCount() - 1);

                emit dataChanged(qil, qir);

                // check for all of the impacted open recordsets
                refreshImpactedRecordsets(t_index);

                return true;
            }

            QMessageBox::critical(nullptr, QObject::tr("Cannot insert record"),
               insert.lastError().text() + "\n" + insert.lastQuery(), QMessageBox::Ok);
        }
        else
        {
            // the record id is always column 0
            QString keycolumnname = m_cache[t_index.row()].fieldName(0);
            QString columnname = m_cache[t_index.row()].fieldName(t_index.column());
            QVariant keyvalue = m_cache[t_index.row()].value(0);
            QVariant oldvalue = m_cache[t_index.row()].value(t_index.column());
            QVariant value = t_value;

            sqlEscape(value, m_column_type[t_index.column()], true);

            QSqlQuery update;
            update.prepare("update " + m_tablename + " set " + columnname + " = ? where " + keycolumnname + " = ? and (" + columnname + " = ? or " + columnname + " is NULL)");
            update.addBindValue(value);
            update.addBindValue(keyvalue);
            update.addBindValue(oldvalue);

            //qDebug() << "update " + m_tablename + " set " + columnname + " = ? where " + keycolumnname + " = ? and (" + columnname + " = ? or " + columnname + " is NULL)";
            //qDebug() << "Value " << value;
            //qDebug() << "Value " << keyvalue;
            //qDebug() << "Value " << oldvalue;

            if(update.exec())
            {
                if (update.numRowsAffected() == 0)
                {
                    QMessageBox::critical(nullptr, QObject::tr("Cannot update value"),
                       QObject::tr("Field was already updated by another process."), QMessageBox::Ok);

                    reloadRecord(t_index);
                }
                else
                {
                    m_cache[t_index.row()].setValue(t_index.column(), value);

                    emit dataChanged(t_index, t_index);

                    // check for all of the impacted open recordsets
                    refreshImpactedRecordsets(t_index);
                    return true;
                }
            }
            else
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot update value"),
                   update.lastError().text() + "\n" + update.lastQuery(), QMessageBox::Ok);
            }
        }
    }

    return false;
}

void PNSqlQueryModel::setBaseSql(const QString t_table)
{
    m_base_sql = t_table;

    m_sql_query = QSqlQuery( BaseSQL() ); // always build query to get the column names for where clause generation
}

void PNSqlQueryModel::setTableName(const QString &t_table, const QString &t_display_name)
{
    m_tablename = t_table;
    m_display_name = t_display_name;
};

void PNSqlQueryModel::refresh()
{
    QString orderby;
    QString fullsql;

    beginResetModel();
    clear();

    if (!m_order_by.isEmpty() )
        orderby = " order by " + m_order_by;

    fullsql = QString("%1 %2 %3").arg( BaseSQL(), constructWhereClause(), orderby);

    m_sql_query = QSqlQuery( fullsql );

    //qDebug() << "Refreshing: ";
    //qDebug() << fullsql;

    // add a blank row for drop downs
    if (m_show_blank)
    {
        m_cache.append(emptyrecord());
    }

    while (m_sql_query.next())
    {
        m_cache.append(m_sql_query.record());

        //QString rowtext = "";
        //for (int c =0; c < m_sql_query.record().count(); c++)
        //    rowtext += m_sql_query.record().value(c).toString() + " : ";

        //qDebug() << rowtext;

    }
    endResetModel();

    m_is_dirty = false;
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
    if (m_cache.size() > t_index.row() && t_role == Qt::BackgroundRole && t_index.row() >= 0)
    {
        if (m_column_is_editable[t_index.column()] == DBReadOnly)
        {
            retval = QVariant(QCOLOR_GRAY);
        }
    }

    return retval;
}

void PNSqlQueryModel::clear()
{
    m_cache.clear();
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

void PNSqlQueryModel::sqlEscape(QVariant& t_column_value, DBColumnType t_column_type, bool t_no_quote) const
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
        case DBDate:
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
        case DBDateTime:
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
        case DBPercent:
        case DBReal:
        case DBInteger:
        case DBBool:
        case DBUSD:
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
        case DBHtml:
        case DBString:
        {
            if (!t_no_quote)
                t_column_value.setValue(t_column_value.toString().replace("'","''"));
            break;
        }
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
                return QCOLOR_BLUE;
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
        case DBHtml:
        case DBString:
        {
            // leave strings alone
            break;
        }
        case DBDate:
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
        case DBDateTime:
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
        case DBReal:
        {
            t_column_value.setValue(t_column_value.toString().replace("$",""));
            t_column_value.setValue(t_column_value.toString().replace("%",""));
            t_column_value.setValue(t_column_value.toString().replace(",",""));
            break;
        }
        case DBInteger:
        case DBBool:
        {
            t_column_value.setValue(t_column_value.toString().replace("$",""));
            t_column_value.setValue(t_column_value.toString().replace("%",""));
            t_column_value.setValue(t_column_value.toString().replace(",",""));
            break;
        }
        case DBUSD:
        {
            t_column_value.setValue(t_column_value.toString().replace("$",""));
            t_column_value.setValue(t_column_value.toString().replace("%",""));
            t_column_value.setValue(t_column_value.toString().replace(",",""));

            QLocale lc;
            t_column_value = lc.toCurrencyString(t_column_value.toDouble());

            break;
        }
        case DBPercent:
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

void PNSqlQueryModel::addColumn(int t_column_number, const QString& t_display_name, DBColumnType t_type, DBColumnSearchable t_searchable, DBColumnRequired t_required, DBColumnEditable t_editable, DBColumnUnique t_unique,
                                QStringList* t_valuelist)
{
    addColumn(t_column_number, t_display_name, t_type, t_searchable, t_required, t_editable, t_unique);

    m_lookup_values[t_column_number] = t_valuelist;
}


void PNSqlQueryModel::addColumn(int t_column_number, const QString& t_display_name, DBColumnType t_type, DBColumnSearchable t_searchable, DBColumnRequired t_required, DBColumnEditable t_editable, DBColumnUnique t_unique,
                                const QString& t_lookup_table, const QString& t_lookup_fk_column_name, const QString& t_lookup_value_column_name)
{
    setHeaderData(t_column_number, Qt::Horizontal, t_display_name);

    m_column_type[t_column_number] = t_type;
    m_column_is_required[t_column_number] = t_required;
    m_column_is_searchable[t_column_number] = t_searchable;
    m_column_is_editable[t_column_number] = t_editable;
    m_column_is_unique[t_column_number] = t_unique;

    m_column_is_filtered[t_column_number] = false;
    m_filter_value[t_column_number] = QString();
    m_filter_compare_type[t_column_number]  = DBCompareType::Equals;

    m_is_user_filtered[t_column_number] = false;
    m_user_filter_values[t_column_number] = QVariantList();
    m_user_search_string[t_column_number] = QVariant();

    m_is_user_range_filtered[t_column_number] = false;
    m_range_search_start[t_column_number] = QVariant();
    m_range_search_end[t_column_number] = QVariant();

    m_lookup_table[t_column_number] = t_lookup_table;
    m_lookup_value_column_name[t_column_number] = t_lookup_value_column_name;
    m_lookup_fk_column_name[t_column_number] = t_lookup_fk_column_name;

    m_lookup_values[t_column_number] = nullptr;
}

int PNSqlQueryModel::rowCount(const QModelIndex &t_parent) const
{
    if (t_parent.isValid())
        return 0;

    return m_cache.size();
}

bool PNSqlQueryModel::copyRecord(QModelIndex t_index)
{
    QSqlRecord newrecord = emptyrecord();

    // don't copy key record so it is identified as a new record
    for (int i = 1; i < m_sql_query.record().count(); i++)
    {
        if (m_column_is_unique[i] == DBUnique)
        {
            newrecord.setValue(i, QString(" Copy of %1").arg(m_cache[t_index.row()].field(i).value().toString()));
        }
        else
        {
            newrecord.setValue(i, m_cache[t_index.row()].field(i).value());
        }
    }

    return(addRecord(newrecord));
}

bool PNSqlQueryModel::addRecord(QSqlRecord& t_newrecord)
{
    QModelIndex qmi = QModelIndex();
    int row = rowCount((qmi));

    beginInsertRows(qmi, row, row);
    m_cache.append(t_newrecord);

    endInsertRows();

    return true;
}

const QModelIndex PNSqlQueryModel::addRecordIndex(QSqlRecord& t_newrecord)
{
    QModelIndex qmi = QModelIndex();
    int row = rowCount((qmi));

    //qDebug() << t_newrecord;

    beginInsertRows(qmi, row, row);
    m_cache.append(t_newrecord);

    endInsertRows();

    return index(row, 0);
}

bool PNSqlQueryModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value1);
    Q_UNUSED(t_fk_value2);

    QSqlRecord qr = emptyrecord();
    return addRecord(qr);
}

bool PNSqlQueryModel::deleteRecord(QModelIndex t_index)
{
    if (!deleteCheck(t_index))
        return false;

    QSqlQuery delrow;
    delrow.prepare("delete from " + m_tablename + " where " + m_sql_query.record().fieldName(0) + " = ? ");
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

bool PNSqlQueryModel::openRecord(QModelIndex t_index)
{
    Q_UNUSED(t_index)

    QMessageBox::critical(nullptr, QObject::tr("Open Record"),
       QObject::tr("Open Record must be defined on child objects."), QMessageBox::Ok);

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
    QString keycolumnname = m_sql_query.record().fieldName(0);

    QString columnname = m_sql_query.record().fieldName(t_index.column());

    QVariant keyvalue;

    if (m_cache.count() > 0) // if not a new record exclude the current record
        keyvalue = m_cache[t_index.row()].value(0).toString();

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

void PNSqlQueryModel::addRelatedTable(const QString& t_table_name, const QString& t_column_name, const QString& t_fk_column_name, const QString& t_title, const DBRelationExportable t_exportable)
{
    QStringList columns;
    QStringList fk_columns;

    columns.append(t_column_name);
    fk_columns.append(t_fk_column_name);

    addRelatedTable(t_table_name, columns, fk_columns, t_title, t_exportable);
}

void PNSqlQueryModel::addRelatedTable(const QString& t_table_name, const QStringList& t_column_names, const QStringList& t_fk_column_names, const QString& t_title, const DBRelationExportable t_exportable)
{
    m_related_table.append(t_table_name);
    m_related_columns.append(t_column_names);
    m_related_fk_columns.append(t_fk_column_names);
    m_relation_title.append(t_title);
    m_relation_exportable.append(t_exportable);
}

bool PNSqlQueryModel::columnChangeCheck(const QModelIndex &t_index)
{
    int reference_count = 0;
    QString message;
    QStringList key_columns;
    QStringList key_values;
    QString project_number_key;
    QString primary_key;

    for (int i = 0; i < m_related_table.size(); ++i)
    {
        int relatedcount = 0;
        bool use_related = false;

        //set the where for all
        QString where_clause;

        for (int c = 0; c < m_related_columns[i].count(); c++)
        {
            QString col_name = m_related_columns[i].at(c);
            QString fk_col_name = m_related_fk_columns[i].at(c);

            // look to see if the current column has a related value otherwise dont consider this related table
            if (fk_col_name.compare(m_cache[t_index.row()].fieldName(t_index.column())) == 0)
                use_related = true;

            QVariant col_val = m_cache[t_index.row()].value(fk_col_name).toString();
            int col_num = getColumnNumber(fk_col_name);

            sqlEscape(col_val, m_column_type[col_num]);

            if (!where_clause.isEmpty())
                where_clause += " and ";

            where_clause += QString(" %1 = '%2' ").arg(col_name, col_val.toString());

            // special key field mapping
            // check to see if search should be limited by project
            if (fk_col_name.compare("project_id") == 0)
            {
                QSqlQuery projsql;
                projsql.prepare(QString("select project_number from projects where project_id ='%1'").arg(col_val.toString()));
                projsql.exec();

                if (projsql.next())
                    project_number_key = projsql.value(0).toString();
            }
            else
            {
                primary_key = col_val.toString();
            }
        }

        if (use_related)
        {
            QSqlQuery select;
            select.prepare("select count(*) from " + m_related_table.at(i) + " where " + where_clause);
            select.exec();

            //qDebug() << "SET VALUE CHECK: " << "select count(*) from " + m_related_table.at(i) + " where " + where_clause;

            if (select.next())
                relatedcount = select.value(0).toInt();

            if (relatedcount > 0)
            {
                reference_count += relatedcount;

                message += select.value(0).toString() + " " + m_relation_title.at(i) + " record(s)\n";
            }
        }
    }

    if (reference_count > 0)
    {
        message = QObject::tr("The ") + m_display_name + QObject::tr(" record is referenced in the following records:\n\n") + message +
                 QObject::tr("\nYou cannot change the ") + m_display_name + QObject::tr(" record until the assocated records are changed. Would you like to run a search for all related records?");

        if ( QMessageBox::question(nullptr, QObject::tr("Cannot change record"),
           message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes )
        {
            if (!project_number_key.isEmpty())
            {
                key_columns.append("project_number");
                key_values.append(project_number_key);
                key_columns.append("datakey");
                key_values.append(primary_key);
            }
            else
            {
                key_columns.append("datakey");
                key_values.append(primary_key);
            }

            global_DBObjects.searchresultsmodel()->PerformKeySearch( key_columns, key_values );

            emit callKeySearch();
        }

        return false;
    }

    return true;
}

bool PNSqlQueryModel::deleteCheck(const QModelIndex &t_index)
{
    int reference_count = 0;
    QString message;
    QStringList key_columns;
    QStringList key_values;
    QString project_number_key;
    QString primary_key;

    for (int i = 0; i < m_related_table.size(); ++i)
    {
        int relatedcount = 0;

        //set the where for all
        QString where_clause;

        for (int c = 0; c < m_related_columns[i].count(); c++)
        {
            QString col_name = m_related_columns[i].at(c);
            QString fk_col_name = m_related_fk_columns[i].at(c);
            QVariant col_val = m_cache[t_index.row()].value(fk_col_name).toString();
            int col_num = getColumnNumber(fk_col_name);

            sqlEscape(col_val, m_column_type[col_num]);

            if (!where_clause.isEmpty())
                where_clause += " and ";

            where_clause += QString(" %1 = '%2' ").arg(col_name, col_val.toString());

            // special key field mapping
            // check to see if search should be limited by project
            if (fk_col_name.compare("project_id") == 0)
            {
                QSqlQuery projsql;
                projsql.prepare(QString("select project_number from projects where project_id ='%1'").arg(col_val.toString()));
                projsql.exec();

                if (projsql.next())
                    project_number_key = projsql.value(0).toString();
            }
            else
            {
                primary_key = col_val.toString();
            }
        }

        QSqlQuery select;
        select.prepare("select count(*) from " + m_related_table.at(i) + " where " + where_clause);
        select.exec();

        //qDebug() << "DELETE CHECK: " << "select count(*) from " + m_related_table.at(i) + " where " + where_clause;

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
            if (!project_number_key.isEmpty())
            {
                key_columns.append("project_number");
                key_values.append(project_number_key);
                key_columns.append("datakey");
                key_values.append(primary_key);
            }
            else
            {
                key_columns.append("datakey");
                key_values.append(primary_key);
            }

            global_DBObjects.searchresultsmodel()->PerformKeySearch( key_columns, key_values );

            emit callKeySearch();
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

const QModelIndex PNSqlQueryModel::findIndex(QVariant& t_lookup_value, int t_search_column)
{
    int row = 0;

    for ( QVector<QSqlRecord>::Iterator itrow = m_cache.begin(); itrow != m_cache.end(); ++itrow )
    {
        if ( itrow->value(t_search_column) == t_lookup_value )
        {
            return index(row, 0); // key is always at 0
        }

        row++;
    }

    return QModelIndex();
}


bool PNSqlQueryModel::reloadRecord(const QModelIndex& t_index)
{
    QSqlQuery select;
    select.prepare(BaseSQL() + " where " + m_sql_query.record().fieldName(0) + " = ? ");
    select.bindValue(0, m_cache[t_index.row()].field(0).value());

    if (select.exec())
    {
        if (select.next())
        {
            m_cache[t_index.row()] = select.record();

            emit dataChanged(t_index.model()->index(t_index.row(), 0), t_index.model()->index(t_index.row(), select.record().count()));

            //qDebug() << "emmiting data changed for " << tablename() << " object " << objectName() << " row " << t_index.row() << " for columns 0 to " << select.record().count();

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

                QString compare_op;

                switch (m_filter_compare_type[hashit.key()])
                {
                case DBCompareType::GreaterThan:
                    compare_op = ">";
                    break;
                case DBCompareType::LessThan:
                    compare_op = "<";
                    break;
                case DBCompareType::NotEqual:
                    compare_op = "<>";
                    break;
                default:
                    compare_op = "=";
                }

                sqlEscape(column_value, m_column_type[hashit.key()]);

                if ( m_column_type[hashit.key()] != DBString && m_column_type[hashit.key()] != DBHtml)
                {
                    if (m_column_type[hashit.key()] == DBBool && column_value == tr("0"))
                    {
                        valuelist += QString(" ( %1 %3 %2").arg(m_sql_query.record().fieldName(hashit.key()), column_value.toString(), compare_op);
                        valuelist += QString(" OR %1 IS NULL) ").arg( m_sql_query.record().fieldName(hashit.key()) );
                    }
                    else
                        valuelist += QString("%1 = %2").arg( m_sql_query.record().fieldName(hashit.key()), column_value.toString() );
                }
                else
                {
                    //qDebug() << "Table Naame: " << BaseSQL() << " Column Num: " << hashit.key() << "  Column Name: " << m_sql_query.record().fieldName(hashit.key());

                    sqlEscape(column_value, m_column_type[hashit.key()]);
                    valuelist += QString("%1 %3 '%2'").arg( m_sql_query.record().fieldName(hashit.key()), column_value.toString(), compare_op);
                }
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
                if ( !m_user_search_string[colnum].isNull() && !(m_user_search_string[colnum] == ""))
                {
                    column_value = m_user_search_string[colnum];

                    sqlEscape(column_value, m_column_type[colnum]);

                    if (!valuelist.isEmpty())
                        valuelist += tr(" AND ");


                    if (  m_column_type[colnum] == DBString ||  m_column_type[colnum] == DBHtml )
                    {
                        if ( !m_lookup_table[colnum].isEmpty() )
                        {
                            QString fk_key_val =  m_user_search_string[colnum].toString();
                            valuelist +=  QString(" %5 in (select %1 from %2 where %3 LIKE '%%4%')").arg(m_lookup_fk_column_name[colnum], m_lookup_table[colnum], m_lookup_value_column_name[colnum], fk_key_val, m_sql_query.record().fieldName(colnum));
                        }
                        else
                            valuelist += QString(" %1 LIKE '%%2%' ").arg(m_sql_query.record().fieldName(colnum), column_value.toString());
                    }
                    else
                    {
                        if ( !m_lookup_table[colnum].isEmpty() )
                        {
                            QString fk_key_val =  m_user_search_string[colnum].toString();
                            valuelist +=  QString(" %5 in (select %1 from %2 where %3 = '%%4%')").arg(m_lookup_fk_column_name[colnum], m_lookup_table[colnum], m_lookup_value_column_name[colnum], fk_key_val, m_sql_query.record().fieldName(colnum));
                        }
                        else
                            valuelist += QString(" %1 = %2 ").arg(m_sql_query.record().fieldName(colnum), column_value.toString());
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

                    if ( m_column_type[colnum] == DBString || m_column_type[colnum] == DBHtml )
                        instring += QString("'%1'").arg(column_value.toString());
                    else
                        instring += QString("%1").arg(column_value.toString());

                    if (m_column_type[colnum] == DBBool && column_value == tr("'0'"))
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

                    if ( !m_lookup_table[colnum].isEmpty() )
                    {
                        QString valuerange;

                        // if the column doesn't have a lookup value in a foreign key
                        // you can do the range search on the value
                        if (!RangeStart.isNull() && RangeStart != tr("''"))
                        {
                            if ( m_column_type[colnum] != DBString && m_column_type[colnum] != DBHtml)
                            {
                                valuerange += QString("%1 >= %2").arg(m_lookup_value_column_name[colnum], RangeStart.toString());
                            }
                            else
                            {
                                valuerange += QString("%1 >= '%2'").arg(m_lookup_value_column_name[colnum], RangeStart.toString());
                            }
                        }

                        if (!RangeEnd.isNull() && RangeEnd != tr("''"))
                        {
                            if (!valuerange.isEmpty())
                                valuerange += tr(" AND ");

                            if ( m_column_type[colnum] != DBString && m_column_type[colnum] != DBHtml)
                            {
                                valuerange += QString("%1 <= %2").arg(m_lookup_value_column_name[colnum], RangeEnd.toString());
                            }
                            else
                            {
                                valuerange += QString("%1 <= '%2'").arg(m_lookup_value_column_name[colnum], RangeEnd.toString());
                            }
                        }

                        if (!valuelist.isEmpty())
                            valuelist += tr(" AND ");

                        QString fk_key_val =  m_user_search_string[colnum].toString();
                        valuelist +=  QString(" %4 in (select %1 from %2 where %3)").arg(m_lookup_fk_column_name[colnum], m_lookup_table[colnum], valuerange, m_sql_query.record().fieldName(colnum));
                    }
                    else
                    {
                        // if the column doesn't have a lookup value in a foreign key
                        // you can do the range search on the value
                        if (!RangeStart.isNull() && RangeStart != tr("''"))
                        {
                            if (!valuelist.isEmpty())
                                valuelist += tr(" AND ");

                            if ( m_column_type[colnum] != DBString && m_column_type[colnum] != DBHtml)
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


                            if ( m_column_type[colnum] != DBString && m_column_type[colnum] != DBHtml)
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

void PNSqlQueryModel::setFilter(int t_column_number, const QString& t_filter_value, DBCompareType t_compare_type)
{
    m_filter_value[t_column_number] = t_filter_value;
    m_column_is_filtered[t_column_number] = true;
    m_filter_compare_type[t_column_number] = t_compare_type;
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
    m_filter_compare_type[t_column_number] = DBCompareType::Equals;
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

QDomElement PNSqlQueryModel::toQDomElement( QDomDocument* t_xml_document, const QString& t_filter )
{
    // if there is a filter let's apply it
    if (!t_filter.isEmpty())
        if (!t_filter.contains(this->tablename(), Qt::CaseInsensitive))
            return QDomElement();

    // some data models are just for drop downs
    if (!isExportable())
        return QDomElement();

    QDomElement xmltable = t_xml_document->createElement("table");
    xmltable.toElement().setAttribute("name", this->tablename());

    for ( const auto& row : m_cache )
    {
        QDomElement xmlrow = t_xml_document->createElement("row");
        xmlrow.setAttribute("id", row.value(0).toString());

        // build the column xml
        for ( int i = 0; i < row.count(); i++ )
        {
            QDomElement xmlcolumn = t_xml_document->createElement("column");
            xmlcolumn.setAttribute("name", row.fieldName(i));

            QVariant val = row.value(i);
            reformatValue(val, getType(i));

            if (getType(i) == DBHtml)
            {
                // need a specific type here
                QDomCDATASection xmlcdata = t_xml_document->createCDATASection(val.toString());
                xmlcolumn.appendChild(xmlcdata);
            }
            else
            {
                QDomText xmltext = t_xml_document->createTextNode(val.toString());
                xmlcolumn.appendChild(xmltext);
            }

            if ( !m_lookup_table[i].isEmpty() )
            {
                QString fk_key_val = row.value(i).toString();
                QString sql = QString("select %1 from %2 where %3 = '%4'").arg(m_lookup_value_column_name[i], m_lookup_table[i], m_lookup_fk_column_name[i], fk_key_val);
                QString lookup_value = global_DBObjects.execute(sql);

                if (!lookup_value.isEmpty())
                    xmlcolumn.setAttribute("lookupvalue", lookup_value);
            }

            xmlrow.appendChild(xmlcolumn);
        }

        // append related tables into the row

        // for loop through a list of related tables
        for (int i = 0; i < m_related_table.count(); i++)
        {
            // not all related items are exportable
            // it would be too much data
            if (m_relation_exportable[i] == DBExportable)
            {
                //find that table in the database objects
                QListIterator<PNSqlQueryModel*> it_recordsets(m_open_recordsets);
                PNSqlQueryModel* recordset = nullptr;

                // look through all recordsets that are open
                while(it_recordsets.hasNext())
                {
                    recordset = it_recordsets.next();

                    if ( recordset->tablename().compare( m_related_table[i] ) == 0 &&
                         recordset->isExportable() &&
                         (  ( !t_filter.isEmpty() && t_filter.contains(recordset->tablename(), Qt::CaseInsensitive) ) || t_filter.isEmpty()  ) // don't refresh the recordset if it is filtered
                         )
                    {
                        //qDebug() << "Found table " << tablename() << " column " << m_related_fk_column[i] << " related to " << recordset->tablename() << " column " << m_related_column[i];

                        // create an export version of that querymodel
                        PNSqlQueryModel* export_version = recordset->createExportVersion();

                        //set the filter for the export version
                        for (int c = 0; c < m_related_columns[i].count(); c++)
                        {
                            QString col_name = m_related_columns[i].at(c);
                            QString fk_col_name = m_related_fk_columns[i].at(c);

                            int col = export_version->getColumnNumber(col_name);
                            int fkcol = getColumnNumber(fk_col_name);

                            export_version->setFilter(col, row.value(fkcol).toString());
                        }

                        export_version->refresh();

                        QDomElement qd = export_version->toQDomElement( t_xml_document, t_filter );

                        for (int c = 0; c < m_related_columns[i].count(); c++)
                        {
                            QString col_name = m_related_columns[i].at(c);
                            QString fk_col_name = m_related_fk_columns[i].at(c);

                            int fkcol = getColumnNumber(fk_col_name);

                            qd.setAttribute(QString("filter_field_%1").arg(c + 1), col_name);
                            qd.setAttribute(QString("filter_value_%1").arg(c + 1), row.value(fkcol).toString());
                        }

                        // add the new XML to the current row
                        xmlrow.appendChild(qd);

                        delete export_version;
                        break; // only grab the first instance of the related table
                    }
                }
            }
        }

        xmltable.appendChild(xmlrow);
    }

    return xmltable;
}

PNSqlQueryModel* PNSqlQueryModel::createExportVersion()
{
    return new PNSqlQueryModel(this);
}

bool PNSqlQueryModel::importXMLNode(const QDomNode& t_domnode)  // this should be table level
{
    QDomElement node = t_domnode.firstChildElement("row");

    while (!node.isNull())
    {
        if (!setData(&node, false))
            return false; // jump out if we have an error

        node = node.nextSiblingElement("row");
    }

    return true;
}

bool PNSqlQueryModel::setData(QDomElement* t_xml_row, bool t_ignore_key)
{
    if (t_xml_row->tagName() != "row")
    {
        //qDebug() << "tag name: " << t_xml_row->tagName() << " is not a 'row'";
        return false;
    }

    QString whereclause;
    QString fields;
    QString updatevalues;
    QString insertvalues;
    QString keyfield = getColumnName(0);
    QString keyvalue;

    // determine if identifier should be used
    if (!t_ignore_key)
    {
        keyvalue = t_xml_row->attribute("id");

        if (!keyvalue.isNull())
            whereclause = QString(" %1 = '%2'").arg(keyfield, keyvalue);
    }


    // if using keys don't check for unique values
    if (keyvalue.isNull())
    {
        // Loop through unique keys to find one that can be used to identify a record without the record id
        for (const QStringList& uk : m_unique_keys)
        {
            // loop key fields assumming they are there
            int found_count = 0;
            QString temp_where;

            for (const QString& kf : uk)
            {
                // CHECK XML FOR COLUMNS
                QDomNode element = t_xml_row->firstChild();
                while (!element.isNull())
                {
                    if (element.toElement().tagName() == "column")
                    {
                        QString field_name = element.toElement().attribute("name");
                        QVariant field_value = element.toElement().text();
                        QString lookup_value = element.toElement().attribute("lookupvalue");

                        int colnum = getColumnNumber(field_name);

                        // if column has a lookup value, look up the key value
                        if (!lookup_value.isNull() && !m_lookup_table[colnum].isEmpty())
                        {
                            QString sql = QString("select %1 from %2 where %3 = '%4'").arg(m_lookup_fk_column_name[colnum], m_lookup_table[colnum], m_lookup_value_column_name[colnum], lookup_value);
                            //qDebug() << "EXEC LOOKUP EXISTING: " << sql;

                            field_value = global_DBObjects.execute(sql);
                        }

                        // if this is a key field add to temp wherer
                        if (field_name.compare(kf, Qt::CaseInsensitive) == 0)
                        {
                            sqlEscape(field_value, m_column_type[colnum], false);

                            if (!temp_where.isEmpty())
                                temp_where += " and ";
                            temp_where += QString(" %1 = '%2'").arg(field_name, field_value.toString());
                            found_count++;
                        }
                    }

                    element = element.nextSibling();
                }

                // IF KEY IS USABLE ADD TO WHERE CLAUSE
                if (found_count == uk.count())
                {
                    if (!whereclause.isEmpty())
                        whereclause += " and ";
                    whereclause += temp_where;
                }

            }
        }
    }

    QDomNode element = t_xml_row->firstChild();
    while (!element.isNull())
    {
        if (element.toElement().tagName() == "column")
        {
            QString field_name = element.toElement().attribute("name");
            QVariant field_value = element.toElement().text();
            QString lookup_value = element.toElement().attribute("lookupvalue");

            int colnum = getColumnNumber(field_name);

            // if key column is specified blank or null don't use it
            // don't use if it isn't an editable column
            if ( !(colnum == 0 && (field_value.isNull() || field_value.toString().isEmpty())) &&
                 isEditable(colnum) )
            {
                if (!fields.isEmpty())
                    fields += ",";

                fields += field_name;

                // if column has a lookup value, look up the key value
                if (!lookup_value.isNull() && !m_lookup_table[colnum].isEmpty())
                {
                    QString sql = QString("select %1 from %2 where %3 = '%4'").arg(m_lookup_fk_column_name[colnum], m_lookup_table[colnum], m_lookup_value_column_name[colnum], lookup_value);
                    //qDebug() << "EXEC LOOKUP FOR FIELD VALUE: " << sql;

                    field_value = global_DBObjects.execute(sql);
                }

                if (!insertvalues.isEmpty())
                    insertvalues += ",";


                if (!updatevalues.isEmpty())
                    updatevalues += ",";

                // if list of values doesn't contain this value the record need rejected
                if (m_lookup_values[colnum] && !m_lookup_values[colnum]->contains(field_value.toString(), Qt::CaseSensitive))
                {
                    QMessageBox::critical(nullptr, QObject::tr("Invalid Field Value"), QString("""%1"" is not a valid field value.").arg(field_value.toString()));
                    return false;
                }

                sqlEscape(field_value, m_column_type[colnum], false);

                if (field_value.isNull())
                {
                    updatevalues += QString("%1 = NULL").arg(field_name);
                    insertvalues += "NULL";
                }
                else
                {
                    updatevalues += QString("%1 = '%2'").arg(field_name, field_value.toString());
                    insertvalues += QString("'%1'").arg(field_value.toString());
                }
            }
        }

        element = element.nextSibling();
    }

    // check to see if record exists
    QString exists_sql = QString("select count(*) from %1 where %2").arg(m_tablename, whereclause);
    QString exists_count = global_DBObjects.execute(exists_sql);
    QString sql;

    //qDebug() << "CHECK EXISTS: " << exists_sql;

    if (exists_count.toInt() > 0)
    {
        // update if exists
        sql = QString("update %1 set %2 where %3").arg(m_tablename, updatevalues, whereclause);
    }
    else
    {
        // insert if it doesn't exist
        // needs an ID since one won't exist
        if (!fields.isEmpty())
            fields += ",";

        fields += getColumnName(0); // get the key field name

        if (!insertvalues.isEmpty())
            insertvalues += ",";

        insertvalues += QString("'%1'").arg(QUuid::createUuid().toString());


        sql = QString("insert into %1 (%2) values (%3)").arg(m_tablename, fields, insertvalues);
    }

    //qDebug() << "XML Generated SQL: " << sql;

    global_DBObjects.execute(sql);

    return true;
}

void PNSqlQueryModel::refreshByTableName()
{
    QListIterator<PNSqlQueryModel*> it_recordsets(m_open_recordsets);
    PNSqlQueryModel* recordset = nullptr;

    // look through all recordsets that are open
    while(it_recordsets.hasNext())
    {
        recordset = it_recordsets.next();

        // look through all related tables and uses of the same table to see if the recordset is match
        // don't check against yourself, especially when importing you are just using a empty recordset
        if ( recordset != this && this->tablename() == recordset->tablename())
        {
            recordset->setDirty();
        }
    }
}

bool PNSqlQueryModel::checkUniqueKeys(const QModelIndex &t_index, const QVariant &t_value)
{
    QString checkfield = m_cache[t_index.row()].field(t_index.column()).name();

    QHash<QString, QStringList>::iterator itk;

    for ( itk = m_unique_keys.begin(); itk != m_unique_keys.end(); ++itk  )
    {
        QString where;
        bool isrelevent = false;

        foreach ( const QString f, itk.value() )
        {
            if ( f.compare(checkfield) == 0 )
                isrelevent = true;

            if (!where.isEmpty())
                where += " and ";
            where += QString("%1 = ?").arg(f);
        }

        // if the field we are checking is relevent check it
        if (isrelevent)
        {
            where = QString("select count(*) from %1 where ").arg(tablename()) + where;

            //qDebug() << "Verifying Unique:  " << where;

            QSqlQuery qry;
            qry.prepare(where);

            foreach ( const QString f, itk.value() )
            {
                if ( f.compare(checkfield) == 0 )
                {
                    //qDebug() << "binding...  " << t_value;
                    qry.addBindValue(t_value); // check the value we want to update with
                }
                else
                {
                    //qDebug() << "binding... " << m_cache[t_index.row()].field(f).value();
                    qry.addBindValue(m_cache[t_index.row()].field(f).value());
                }
            }

            qry.exec();

            if (qry.next())
            {
                if ( qry.value(0).toInt() > 0)
                {

                    QMessageBox::warning(nullptr, QObject::tr("Cannot update record"),
                       QString("%1 must be unique.").arg(itk.key()));

                    return false;
                }
            }
        }
    }

    return true;
}

//TODO: establish a progress bar while generating the XML

