// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseobjects.h"

#include <QString>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMessageBox>
#include <QSqlError>
#include <QDateTime>
#include <QUuid>
#include <QRegularExpressionMatch>
#include <QHashIterator>
#include <QDomDocument>
#include <QDomNode>
#include <QList>
#include <QLocale>
#include <QReadWriteLock>
#include <QApplication>
#include <QPalette>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

SqlQueryModel::SqlQueryModel(DatabaseObjects* dbo) : QAbstractTableModel(dbo)
{
    m_dbo = dbo;
    m_gui = m_dbo->hasGUI();

    // only recordsets attached to gui need to be updated with related data items changes
    // non gui mult-threaded recordsets can en up in deadlocks
    if (m_gui)
    {
        m_dbo->addModel(this);
    }
}

SqlQueryModel::~SqlQueryModel()
{
    if (m_gui)
        m_dbo->removeModel(this);
}

void SqlQueryModel::refreshImpactedRecordsets(QModelIndex index)
{
    QListIterator<SqlQueryModel*> it_recordsets(m_dbo->getOpenModels());
    SqlQueryModel* recordset = nullptr;

    // look through all recordsets that are open
    while(it_recordsets.hasNext())
    {
        recordset = it_recordsets.next();

        // look through all related tables and uses of the same table to see if the recordset is match
        // don't check against yourself
        if ( recordset != this)
        {
            // O(1) hash lookup instead of O(n) linear scan through m_relatedTable
            auto it = m_relatedTableIndex.constFind(recordset->tablename());
            if (it != m_relatedTableIndex.constEnd())
            {
                const int i = it.value();
                // we found a related table; look for the related columns
                for (const QString& c : m_relatedColumns[i])
                {
                    int ck_col = recordset->getColumnNumber(c);

                    // if related column is being used then search
                    if (ck_col != -1)
                    {
                        QVariant val = m_cache[index.row()].value(0);
                        QModelIndex qmi = recordset->findIndex(val, ck_col);
                        if (qmi.isValid())
                        {
                            recordset->reloadRecord(qmi);
                        }
                    }
                }
            }

            // if this is the same table it needs updated too
            if (recordset->tablename() == tablename())  // if it is the same table in a different recordset it still needs updated
            {
                QVariant val = m_cache[index.row()].value(0);
                QModelIndex qmi = recordset->findIndex(val, 0);
                if (qmi.isValid())
                {
                    recordset->reloadRecord(qmi);
                }
                else
                {
                    recordset->copyAndFilterRow(index, *this);
                }
            }
        }
    }
}

int SqlQueryModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_columnCount;
    else
        return 0;
}

Qt::ItemFlags SqlQueryModel::flags(
        const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    if (m_columnIsEditable[index.column()] == DBEditable)
        flags |= Qt::ItemIsEditable;

    return flags;
}

bool SqlQueryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
        // nothing changed, so do nothing
        // Compare using escaped (DB-compatible) form to handle type mismatches
        // (e.g., cached epoch qint64 vs incoming "MM/dd/yyyy" string for DBDate,
        //  or cached double vs incoming "$1,234.56" string for DBUSD)
        {
            QVariant testEscaped = value;
            sqlEscape(testEscaped, m_columnType[index.column()], true);
            const QVariant& cached = m_cache[index.row()].value(index.column());
            if (cached == testEscaped ||
                (!cached.isNull() && !testEscaped.isNull() && cached.toString() == testEscaped.toString()))
                return false;
        }

        // make sure column is edit_table
        // exit if no update table defined
        if ((m_columnIsEditable[index.column()] == DBReadOnly) || m_tablename.isEmpty())
        {
            emit dataChanged(index, index); // reload correct value
            return false;
        }

        if ((m_columnIsRequired[index.column()] == DBRequired) || index.column() == 0)
        {
            if (value.isNull() || value == "")
            {
                if (m_gui)
                {
                    QMessageBox::critical(nullptr, QObject::tr("Cannot update record"),
                       m_headers[index.column()][Qt::EditRole].toString() + QObject::tr(" is a required field."), QMessageBox::Ok);
                }

                emit dataChanged(index, index); // reload correct value
                return false;
            }
        }

        // check to see if you can change it
        if (!columnChangeCheck(index))
        {
            emit dataChanged(index, index); // reload correct value
            return false;
        }

        if (m_columnIsUnique[index.column()] == DBUnique)
        {
            if (!isUniqueValue(value, index))
            {
                if (m_gui)
                {
                    QMessageBox::critical(nullptr, QObject::tr("Cannot update record"),
                       m_headers[index.column()][Qt::EditRole].toString() + QObject::tr(" must be a unique value."), QMessageBox::Ok);
                }

                emit dataChanged(index, index); // reload correct value

                return false;
            }
        }

        // make sure we aren't violating any unique key sets
        if (!checkUniqueKeys(index, value))
        {
            emit dataChanged(index, index); // reload correct value
            return false;
        }

        // new records will not have an id and need inserted
        if ( m_cache[index.row()][0].isNull() )
        {
            QString fields;
            QString values;

            // the record id is always column 0
            // new records will need a new key id set
            m_cache[index.row()][0] = QUuid::createUuid().toString();

            QVariant escapedValue = value;
            sqlEscape(escapedValue, m_columnType[index.column()], true);

            // set the cached value
            m_cache[index.row()][index.column()] = escapedValue;

            for (int i = 0; i < m_columnCount; i++)
            {
                if ((m_columnIsEditable[i] == DBEditable) || i == 0)
                {
                    if (!fields.isEmpty())
                        fields += ", ";

                    if (!values.isEmpty())
                        values += ", ";

                    fields += m_columnName[i];

                    values += " ? ";
                }
            }

            QSqlQuery insert(getDBOs()->getDb());
            insert.prepare("insert into " + m_tablename + " ( " + fields + " ) values ( " + values + " )");

            int bindcount = 0;
            for (int i = 0; i < m_columnCount; i++)
            {
                if ((m_columnIsEditable[i] == DBEditable) || i == 0)
                {
                    insert.bindValue(bindcount, m_cache[index.row()][i]);
                    // qDebug() << "Binding Value " << m_cache[index.row()][i] << " for " <<  this->getColumnName(i);
                    bindcount++;
                }

                // all required fields must be available, otherwise we get a primary key error
                if ( (m_columnIsRequired[i] == DBRequired) && m_cache[index.row()][i].isNull() && i != 0)
                {
                    // don't insert the record until the required fields are filled in
                    // make the record a new record again
                    m_cache[index.row()][0] = QVariant();
                    emit dataChanged(index, index); // reload correct value

                    return false;
                }
            }

            DB_LOCK;
            getDBOs()->getDb().transaction();
            if(insert.exec())
            {
                getDBOs()->getDb().commit();
                DB_UNLOCK;

                QModelIndex qil = createIndex(index.row(), 0);
                QModelIndex qir = createIndex(index.row(), columnCount() - 1);

                emit dataChanged(qil, qir);

                // check for all of the impacted open recordsets
                if (m_gui) // some recordsets aren't attached to the gui
                    refreshImpactedRecordsets(index);

                return true;
            }
            getDBOs()->getDb().rollback();
            DB_UNLOCK;

            if (m_gui)
            {
                QMessageBox::critical(nullptr, QObject::tr("Cannot insert record"),
                   insert.lastError().text() + "\n" + insert.lastQuery(), QMessageBox::Ok);
            }
        }
        else
        {
            // the record id is always column 0
            QString keycolumnname = m_columnName[0];
            QString columnname = m_columnName[index.column()];
            QVariant keyvalue = m_cache[index.row()][0];
            QVariant oldvalue = m_cache[index.row()][index.column()];
            QVariant escapedValue = value;

            sqlEscape(escapedValue, m_columnType[index.column()], true);

            QSqlQuery update(getDBOs()->getDb());
            update.prepare("update " + m_tablename + " set " + columnname + " = ? where " + keycolumnname + " = ? and (" + columnname + " = ? or " + columnname + " is NULL)");
            update.addBindValue(escapedValue);
            update.addBindValue(keyvalue);
            update.addBindValue(oldvalue);

            DB_LOCK;
            getDBOs()->getDb().transaction();
            if(update.exec())
            {
                getDBOs()->getDb().commit();
                DB_UNLOCK;

                if (update.numRowsAffected() == 0)
                {
                    if (m_gui)
                    {
                        QMessageBox::critical(nullptr, QObject::tr("Cannot update value"),
                           QObject::tr("Field was already updated by another process."), QMessageBox::Ok);
                    }

                    reloadRecord(index);
                }
                else
                {
                    // Store escaped (DB-compatible) value so cache stays consistent
                    // with DB format. Storing raw 'value' (e.g. "12/15/2023") would
                    // break reformatValue (toLongLong gives 0) and break subsequent
                    // WHERE clauses (string vs integer epoch mismatch).
                    m_cache[index.row()][index.column()] = escapedValue;

                    emit dataChanged(index, index);

                    // check for all of the impacted open recordsets
                    if (m_gui) // some recordsets aren't attached to the gui
                       refreshImpactedRecordsets(index);

                    return true;
                }
            }
            else
            {
                getDBOs()->getDb().rollback();
                DB_UNLOCK;

                if (m_gui)
                {
                    QMessageBox::critical(nullptr, QObject::tr("Cannot update value"),
                       update.lastError().text() + "\n" + update.lastQuery(), QMessageBox::Ok);
                }

            }
        }
    }

    return false;
}

void SqlQueryModel::setBaseSql(const QString& table)
{
    m_baseSql = table;

    // populate the column name lookup hash for fast getColumnNumber() lookups
    m_columnNameLookup.clear();
    for (int i = 0; i < m_columnCount; i++)
        m_columnNameLookup[m_columnName[i]] = i;
}

void SqlQueryModel::setTableName(const QString &table, const QString &displayName)
{
    m_tablename = table;
    m_displayName = displayName;
};

void SqlQueryModel::refresh()
{
    QSqlQuery sql_query;
    QString orderby;
    QString fullsql;
    QString skip;
    QString top;

    beginResetModel();
    clear();

    if (!m_orderBy.isEmpty() )
        orderby = " order by " + m_orderBy;

    if (m_top != 0)
        top = QString("LIMIT %1").arg(m_top);

    if (m_skip != 0 && m_top != 0)
        skip = QString("OFFSET %1").arg(m_skip);

    fullsql = QString("%1 %2 %3 %4 %5").arg( BaseSQL(), constructWhereClause(), orderby, top, skip);

    getDBOs()->getDb().transaction();
    sql_query = QSqlQuery( getDBOs()->getDb() );
    sql_query.setForwardOnly(true);
    sql_query.prepare(fullsql);
    sql_query.exec();

    // add a blank row for drop downs
    if (m_showBlank)
    {
        m_cache.append(emptyrecord());
    }

    while (sql_query.next())
    {
        QVector<QVariant> record(m_columnCount);

        for (int i = 0; i < sql_query.record().count(); i++)
            record[i] = sql_query.value(i);

        m_cache.append(record);
    }
    getDBOs()->getDb().commit();

    endResetModel();
}

QVariant SqlQueryModel::data(const QModelIndex &index, int role) const
{
    QVariant retval;

    if (m_cache.size() > index.row() && (role == Qt::DisplayRole || role == Qt::EditRole) && index.row() >= 0)
    {
        retval = m_cache[index.row()][index.column()];
        reformatValue(retval, m_columnType[index.column()]);
    }

    // make a light gray backround when not edit_table
    if (m_cache.size() > index.row() && role == Qt::BackgroundRole && index.row() >= 0)
    {
        if (m_columnIsEditable[index.column()] == DBReadOnly)
        {
            retval = QVariant(QApplication::palette().color(QPalette::Button));
        }
    }

    return retval;
}

void SqlQueryModel::clear()
{
    m_cache.clear();
}

QDateTime SqlQueryModel::parseDateTime(const QString& entrydate)
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

void SqlQueryModel::sqlEscape(QVariant& columnValue, DBColumnType columnType, bool noQuote) const
{
    // don't store blank values
    if ( columnValue.isNull() )
    {
        return;
    }

    // make blank entries nullptr
    if ( columnValue.toString().isEmpty())
    {
        columnValue.clear();
        return;
    }

    switch (columnType) {
        case DBDate:
        {
            QDateTime datecol = parseDateTime(columnValue.toString());

            if ( datecol.isValid() )
            {
                columnValue = datecol.toSecsSinceEpoch();
            }
            else
                columnValue.clear();

            break;
        }
        case DBDateTime:
        {
            QDateTime datecol = parseDateTime(columnValue.toString());
            if ( datecol.isValid() )
            {
                if (datecol.date().year() < 1000)
                {
                    qint32 adjustment = QDate::currentDate().year();
                    adjustment = trunc((float)adjustment / 100.0) * 100;

                    datecol = datecol.addYears(adjustment);
                }

                columnValue = datecol.toSecsSinceEpoch();
            }
            else
                columnValue.clear();

            break;
        }
        case DBPercent:
        case DBReal:
        case DBInteger:
        case DBBool:
        case DBUSD:
        {
            columnValue.setValue(columnValue.toString().toLower());
            if (columnValue == "true")
                columnValue = "1";
            if (columnValue == "false")
                columnValue = "0";

            stripFormatting(columnValue);

            break;
        }
        case DBHtml:
        case DBString:
        {
            if (!noQuote)
                columnValue.setValue(columnValue.toString().replace("'","''"));
            break;
        }
        default:
        {
            break;
        }
    }
}

QVariant SqlQueryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if ( role == Qt::ForegroundRole )
        {
            if (hasUserFilters(section))
                return QCOLOR_BLUE;
        }
        else if ( role == Qt::DisplayRole )
        {
            QVariant val = m_headers.value(section).value(role);

            if (!val.isValid())
                val = m_headers.value(section).value(Qt::EditRole);

            if (val.isValid())
                return val;

            if (m_columnCount > section)
                return m_columnName[section];
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

bool SqlQueryModel::setHeaderData(int section, Qt::Orientation orientation,
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

void SqlQueryModel::stripFormatting(QVariant& value) const
{
    value.setValue(value.toString().replace("$",""));
    value.setValue(value.toString().replace("%",""));
    value.setValue(value.toString().replace(",",""));
}

void SqlQueryModel::reformatValue(QVariant& columnValue, DBColumnType columnType) const
{
    // don't reformat empty values
    if (columnValue.isNull())
        return;

    switch (columnType) {
        case DBHtml:
        case DBString:
        {
            // leave strings alone
            break;
        }
        case DBDate:
        {
            QDateTime datecol;
            datecol.setSecsSinceEpoch(columnValue.toLongLong());

            if ( datecol.isValid() )
            {
                columnValue = datecol.toString("MM/dd/yyyy");
            }
            else
                columnValue.clear();

            break;
        }
        case DBDateTime:
        {
            QDateTime datecol;
            datecol.setSecsSinceEpoch(columnValue.toLongLong());

            if ( datecol.isValid() )
            {
                columnValue = datecol.toString("MM/dd/yyyy hh:mm:ss");
            }
            else
                columnValue.clear();

            break;
        }
        case DBReal:
        {
            stripFormatting(columnValue);
            break;
        }
        case DBInteger:
        case DBBool:
        {
            stripFormatting(columnValue);
            break;
        }
        case DBUSD:
        {
            stripFormatting(columnValue);

            QLocale lc;
            columnValue = lc.toCurrencyString(columnValue.toDouble());

            break;
        }
        case DBPercent:
        {
            stripFormatting(columnValue);

            columnValue = QString::asprintf("%.2f%%",columnValue.toDouble());
            break;
        }
        default:
            break;
    }
}

void SqlQueryModel::addColumn(const QString& columnName, const QString& displayName, DBColumnType type, DBColumnSearchable searchable, DBColumnRequired required, DBColumnEditable editable, DBColumnUnique unique,
                                QStringList* valuelist)
{
    addColumn(columnName, displayName, type, searchable, required, editable, unique);

    m_lookupValues[m_columnCount - 1] = valuelist;
}


void SqlQueryModel::addColumn(const QString& columnName, const QString& displayName, DBColumnType type, DBColumnSearchable searchable, DBColumnRequired required, DBColumnEditable editable, DBColumnUnique unique,
                                const QString& lookupTable, const QString& lookupFkColumnName, const QString& lookupValueColumnName)
{
    setHeaderData(m_columnCount, Qt::Horizontal, displayName);

    m_columnName[m_columnCount] = columnName;
    m_columnType[m_columnCount] = type;
    m_columnIsRequired[m_columnCount] = required;
    m_columnIsSearchable[m_columnCount] = searchable;
    m_columnIsEditable[m_columnCount] = editable;
    m_columnIsUnique[m_columnCount] = unique;

    m_columnIsFiltered[m_columnCount] = false;
    m_filterValue[m_columnCount] = QString();
    m_filterCompareType[m_columnCount]  = DBCompareType::Equals;

    m_isUserFiltered[m_columnCount] = false;
    m_userFilterValues[m_columnCount] = QVariantList();
    m_userSearchString[m_columnCount] = QVariant();

    m_isUserRangeFiltered[m_columnCount] = false;
    m_rangeSearchStart[m_columnCount] = QVariant();
    m_rangeSearchEnd[m_columnCount] = QVariant();

    m_lookupTable[m_columnCount] = lookupTable;
    m_lookupValueColumnName[m_columnCount] = lookupValueColumnName;
    m_lookupFkColumnName[m_columnCount] = lookupFkColumnName;

    m_lookupValues[m_columnCount] = nullptr;

    m_columnNameLookup[columnName] = m_columnCount;

    m_columnCount++;
}

void SqlQueryModel::renameColumn(const int columnNumber, const QString& columnName, const QString& displayName)
{
    if (columnNumber < m_columnCount)
    {
        m_columnNameLookup.remove(m_columnName[columnNumber]);
        m_columnName[columnNumber] = columnName;
        m_columnNameLookup[columnName] = columnNumber;
        setHeaderData(columnNumber, Qt::Horizontal, displayName);
    }
}

int SqlQueryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_cache.size();
}

const QModelIndex SqlQueryModel::copyRecord(QModelIndex index)
{
    QVector<QVariant> newrecord = emptyrecord();
    QString unique_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");

    // don't copy key record so it is identified as a new record
    for (int i = 1; i < m_columnCount; i++)
    {
        if (m_columnIsUnique[i] == DBUnique)
        {
            newrecord[i] = QString(" Copy [%2] of %1").arg(m_cache[index.row()][i].toString(), unique_stamp);
        }
        else
        {
            newrecord[i] = m_cache[index.row()][i];
        }
    }

    return(addRecord(newrecord));
}

const QModelIndex SqlQueryModel::addRecord(QVector<QVariant>& newrecord)
{
    QModelIndex qmi = QModelIndex();
    int row = rowCount((qmi));

    beginInsertRows(qmi, row, row);
    m_cache.append(newrecord);
    endInsertRows();

    return index(row, 0);
}

const QModelIndex SqlQueryModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue1);
    Q_UNUSED(fkValue2);

    QVector<QVariant> qr = emptyrecord();
    return addRecord(qr);
}

void SqlQueryModel::removeCacheRecord(QModelIndex index)
{
    beginRemoveRows(QModelIndex(), index.row(), index.row());

    m_cache.remove(index.row());

    endRemoveRows();
    // beginRemoveRows/endRemoveRows is sufficient to notify views of the removal.
    // Emitting dataChanged for a row that no longer exists is incorrect and removed.
}

bool SqlQueryModel::deleteRecord(QModelIndex index)
{
    if (!deleteCheck(index))
        return false;

    QSqlQuery delrow(getDBOs()->getDb());
    QVariant keyval = m_cache[index.row()][0];
    delrow.prepare("delete from " + m_tablename + " where " + m_columnName[0] + " = ? ");
    delrow.bindValue(0, keyval);

    DB_LOCK;
    if(delrow.exec())
    {
        DB_UNLOCK;

        removeCacheRecord(index);

        deleteRelatedRecords(keyval);
        return true;
    }

    DB_UNLOCK;

    if (m_gui)
    {
        QMessageBox::critical(nullptr, QObject::tr("Cannot delete item"),
           delrow.lastError().text() + "\n" + delrow.lastQuery(), QMessageBox::Ok);
    }

    return false;
}

QVector<QVariant> SqlQueryModel::emptyrecord()
{
    QVector<QVariant> record(m_columnCount);
    return record;
}

bool SqlQueryModel::isUniqueValue(const QVariant &newValue, const QModelIndex &index)
{
    DB_LOCK;
    QString keycolumnname = m_columnName[0];

    QString columnname = m_columnName[index.column()];

    QVariant keyvalue;

    if (m_cache.count() > 0) // if not a new record exclude the current record
        keyvalue = m_cache[index.row()][0].toString();

    QSqlQuery select(getDBOs()->getDb());
    select.prepare("select count(*) from " + m_tablename + " where " + keycolumnname + " <> ? and " + columnname + " = ?");
    select.bindValue(0, keyvalue);
    select.bindValue(1, newValue);

    select.exec();
    if (select.next())
        if (select.value(0).toInt() > 0)
        {
            DB_UNLOCK;
            return false;
        }

    DB_UNLOCK;
    return true;
}

void SqlQueryModel::addRelatedTable(const QString& tableName, const QString& columnName, const QString& fkColumnName, const QString& title, const DBRelationExportable exportable)
{
    QStringList columns;
    QStringList fk_columns;

    columns.append(columnName);
    fk_columns.append(fkColumnName);

    addRelatedTable(tableName, columns, fk_columns, title, exportable);
}

void SqlQueryModel::addRelatedTable(const QString& tableName, const QStringList& columnNames, const QStringList& fkColumnNames, const QString& title, const DBRelationExportable exportable)
{
    m_relatedTableIndex[tableName] = m_relatedTable.size();
    m_relatedTable.append(tableName);
    m_relatedColumns.append(columnNames);
    m_relatedFkColumns.append(fkColumnNames);
    m_relationTitle.append(title);
    m_relationExportable.append(exportable);
}

bool SqlQueryModel::columnChangeCheck(const QModelIndex &index)
{
    int reference_count = 0;
    QString message;
    QStringList key_columns;
    QStringList key_values;
    QString project_number_key;
    QString primary_key;

    for (int i = 0; i < m_relatedTable.size(); ++i)
    {
        int relatedcount = 0;
        bool use_related = false;

        //set the where for all
        QString where_clause;

        for (int c = 0; c < m_relatedColumns[i].count(); c++)
        {
            QString col_name = m_relatedColumns[i].at(c);
            QString fk_col_name = m_relatedFkColumns[i].at(c);

            // look to see if the current column has a related value otherwise dont consider this related table
            if (fk_col_name.compare(m_columnName[index.column()]) == 0)
                use_related = true;

            int col_num = getColumnNumber(fk_col_name);
            QVariant col_val = m_cache[index.row()][col_num].toString();

            sqlEscape(col_val, m_columnType[col_num]);

            if (!where_clause.isEmpty())
                where_clause += " and ";

            where_clause += QString(" %1 = '%2' ").arg(col_name, col_val.toString());

            // special key field mapping
            // check to see if search should be limited by project
            if (fk_col_name.compare("project_id") == 0)
            {
                DB_LOCK;
                QSqlQuery projsql(getDBOs()->getDb());
                projsql.prepare(QString("select project_number from projects where id ='%1'").arg(col_val.toString()));

                projsql.exec();

                if (projsql.next())
                    project_number_key = projsql.value(0).toString();

                DB_UNLOCK;
            }
            else
            {
                primary_key = col_val.toString();
            }
        }

        if (use_related)
        {
            DB_LOCK;
            QSqlQuery select(getDBOs()->getDb());
            select.prepare("select count(*) from " + m_relatedTable.at(i) + " where " + where_clause);

            select.exec();

            //qDebug() << "SET VALUE CHECK: " << "select count(*) from " + m_relatedTable.at(i) + " where " + where_clause;

            if (select.next())
            {
                relatedcount = select.value(0).toInt();

                if (relatedcount > 0)
                {
                    reference_count += relatedcount;

                    message += select.value(0).toString() + " " + m_relationTitle.at(i) + " items(s)\n";
                }
            }

            DB_UNLOCK;
        }
    }

    if (reference_count > 0)
    {
        if (m_gui)
        {
            message = m_displayName + QObject::tr(" is referenced in the following item(s):\n\n") + message +
                     QObject::tr("\nYou cannot change ") + m_displayName + QObject::tr(" until they are no longer assocated with the following items. Would you like to run a search for all related items?");

            if ( QMessageBox::question(nullptr, QObject::tr("Cannot Change Item"),
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

                getDBOs()->searchresultsmodel()->PerformKeySearch( key_columns, key_values );

                emit callKeySearch();
            }
        }

        return false;
    }

    return true;
}

bool SqlQueryModel::deleteCheck(const QModelIndex &index)
{
    int reference_count = 0;
    QString message;
    QStringList key_columns;
    QStringList key_values;
    QString project_number_key;
    QString primary_key;

    for (int i = 0; i < m_relatedTable.size(); ++i)
    {
        int relatedcount = 0;

        //set the where for all
        QString where_clause;

        for (int c = 0; c < m_relatedColumns[i].count(); c++)
        {
            QString col_name = m_relatedColumns[i].at(c);
            QString fk_col_name = m_relatedFkColumns[i].at(c);
            int col_num = getColumnNumber(fk_col_name);
            QVariant col_val = m_cache[index.row()][col_num].toString();

            sqlEscape(col_val, m_columnType[col_num]);

            if (!where_clause.isEmpty())
                where_clause += " and ";

            where_clause += QString(" %1 = '%2' ").arg(col_name, col_val.toString());

            // special key field mapping
            // check to see if search should be limited by project
            if (fk_col_name.compare("project_id") == 0)
            {
                DB_LOCK;
                QSqlQuery projsql(getDBOs()->getDb());
                projsql.prepare(QString("select project_number from projects where id ='%1'").arg(col_val.toString()));

                projsql.exec();

                if (projsql.next())
                    project_number_key = projsql.value(0).toString();

                DB_UNLOCK;
            }
            else
            {
                primary_key = col_val.toString();
            }
        }

        DB_LOCK;

        QSqlQuery select(getDBOs()->getDb());
        select.prepare("select count(*) from " + m_relatedTable.at(i) + " where " + where_clause);
        select.exec();

        //qDebug() << "DELETE CHECK: " << "select count(*) from " + m_relatedTable.at(i) + " where " + where_clause;

        if (select.next())
        {
            relatedcount = select.value(0).toInt();

            if (relatedcount > 0)
            {
                reference_count += relatedcount;

                message += select.value(0).toString() + " " + m_relationTitle.at(i) + " items(s)\n";
            }
        }

        DB_UNLOCK;
    }

    if (reference_count > 0)
    {
        if (m_gui)
        {
            message = m_displayName + QObject::tr(" is referenced in the following:\n\n") + message +
                     QObject::tr("\nYou cannot delete the ") + m_displayName + QObject::tr(" until the assocated items no longer reference it. Would you like to run a search for all related items?");

            if ( QMessageBox::question(nullptr, QObject::tr("Cannot delete item"),
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

                getDBOs()->searchresultsmodel()->PerformKeySearch( key_columns, key_values );

                emit callKeySearch();
            }
        }

        return false;
    }
    else
    {
        if (m_gui)
        {
            if ( QMessageBox::question(nullptr, QObject::tr("Delete item?"),
                QObject::tr("Are you sure you want to delete ") + m_displayName + QObject::tr("?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes )
                return true;
            else
                return false;
        }
        else
            return true;
    }
}

const QVariant SqlQueryModel::findValue(QVariant& lookupValue, int searchColumn, int returnColumn)
{
    for ( QVector<QVector<QVariant>>::Iterator itrow = m_cache.begin(); itrow != m_cache.end(); ++itrow )
    {
        if ( (*itrow)[searchColumn].toString().compare(lookupValue.toString()) == 0 )
        {
            return (*itrow)[returnColumn]; // key is always at 0
        }
    }

    return QVariant();
}

const QModelIndex SqlQueryModel::findIndex(QVariant& lookupValue, int searchColumn)
{
    int row = 0;

    for ( QVector<QVector<QVariant>>::Iterator itrow = m_cache.begin(); itrow != m_cache.end(); ++itrow )
    {
        if ( (*itrow)[searchColumn].toString().compare(lookupValue.toString()) == 0 )
        {
            return index(row, 0); // key is always at 0
        }

        row++;
    }

    return QModelIndex();
}


bool SqlQueryModel::reloadRecord(const QModelIndex& index)
{
    DB_LOCK;
    QSqlQuery select(getDBOs()->getDb());
    select.prepare("SELECT * FROM (" + BaseSQL() + ") _t WHERE _t." + m_columnName[0] + " = ?");
    select.bindValue(0, m_cache[index.row()][0]);

    if (select.exec())
    {
        if (select.next())
        {
            QVector<QVariant> record(m_columnCount);

            for (int i = 0; i < m_columnCount; i++)
                record[i] = select.value(i);

            m_cache[index.row()] = record;

            DB_UNLOCK;

            // Use createIndex() with m_columnCount - 1 so the range is within
            // valid bounds. index.model()->index() was off-by-one (count vs last index)
            // which caused Qt views to silently discard the repaint request.
            emit dataChanged(createIndex(index.row(), 0), createIndex(index.row(), m_columnCount - 1));

            return true;
        }
    }

    DB_UNLOCK;

    return false;
}

QString SqlQueryModel::constructWhereClause(bool includeUserFilter)
{
    QString valuelist;
    QVariant column_value;

    QHashIterator<int, bool> hashit(m_columnIsFiltered);

    // build out the filtered value portion of the where clause
    // filtered values don't consider the value of foreign keys
    // they aren't a filter the user sets in the filter tool
    while (hashit.hasNext())
    {
        hashit.next();

        if (hashit.value()) // has a filter
        {

            column_value = m_filterValue[hashit.key()];

            if (!column_value.isNull())
            {
                if (!valuelist.isEmpty())
                    valuelist += tr(" AND ");

                // In: comma-separated filter value becomes  column IN ('v1','v2',...)
                if (m_filterCompareType[hashit.key()] == DBCompareType::In)
                {
                    const QStringList parts = column_value.toString().split(',', Qt::SkipEmptyParts);
                    QStringList quoted;
                    quoted.reserve(parts.size());
                    for (const QString& part : parts)
                        quoted.append(QString("'%1'").arg(part.trimmed().replace("'", "''")));
                    valuelist += QString("%1 IN (%2)").arg(m_columnName[hashit.key()], quoted.join(','));
                }
                else
                {
                    QString compare_op;

                    switch (m_filterCompareType[hashit.key()])
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
                    case DBCompareType::Like:
                        compare_op = "LIKE";
                        break;
                    default:
                        compare_op = "=";
                    }

                    sqlEscape(column_value, m_columnType[hashit.key()]);

                    if ( m_columnType[hashit.key()] != DBString && m_columnType[hashit.key()] != DBHtml)
                    {
                        if (m_columnType[hashit.key()] == DBBool && column_value.toString().compare("0") == 0)
                        {
                            valuelist += QString(" ( %1 %3 %2").arg(m_columnName[hashit.key()], column_value.toString(), compare_op);
                            valuelist += QString(" OR %1 IS NULL) ").arg( m_columnName[hashit.key()] );
                        }
                        else
                            valuelist += QString("%1 = %2").arg( m_columnName[hashit.key()], column_value.toString() );
                    }
                    else
                    {
                        sqlEscape(column_value, m_columnType[hashit.key()]);
                        valuelist += QString("%1 %3 '%2'").arg( m_columnName[hashit.key()], column_value.toString(), compare_op);
                    }
                }
            }
        }
    }

    // if the user filter is set to active and the function caller wan't to include user filters specified
    // in the filter tool then add them to the where clause
    if (m_userFilterActive && includeUserFilter)
    {

        // qDebug() << "m_isUserFiltered count is " << m_isUserFiltered.count();

        // iterate through all the fields and apply the user filters if they have them
        // user search strings will search a foreign key value and not the foreign key id
        QHashIterator<int, bool> hashitsrch(m_isUserFiltered);
        while (hashitsrch.hasNext())
        {
            hashitsrch.next();

            int colnum = hashitsrch.key();

            // qDebug() << " -- key() found was " << colnum << " value() found was " << hashitsrch.value();

            if ( m_isUserFiltered[colnum] || m_isUserRangeFiltered[colnum] ) // if has a user filter then add the various kinds
            {
                // if the column has a sub string search them apply it
                if ( !m_userSearchString[colnum].isNull() && !(m_userSearchString[colnum].toString().isEmpty()))
                {
                    column_value = m_userSearchString[colnum];

                    sqlEscape(column_value, m_columnType[colnum]);

                    if (!valuelist.isEmpty())
                        valuelist += tr(" AND ");


                    if (  m_columnType[colnum] == DBString ||  m_columnType[colnum] == DBHtml )
                    {
                        if ( !m_lookupTable[colnum].isEmpty() )
                        {
                            QString fk_key_val =  m_userSearchString[colnum].toString();
                            valuelist +=  QString(" %5 in (select %1 from %2 where %3 LIKE '%%4%')").arg(m_lookupFkColumnName[colnum], m_lookupTable[colnum], m_lookupValueColumnName[colnum], fk_key_val, m_columnName[colnum]);
                        }
                        else
                            valuelist += QString(" %1 LIKE '%%2%' ").arg(m_columnName[colnum], column_value.toString());
                    }
                    else
                    {
                        if ( !m_lookupTable[colnum].isEmpty() )
                        {
                            QString fk_key_val =  m_userSearchString[colnum].toString();
                            valuelist +=  QString(" %5 in (select %1 from %2 where %3 = '%%4%')").arg(m_lookupFkColumnName[colnum], m_lookupTable[colnum], m_lookupValueColumnName[colnum], fk_key_val, m_columnName[colnum]);
                        }
                        else
                            valuelist += QString(" %1 = %2 ").arg(m_columnName[colnum], column_value.toString());
                    }
                }

                // if column has a list of values to filter add them to the where clause
                QVariantList& ColumnValues = m_userFilterValues[colnum];
                QString instring;
                bool checkfornullptr = false;

                for ( const auto& colval : ColumnValues)
                {
                    if (!instring.isEmpty())
                        instring += tr(", ");

                    column_value = colval;

                    sqlEscape(column_value, m_columnType[colnum]);

                    if ( m_columnType[colnum] == DBString || m_columnType[colnum] == DBHtml )
                        instring += QString("'%1'").arg(column_value.toString());
                    else
                        instring += QString("%1").arg(column_value.toString());

                    if (m_columnType[colnum] == DBBool && column_value.toString().compare("'0'") == 0)
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
                        valuelist += QString(" ( %1 IN (%2)").arg(m_columnName[hashitsrch.key()], instring);
                        valuelist += QString(" OR %1 IS NULL) ").arg(m_columnName[hashitsrch.key()]);
                    }
                    else
                        valuelist += QString(" %1 IN (%2) ").arg(m_columnName[hashitsrch.key()], instring);
                }

                // and any range filters that have been included into the where clause
                if (m_isUserRangeFiltered[colnum])
                {
                    QVariant RangeStart(m_rangeSearchStart[colnum]);
                    QVariant RangeEnd(m_rangeSearchEnd[colnum]);

                    sqlEscape(RangeStart, m_columnType[colnum]);
                    sqlEscape(RangeEnd, m_columnType[colnum]);

                    // if the range is searching accross a foreign key value then
                    // search and return a list of foreign key ids that apply
                    // search foreign key value if exists otherwise search for the value in the field

                    if ( !m_lookupTable[colnum].isEmpty() )
                    {
                        QString valuerange;

                        // if the column doesn't have a lookup value in a foreign key
                        // you can do the range search on the value
                        if (!RangeStart.isNull() && RangeStart != tr("''"))
                        {
                            if ( m_columnType[colnum] != DBString && m_columnType[colnum] != DBHtml)
                            {
                                valuerange += QString("%1 >= %2").arg(m_lookupValueColumnName[colnum], RangeStart.toString());
                            }
                            else
                            {
                                valuerange += QString("%1 >= '%2'").arg(m_lookupValueColumnName[colnum], RangeStart.toString());
                            }
                        }

                        if (!RangeEnd.isNull() && RangeEnd != tr("''"))
                        {
                            if (!valuerange.isEmpty())
                                valuerange += tr(" AND ");

                            if ( m_columnType[colnum] != DBString && m_columnType[colnum] != DBHtml)
                            {
                                valuerange += QString("%1 <= %2").arg(m_lookupValueColumnName[colnum], RangeEnd.toString());
                            }
                            else
                            {
                                valuerange += QString("%1 <= '%2'").arg(m_lookupValueColumnName[colnum], RangeEnd.toString());
                            }
                        }

                        if (!valuelist.isEmpty())
                            valuelist += tr(" AND ");

                        valuelist +=  QString(" %4 in (select %1 from %2 where %3)").arg(m_lookupFkColumnName[colnum], m_lookupTable[colnum], valuerange, m_columnName[colnum]);
                    }
                    else
                    {
                        // if the column doesn't have a lookup value in a foreign key
                        // you can do the range search on the value
                        if (!RangeStart.isNull() && RangeStart != tr("''"))
                        {
                            if (!valuelist.isEmpty())
                                valuelist += tr(" AND ");

                            if ( m_columnType[colnum] != DBString && m_columnType[colnum] != DBHtml)
                            {
                                valuelist += QString("%1 >= %2").arg(m_columnName[colnum], RangeStart.toString());
                            }
                            else
                            {
                                valuelist += QString("%1 >= '%2'").arg(m_columnName[colnum], RangeStart.toString());
                            }
                        }

                        if (!RangeEnd.isNull() && RangeEnd != tr("''"))
                        {
                            if (!valuelist.isEmpty())
                                valuelist += tr(" AND ");


                            if ( m_columnType[colnum] != DBString && m_columnType[colnum] != DBHtml)
                            {
                                valuelist += QString("%1 <= %2").arg(m_columnName[colnum], RangeEnd.toString());
                            }
                            else
                            {
                                valuelist += QString("%1 <= '%2'").arg(m_columnName[colnum], RangeEnd.toString());
                            }
                        }
                    }
                }
            }
        }
    }

    if (!valuelist.isEmpty())
        valuelist = QString(" WHERE %1 COLLATE NOCASE ").arg(valuelist);

    //qDebug() << "WHERE CLAUSE: " << valuelist;

    return valuelist;
}

void SqlQueryModel::setFilter(int columnNumber, const QString& filterValue, DBCompareType compareType)
{
    m_filterValue[columnNumber] = filterValue;
    m_columnIsFiltered[columnNumber] = true;
    m_filterCompareType[columnNumber] = compareType;
}

QVariant SqlQueryModel::getFilter(int columnNumber)
{
    if (!m_columnIsFiltered[columnNumber])
        return QVariant();

    return(QVariant(m_filterValue[columnNumber]));
}



void SqlQueryModel::clearAllFilters()
{
    QHashIterator<int, bool> hashit(m_columnIsFiltered);

    while (hashit.hasNext())
    {
        hashit.next();
        clearFilter(hashit.key());
    }
}

void SqlQueryModel::clearFilter(int columnNumber)
{
    m_columnIsFiltered[columnNumber] = false;
    m_filterValue[columnNumber].clear();
    m_filterCompareType[columnNumber] = DBCompareType::Equals;
}

void SqlQueryModel::setUserFilter(int columnNumber, const QVariantList& filterValues)
{
    m_isUserFiltered[columnNumber] = true;
    m_userFilterValues[columnNumber] = filterValues;
}

void SqlQueryModel::setUserSearchString(int columnNumber, const QVariant& searchValue)
{
    m_isUserFiltered[columnNumber] = true;
    m_userSearchString[columnNumber] = searchValue;
}

void SqlQueryModel::setUserSearchRange(int columnNumber, const QVariant& searchBeginValue, const QVariant& searchEndValue )
{
    m_isUserRangeFiltered[columnNumber] = true;
    m_rangeSearchStart[columnNumber] = searchBeginValue;
    m_rangeSearchEnd[columnNumber] = searchEndValue;
}

void SqlQueryModel::getUserSearchRange(int columnNumber, QVariant& searchBeginValue, QVariant& searchEndValue )
{
    if (m_rangeSearchStart.contains(columnNumber))
    {
        searchBeginValue = m_rangeSearchStart[columnNumber];
        searchEndValue = m_rangeSearchEnd[columnNumber];
    }
}

void SqlQueryModel::clearAllUserSearches()
{
    QHashIterator<int, bool> hashit(m_isUserFiltered);

    while (hashit.hasNext())
    {
        hashit.next();
        clearUserFilter(hashit.key());
        clearUserSearchString(hashit.key());
        clearUserSearchRange(hashit.key());
    }
}

void SqlQueryModel::clearUserFilter(int columnNumber)
{
    if (m_isUserFiltered.contains(columnNumber))
    {
        m_isUserFiltered[columnNumber] = false;
        m_userFilterValues[columnNumber].clear();
    }
}

void SqlQueryModel::clearUserSearchString(int columnNumber)
{
    if (m_userSearchString.contains(columnNumber))
        m_userSearchString[columnNumber].clear();
}

void SqlQueryModel::clearUserSearchRange(int columnNumber)
{
    if (m_isUserRangeFiltered.contains(columnNumber))
    {
        m_isUserRangeFiltered[columnNumber] = false;
        m_rangeSearchStart[columnNumber].clear();
        m_rangeSearchEnd[columnNumber].clear();
    }
}

bool SqlQueryModel::hasUserFilters(int columnNumber) const
{
    if (m_isUserRangeFiltered[columnNumber] || m_isUserFiltered[columnNumber] )
        return true;
    else
        return false;
}

bool SqlQueryModel::hasUserFilters() const
{
    QHashIterator<int, bool> hashit(m_columnIsFiltered);

    while (hashit.hasNext())
    {
        hashit.next();
        if ( hasUserFilters(hashit.key()) )
            return true;
    }

    return false;
}

void SqlQueryModel::activateUserFilter(const QString& filterName)
{
    m_userFilterActive = true;
    refresh();

    if (!filterName.isEmpty())
    {
        QString filter_name = filterName;
        filter_name.replace(" ", "_", Qt::CaseSensitive);

        QString parmname;
        QString val = "true";

        parmname = QString("UserFilter:%1:IsActive").arg(filter_name);

        getDBOs()->saveParameter(parmname, val);
    }
}

void SqlQueryModel::deactivateUserFilter(const QString& filterName)
{
    m_userFilterActive = false;
    refresh();

    if (!filterName.isEmpty())
    {
        QString filter_name = filterName;
        filter_name.replace(" ", "_", Qt::CaseSensitive);

        QString parmname;
        QString val = "false";

        parmname = QString("UserFilter:%1:IsActive").arg(filter_name);

        getDBOs()->saveParameter(parmname, val);
    }
}

void SqlQueryModel::loadLastUserFilterState(const QString& filterName)
{
    QString filter_name = filterName;
    filter_name.replace(" ", "_", Qt::CaseSensitive);

    QString parmname;
    QString val;

    parmname = QString("UserFilter:%1:IsActive").arg(filter_name);
    parmname.replace(" ", "_", Qt::CaseSensitive);

    val = getDBOs()->loadParameter(parmname);

    if (val.compare("true") == 0)
        m_userFilterActive = true;
    else
        m_userFilterActive = false;
}

void SqlQueryModel::saveUserFilter(const QString& filterName)
{
    QString filter_name = filterName;
    filter_name.replace(" ", "_", Qt::CaseSensitive);

    QDomDocument doc;
    QDomElement root = doc.createElement(filter_name);
    doc.appendChild(root);

    QDomElement child;
    QDomElement columnvalue;

    root.setAttribute("ObjectType", "UserFilter");

    for (auto it = m_columnType.cbegin(); it != m_columnType.cend(); ++it)
    {
        child = doc.createElement(m_columnName[it.key()]);
        child.setAttribute("ObjectType", "Field");
        child.setAttribute("RangeSearchStart", m_rangeSearchStart[it.key()].toString());
        child.setAttribute("RangeSearchEnd", m_rangeSearchEnd[it.key()].toString());
        child.setAttribute("UserSearchString", m_userSearchString[it.key()].toString());
        child.setAttribute("FieldNumber", it.key());

        root.appendChild(child);

        int i = 0;

        for (auto ait = m_userFilterValues[it.key()].cbegin(); ait != m_userFilterValues[it.key()].cend(); ++ait)
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

    getDBOs()->saveParameter( parmname, xml );
}

void SqlQueryModel::loadUserFilter(const QString& filterName)
{
    QString filter_name = filterName;
    filter_name.replace(" ", "_", Qt::CaseSensitive);

    QString parmname;

    parmname = QString("UserFilter:%1").arg(filter_name);
    parmname.replace(" ", "_", Qt::CaseSensitive);

    QString xml = getDBOs()->loadParameter(parmname);

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

        m_rangeSearchStart[field] = child.toElement().attribute("RangeSearchStart");
        m_rangeSearchEnd[field] = child.toElement().attribute("RangeSearchEnd");
        m_userSearchString[field] = child.toElement().attribute("UserSearchString");

        if (!m_rangeSearchStart[field].toString().isEmpty() || !m_rangeSearchEnd[field].toString().isEmpty() )
            m_isUserRangeFiltered[field] = true;
        else
            m_isUserRangeFiltered[field] = false;

        m_userFilterValues[field].clear();

        if (m_userSearchString[field].toString().isEmpty())
            m_isUserFiltered[field] = false;
        else
            m_isUserFiltered[field] = true;

        subchild = child.firstChild();

        while (!subchild.isNull())
        {
            m_userFilterValues[field].append(subchild.toElement().attribute("SearchValue"));
            m_isUserFiltered[field] = true;

            subchild = subchild.nextSibling();
        }

        child = child.nextSibling();
    }
}

QString SqlQueryModel::getColumnName( QString& displayName )
{
    for ( int i = 0; i < m_headers.size(); i++ )
    {
        if ( m_headers[i][Qt::EditRole].toString().compare(displayName) == 0 )
            return m_columnName[i];
    }

    return QString();
}

int SqlQueryModel::getColumnNumber(const QString& fieldName)
{
    auto it = m_columnNameLookup.find(fieldName);
    if (it != m_columnNameLookup.end())
        return it.value();
    return -1;
}

QString SqlQueryModel::removeInvalidXmlCharacters(const QString &input)
{
    QString result;
    result.reserve(input.size());

    for (QChar c : input) {
        uint ucs4 = c.unicode();
        // Valid XML characters are:
        // U+0009, U+000A, U+000D, U+0020 to U+D7FF, U+E000 to U+FFFD, U+10000 to U+10FFFF
        if (ucs4 == 0x0009 || ucs4 == 0x000A || ucs4 == 0x000D ||
            (ucs4 >= 0x0020 && ucs4 <= 0xD7FF) ||
            (ucs4 >= 0xE000 && ucs4 <= 0xFFFD))
        {
            result.append(c);
        } else if (ucs4 >= 0x10000 && ucs4 <= 0x10FFFF) {
            result.append(c);
        }
    }

    return result;
}

QDomElement SqlQueryModel::toQDomElement( QDomDocument* xmlDocument, const QString& filter )
{
    // if there is a filter let's apply it
    if (!filter.isEmpty())
        if (!filter.contains(this->tablename(), Qt::CaseInsensitive))
            return QDomElement();

    // some data models are just for drop downs
    if (!isExportable())
        return QDomElement();

    QDomElement xmltable = xmlDocument->createElement("table");
    xmltable.toElement().setAttribute("name", this->tablename());

    for ( const auto& row : m_cache )
    {
        QDomElement xmlrow = xmlDocument->createElement("row");

        // build the column xml
        for ( int i = 0; i < row.count(); i++ )
        {
            QDomElement xmlcolumn = xmlDocument->createElement("column");
            xmlcolumn.setAttribute("name", m_columnName[i]);

            QVariant val = row.value(i);
            reformatValue(val, getType(i));

            if (getType(i) == DBHtml)
            {
                // need a specific type here
                QDomCDATASection xmlcdata = xmlDocument->createCDATASection(removeInvalidXmlCharacters(val.toString()));
                xmlcolumn.appendChild(xmlcdata);
            }
            else
            {
                QDomText xmltext = xmlDocument->createTextNode(removeInvalidXmlCharacters(val.toString()));
                xmlcolumn.appendChild(xmltext);
            }

            if ( !m_lookupTable[i].isEmpty() )
            {
                QString fk_key_val = row.value(i).toString();
                QString sql = QString("select %1 from %2 where %3 = '%4'").arg(m_lookupValueColumnName[i], m_lookupTable[i], m_lookupFkColumnName[i], fk_key_val);
                QString lookup_value = getDBOs()->execute(sql);

                if (!lookup_value.isEmpty())
                    xmlcolumn.setAttribute("lookupvalue", lookup_value);
            }

            //qDebug() << "column: " << m_columnName[i] << " table: " << this->tablename() << " value: " << val.toString();

            xmlrow.appendChild(xmlcolumn);
        }

        // append related tables into the row

        // for loop through a list of related tables
        for (int i = 0; i < m_relatedTable.count(); i++)
        {
            // not all related items are exportable
            // it would be too much data
            if (m_relationExportable[i] == DBExportable)
            {
                // create an export version of that querymodel
                SqlQueryModel* export_version = m_dbo->createExportObject(m_relatedTable[i]);

                //set the filter for the export version
                for (int c = 0; c < m_relatedColumns[i].count(); c++)
                {
                    QString col_name = m_relatedColumns[i].at(c);
                    QString fk_col_name = m_relatedFkColumns[i].at(c);

                    int col = export_version->getColumnNumber(col_name);
                    int fkcol = getColumnNumber(fk_col_name);

                    export_version->setFilter(col, row.value(fkcol).toString());
                }

                export_version->refresh();

                QDomElement qd = export_version->toQDomElement( xmlDocument, filter );

                for (int c = 0; c < m_relatedColumns[i].count(); c++)
                {
                    QString col_name = m_relatedColumns[i].at(c);
                    QString fk_col_name = m_relatedFkColumns[i].at(c);

                    int fkcol = getColumnNumber(fk_col_name);

                    qd.setAttribute(QString("filter_field_%1").arg(c + 1), col_name);
                    qd.setAttribute(QString("filter_value_%1").arg(c + 1), row.value(fkcol).toString());
                }

                // add the new XML to the current row
                xmlrow.appendChild(qd);

                delete export_version;
            }
        }

        xmltable.appendChild(xmlrow);
    }

    return xmltable;
}

bool SqlQueryModel::setFilter(QDomNode& xmlfilter)
{
    // Iterate over attributes
    QDomNamedNodeMap attributes = xmlfilter.attributes();

    if (!attributes.namedItem("skip").isNull())
        m_skip = attributes.namedItem("skip").nodeValue().toULong();

    if (!attributes.namedItem("top").isNull())
        m_top = attributes.namedItem("top").nodeValue().toULong();

    for (int i = 0; i < attributes.count(); ++i) {
        QDomNode attribute = attributes.item(i);

        if (attribute.nodeName().startsWith("filter_field_"))
        {
            QString field_name = attribute.nodeValue();
            QString field_filter_val = attribute.nodeName().replace("filter_field_", "filter_value_");

            if (!attributes.namedItem(field_filter_val).isNull())
            {
                QString filter_value = attributes.namedItem(field_filter_val).nodeValue();

                int c = getColumnNumber(field_name);
                if (c != -1)
                    setFilter(c, filter_value, DBCompareType::Like);
            }
        }
    }

    return true;
}

bool SqlQueryModel::importXMLNode(const QDomNode& domnode)  // this should be table level
{
    QDomElement node = domnode.firstChildElement("row");

    while (!node.isNull())
    {
        if (!setData(&node, false))
            return false; // jump out if we have an error

        node = node.nextSiblingElement("row");
    }

    return true;
}

bool SqlQueryModel::setData(QDomElement* xmlRow, bool ignoreKey)
{
    if (xmlRow->tagName() != "row")
    {
        //qDebug() << "tag name: " << xmlRow->tagName() << " is not a 'row'";
        return false;
    }

    bool isdelete = (xmlRow->attribute("delete").compare("true", Qt::CaseInsensitive) == 0);

    QString whereclause;
    QString fields;
    QString updatevalues;
    QString insertvalues;
    QString keyfield = getColumnName(0);
    QString keyvalue;

    // determine if identifier should be used
    if (!ignoreKey)
    {
        keyvalue = xmlRow->attribute("id");

        if (!keyvalue.isNull())
        {
            whereclause = QString(" %1 = '%2'").arg(keyfield, keyvalue);
        }
    }

    // if using keys don't check for unique values
    if (keyvalue.isNull())
    {
        // Loop through unique keys to find one that can be used to identify a record without the record id
        for (const QStringList& uk : m_uniqueKeys)
        {
            // loop key fields assumming they are there
            int found_count = 0;
            QString temp_where;

            for (const QString& kf : uk)
            {
                // CHECK XML FOR COLUMNS
                QDomNode element = xmlRow->firstChild();
                while (!element.isNull())
                {
                    if (element.toElement().tagName().compare("column") == 0)
                    {
                        QString field_name = element.toElement().attribute("name");
                        QVariant field_value = element.toElement().text();
                        QString lookup_value = element.toElement().attribute("lookupvalue");

                        int colnum = getColumnNumber(field_name);

                        // if column has a lookup value, look up the key value
                        if (!lookup_value.isNull() && !m_lookupTable[colnum].isEmpty())
                        {
                            QString sql = QString("select %1 from %2 where %3 = '%4'").arg(m_lookupFkColumnName[colnum], m_lookupTable[colnum], m_lookupValueColumnName[colnum], lookup_value);
#ifdef QT_DEBUG
                            QLog_Debug(DEBUGLOG, QString("EXEC LOOKUP EXISTING: %1").arg(sql));
#endif

                            field_value = getDBOs()->execute(sql);
                        }

                        // if this is a key field add to temp where clause
                        if (field_name.compare(kf, Qt::CaseInsensitive) == 0)
                        {
                            sqlEscape(field_value, m_columnType[colnum], false);

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

    QDomNode element = xmlRow->firstChild();
    while (!element.isNull())
    {
        if (element.toElement().tagName().compare("column") == 0)
        {
            QString field_name = element.toElement().attribute("name");
            QVariant field_value = element.toElement().text();
            QString lookup_value = element.toElement().attribute("lookupvalue");

            int colnum = getColumnNumber(field_name);

            // if the keyval was included set it
            if (colnum == 0)
            {
                keyvalue = field_value.toString();
                whereclause = QString(" %1 = '%2'").arg(keyfield, keyvalue);
            }

            // if key column is specified blank or null don't use it
            // don't use if it isn't an editable column
            if ( !(colnum == 0 && (field_value.isNull() || field_value.toString().isEmpty())) &&
                 isEditable(colnum) )
            {
                if (!fields.isEmpty())
                    fields += ",";

                fields += field_name;

                // if column has a lookup value, look up the key value
                if (!lookup_value.isNull() && !m_lookupTable[colnum].isEmpty())
                {
                    QString sql = QString("select %1 from %2 where %3 = '%4'").arg(m_lookupFkColumnName[colnum], m_lookupTable[colnum], m_lookupValueColumnName[colnum], lookup_value);

                    field_value = getDBOs()->execute(sql);
                }

                if (!insertvalues.isEmpty())
                    insertvalues += ",";


                if (!updatevalues.isEmpty())
                    updatevalues += ",";

                // if list of values doesn't contain this value the record need rejected
                if (m_lookupValues[colnum] && !m_lookupValues[colnum]->contains(field_value.toString(), Qt::CaseSensitive))
                {
                    if (m_gui)
                        QMessageBox::critical(nullptr, QObject::tr("Invalid Field Value"), QString("""%1"" is not a valid field value.").arg(field_value.toString()));

                    return false;
                }

                sqlEscape(field_value, m_columnType[colnum], false);

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
    QString exists_sql = QString("select %3 from %1 where %2").arg(m_tablename, whereclause, getColumnName(0));
    QString id_field = getDBOs()->execute(exists_sql);
    QString sql;
    KeyColumnChange::OperationType disp_optype;

    if (!id_field.isEmpty())
    {
        // delete or update if exists
        if (isdelete)
        {
            sql = QString("delete from %1 where %3").arg(m_tablename, whereclause);
            disp_optype = KeyColumnChange::Delete;
        }
        else
        {
            sql = QString("update %1 set %2 where %3").arg(m_tablename, updatevalues, whereclause);
            disp_optype = KeyColumnChange::Update;
        }
    }
    else
    {
        // insert if it doesn't exist
        // needs an ID since one won't exist
        if (!fields.isEmpty())
            fields += ",";

        QString keycol = getColumnName(0); // get the key field name
        fields += keycol;

        if (!insertvalues.isEmpty())
            insertvalues += ",";

        if (keyvalue.isNull())
            keyvalue = QUuid::createUuid().toString();

        id_field = keyvalue;

        insertvalues += QString("'%1'").arg(keyvalue);

        disp_optype = KeyColumnChange::Insert;

        sql = QString("insert into %1 (%2) values (%3)").arg(m_tablename, fields, insertvalues);
    }

#ifdef QT_DEBUG
    QLog_Debug(DEBUGLOG, QString("XML Generated SQL:%1 ").arg(sql));
#endif

    getDBOs()->execute(sql);

    getDBOs()->pushRowChange(tablename(), id_field, disp_optype);  // track change to update display later

    return true;
}

bool SqlQueryModel::checkUniqueKeys(const QModelIndex &index, const QVariant &value)
{
    QString checkfield = m_columnName[index.column()];

    QHash<QString, QStringList>::iterator itk;

    for ( itk = m_uniqueKeys.begin(); itk != m_uniqueKeys.end(); ++itk  )
    {
        QString where;
        bool isrelevent = false;

        for (const QString& f : itk.value())
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

            DB_LOCK;
            QSqlQuery qry(getDBOs()->getDb());
            qry.prepare(where);

            for (const QString& f : itk.value())
            {
                if ( f.compare(checkfield) == 0 )
                {
                    //qDebug() << "binding...  " << value;
                    qry.addBindValue(value); // check the value we want to update with
                }
                else
                {
                    //qDebug() << "binding... " << m_cache[index.row()].field(f).value();
                    int c = getColumnNumber(f);
                    qry.addBindValue(m_cache[index.row()][c]);
                }
            }


            qry.exec();


            if (qry.next())
            {

                if ( qry.value(0).toInt() > 0)
                {
                    DB_UNLOCK;

                    if (m_gui)
                    {
                        QMessageBox::warning(nullptr, QObject::tr("Cannot update record"),
                           QString("%1 must be unique.").arg(itk.key()));
                    }

                    return false;
                }
            }

            DB_UNLOCK;
        }
    }

    return true;
}

bool SqlQueryModel::matchesFilter(int column, const QVariant& value)
{
    if (hasFilter(column))
    {
        if (m_filterCompareType[column] == DBCompareType::In)
        {
            const QStringList parts = m_filterValue[column].toString().split(',', Qt::SkipEmptyParts);
            bool found = false;
            for (const QString& part : parts)
            {
                if (value.toString().trimmed().compare(part.trimmed(), Qt::CaseInsensitive) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }
        else if (getFilter(column) != value)
            return false;
    }

    if (hasUserFilters(column))
    {
        QVariantList vallist = getUserFilter(column);
        bool matches = false;

        for (const QVariant& v : vallist)
        {
            if (value.toString().contains(v.toString(), Qt::CaseInsensitive))
                matches = true;
        }

        if (!matches)
            return false;
    }

    return true;
}

bool SqlQueryModel::loadAndFilterRow(const QVariant& id)  // load the record and see if it matches filter
{
    QVector<QVariant> newrecord = emptyrecord();

    DB_LOCK;
    QSqlQuery select(getDBOs()->getDb());
    select.prepare("SELECT * FROM (" + BaseSQL() + ") _t WHERE _t." + m_columnName[0] + " = ?");
    select.bindValue(0, id);

    if (select.exec())
    {
        if (select.next())
        {
            for (int i = 0; i < m_columnCount; i++)
                newrecord[i] = select.value(i);
        }
    }

    DB_UNLOCK;

    int c = columnCount();

    while (c--)
    {
        if (!matchesFilter(c, newrecord[c]))
            return false;
    }

    addRecord(newrecord);

    return true;
}

bool SqlQueryModel::copyAndFilterRow(QModelIndex& qmi, SqlQueryModel& pnmodel)
{
    QVector<QVariant> newrecord = emptyrecord();

    int c = pnmodel.columnCount();

    // qDebug() << "copy from " << pnmodel.tablename() << " : " << pnmodel.objectName() << " to " << tablename() << " : " << objectName();

    // copy to columns of a simialar name
    // this function is used assuming it is the same table, that's why it copies the id
    while (c--)
    {
        QString sourcename = pnmodel.getColumnName(c);
        int dc = getColumnNumber(sourcename);

        // qDebug() << "copy from column " << sourcename;

        if (dc >= 0)
        {
            QVariant colval = pnmodel.data(pnmodel.index(qmi.row(), c));

            // qDebug() << " - found matching column " << sourcename;

            if (!matchesFilter(dc, colval))
                return false;

            newrecord[dc] = colval;
        }
    }

    addRecord(newrecord);

    return true;
}

void SqlQueryModel::deleteRelatedRecords(QVariant& keyval)
{
    QListIterator<SqlQueryModel*> it_recordsets(m_dbo->getOpenModels());
    SqlQueryModel* recordset = nullptr;

    // look through all recordsets that are open
    while(it_recordsets.hasNext())
    {
        recordset = it_recordsets.next();

        if ( recordset != this)
        {
            if ( recordset->tablename().compare(tablename()) == 0 )
            {
                QModelIndex qmi = recordset->findIndex(keyval, 0);
                if (qmi.isValid())
                {
                    recordset->removeCacheRecord(qmi);
                    // qDebug() << "Removed corresponding record";
                }
            }
        }
    }
}
