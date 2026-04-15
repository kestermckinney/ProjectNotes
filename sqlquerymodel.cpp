// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
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
                        while (qmi.isValid())
                        {
                            recordset->reloadRecord(qmi);
                            qmi = recordset->findNextIndex(val, ck_col, qmi);
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

    DB_LOCK;
    getDBOs()->getDb().transaction();
    sql_query = QSqlQuery( getDBOs()->getDb() );
    sql_query.setForwardOnly(true);
    sql_query.prepare(fullsql);
    if (!sql_query.exec())
    {
        QLog_Error(ERRORLOG,QString("%1 SQL QUERY FAILED: %2\nSQL: %3").arg(objectName(), sql_query.lastError().text(), fullsql));
    }
#ifdef QT_DEBUG
    else
    {
        QLog_Debug(DEBUGLOG,QString("SQL QUERY EXECUTED SUCCESSFUL: %1\nSQL: %2").arg(objectName(), fullsql));
    }
#endif

    // add a blank row for drop downs
    if (m_showBlank)
    {
        m_cache.append(emptyrecord());
    }

    while (sql_query.next())
    {
        QVector<QVariant> record(m_columnCount);

        for (int i = 0; i < m_columnCount && i < sql_query.record().count(); i++)
            record[i] = sql_query.value(i);

        m_cache.append(record);
    }
    getDBOs()->getDb().commit();
    DB_UNLOCK;

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

    // Named column roles for QML delegates: Qt::UserRole + colIndex → column value.
    // This lets QML access any column via model.columnName without calling data() explicitly.
    if (role >= Qt::UserRole && index.row() >= 0 && index.row() < m_cache.size()) {
        int col = role - Qt::UserRole;
        if (col >= 0 && col < m_columnCount) {
            retval = m_cache[index.row()][col];
            reformatValue(retval, m_columnType[col]);
            return retval;
        }
    }

    // make a light gray backround when not edit_table
    if (m_cache.size() > index.row() && role == Qt::BackgroundRole && index.row() >= 0)
    {
        if (m_columnIsEditable[index.column()] == DBReadOnly)
        {
            QColor base = QApplication::palette().color(QPalette::Base);
            retval = QVariant(base.lightness() > 128
                ? base.darker(115)
                : QApplication::palette().color(QPalette::Button));
        }
    }

    return retval;
}

QHash<int, QByteArray> SqlQueryModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractTableModel::roleNames();
    for (auto it = m_columnName.constBegin(); it != m_columnName.constEnd(); ++it)
        roles[Qt::UserRole + it.key()] = it.value().toUtf8();
    return roles;
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
        case DBPhoneNumber:
        {
            QString raw = columnValue.toString();

            // if the value contains any letters, store as entered (supports extensions, word-based numbers)
            bool hasLetters = false;
            for (const QChar& c : raw)
            {
                if (c.isLetter())
                {
                    hasLetters = true;
                    break;
                }
            }

            if (hasLetters)
            {
                if (!noQuote)
                    columnValue.setValue(raw.replace("'","''"));
                else
                    columnValue.setValue(raw);
                break;
            }

            // strip formatting characters, keep digits and leading +
            QString normalized;
            QString trimmed = raw.trimmed();
            int start = 0;
            if (!trimmed.isEmpty() && trimmed[0] == '+')
            {
                normalized = "+";
                start = 1;
            }

            for (int i = start; i < trimmed.length(); i++)
            {
                if (trimmed[i].isDigit())
                    normalized += trimmed[i];
            }

            if (!noQuote)
                normalized.replace("'","''");

            columnValue.setValue(normalized);
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
        case DBPhoneNumber:
        {
            QString stored = columnValue.toString();

            // if value contains letters (word-based numbers, extensions like "x123"), display as-is
            bool hasLetters = false;
            for (const QChar& c : stored)
            {
                if (c.isLetter())
                {
                    hasLetters = true;
                    break;
                }
            }
            if (hasLetters)
                break;

            bool international = stored.startsWith('+');
            QString digits = international ? stored.mid(1) : stored;

            if (digits.length() == 10)
            {
                // (XXX) XXX-XXXX
                columnValue = QString("(%1) %2-%3")
                    .arg(digits.mid(0, 3))
                    .arg(digits.mid(3, 3))
                    .arg(digits.mid(6, 4));
            }
            else if (digits.length() == 11 && digits[0] == '1')
            {
                // +1 (XXX) XXX-XXXX
                columnValue = QString("+1 (%1) %2-%3")
                    .arg(digits.mid(1, 3))
                    .arg(digits.mid(4, 3))
                    .arg(digits.mid(7, 4));
            }
            else if (international && digits.length() > 7)
            {
                // Generic international: +CC XXXXXXXXX
                int ccLen = (digits.length() <= 9) ? 1 : (digits.length() <= 11) ? 2 : 3;
                columnValue = QString("+%1 %2")
                    .arg(digits.mid(0, ccLen))
                    .arg(digits.mid(ccLen));
            }
            // else leave as-is (unrecognized format)
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

    // don't copy key record so it is identified as a new record
    for (int i = 1; i < m_columnCount; i++)
    {
        if (m_columnIsUnique[i] == DBUnique)
        {
            newrecord[i] = QString("Copy of %1").arg(m_cache[index.row()][i].toString());
        }
        else
        {
            newrecord[i] = m_cache[index.row()][i];
        }
    }

    prepareCopiedRecord(newrecord, index);

    QModelIndex newIndex = addRecord(newrecord);
    insertCacheRow(newIndex.row());
    return newIndex;
}

bool SqlQueryModel::insertCacheRow(int row)
{
    if (row < 0 || row >= m_cache.size())
        return false;

    // assign a new UUID key
    m_cache[row][0] = QUuid::createUuid().toString();

    QString fields;
    QString values;

    for (int i = 0; i < m_columnCount; i++)
    {
        if ((m_columnIsEditable[i] == DBEditable) || i == 0)
        {
            if (!fields.isEmpty()) fields += ", ";
            if (!values.isEmpty()) values += ", ";
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
            insert.bindValue(bindcount, m_cache[row][i]);
            bindcount++;
        }
    }

    DB_LOCK;
    getDBOs()->getDb().transaction();
    if (insert.exec())
    {
        getDBOs()->getDb().commit();
        DB_UNLOCK;

        QModelIndex qil = createIndex(row, 0);
        QModelIndex qir = createIndex(row, columnCount() - 1);
        emit dataChanged(qil, qir);

        if (m_gui)
            refreshImpactedRecordsets(qil);

        return true;
    }
    getDBOs()->getDb().rollback();
    DB_UNLOCK;

    if (m_gui)
    {
        QMessageBox::critical(nullptr, QObject::tr("Cannot insert record"),
           insert.lastError().text() + "\n" + insert.lastQuery(), QMessageBox::Ok);
    }

    // revert the key so it stays as a new record
    m_cache[row][0] = QVariant();
    return false;
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
    delrow.prepare("UPDATE " + m_tablename + " SET deleted = 1 WHERE " + m_columnName[0] + " = ?");
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
    select.prepare("select count(*) from " + m_tablename + " where " + keycolumnname + " <> ? and " + columnname + " = ? AND deleted = 0");
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

                promptShowClosedProjects(key_columns, key_values, reference_count);
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
        select.prepare("select count(*) from " + m_relatedTable.at(i) + " where " + where_clause + " AND deleted = 0");
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

                promptShowClosedProjects(key_columns, key_values, reference_count);
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

void SqlQueryModel::promptShowClosedProjects(const QStringList &keyColumns, const QStringList &keyValues, int expectedCount)
{
    if (!getDBOs()->getShowClosedProjects() &&
        getDBOs()->searchresultsmodel()->rowCount(QModelIndex()) < expectedCount)
    {
        if (QMessageBox::question(nullptr,
                QObject::tr("Records Not Shown"),
                QObject::tr("Some related records are not visible because they belong to closed projects.\n\n"
                            "Would you like to show closed projects?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            getDBOs()->setShowClosedProjects(true);
            getDBOs()->setGlobalSearches(false);  // update model filters without a full refresh
            getDBOs()->searchresultsmodel()->PerformKeySearch(keyColumns, keyValues);
        }
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
    for ( int row = 0; row < rowCount(QModelIndex()); row++ )
    {
        if ( m_cache[row][searchColumn].toString().compare(lookupValue.toString()) == 0 )
        {
            return index(row, 0); // key is always at 0
        }
    }

    return QModelIndex();
}

const QModelIndex SqlQueryModel::findNextIndex(QVariant& lookupValue, int searchColumn, QModelIndex& startIndex)
{
    for ( int row = startIndex.row() + 1; row < rowCount(QModelIndex()); row++ )
    {
        if ( m_cache[row][searchColumn].toString().compare(lookupValue.toString()) == 0 )
        {
            return index(row, 0); // key is always at 0
        }
    }

    return QModelIndex();
}


bool SqlQueryModel::reloadRecord(const QModelIndex& index)
{
    DB_LOCK;
    QSqlQuery select(getDBOs()->getDb());
    // Include base filters (e.g. deleted filter) so that a record soft-deleted via sync
    // is not found here — letting the caller remove it from the cache.
    select.prepare("SELECT * FROM (" + BaseSQL() + constructWhereClause(false) + ") _t WHERE _t." + m_columnName[0] + " = ?");
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

                // Qualify the column name with the table name to avoid ambiguity in JOIN queries
                // (e.g. ProjectTeamMembersModel joins project_people and people, both have "id")
                QString qualifiedCol = m_tablename + "." + m_columnName[hashit.key()];

                // In: comma-separated filter value becomes  column IN ('v1','v2',...)
                if (m_filterCompareType[hashit.key()] == DBCompareType::In)
                {
                    const QStringList parts = column_value.toString().split(',', Qt::SkipEmptyParts);
                    QStringList quoted;
                    quoted.reserve(parts.size());
                    for (const QString& part : parts)
                        quoted.append(QString("'%1'").arg(part.trimmed().replace("'", "''")));
                    valuelist += QString("%1 IN (%2)").arg(qualifiedCol, quoted.join(','));
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
                            valuelist += QString(" ( %1 %3 %2").arg(qualifiedCol, column_value.toString(), compare_op);
                            valuelist += QString(" OR %1 IS NULL) ").arg(qualifiedCol);
                        }
                        else
                            valuelist += QString("%1 = %2").arg(qualifiedCol, column_value.toString());
                    }
                    else
                    {
                        sqlEscape(column_value, m_columnType[hashit.key()]);
                        valuelist += QString("%1 %3 '%2'").arg(qualifiedCol, column_value.toString(), compare_op);
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

    // Always filter out soft-deleted rows (unless the view handles it internally)
    if (!m_deletedFilterInView) {
        QString deletedFilter = QString("(%1.deleted IS NULL OR %1.deleted = 0)").arg(m_tablename);
        if (!valuelist.isEmpty())
            valuelist = deletedFilter + " AND " + valuelist;
        else
            valuelist = deletedFilter;
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

// importXMLNode - table-level import entry point.
//
// Called by importXMLDoc (databaseobjects.cpp) once per table section in the XML.
// domnode is the <table name="people"> (or similar) element. The function iterates
// every <row> child and delegates to setData(QDomElement*, bool) for the
// INSERT / UPDATE / soft-delete decision on each row.
//
// Returns false immediately on the first row that setData rejects (e.g. an invalid
// field value), so the caller can abort the whole import.
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

// setData (XML import overload) - core INSERT / UPDATE / soft-delete logic for a single row.
//
// Called by importXMLNode for each <row> element in the XML.  The ignoreKey parameter
// is always false in normal import use; it was historically used to force an INSERT
// by skipping the existence check, but that path has been removed.
//
// XML row format example:
//   <row>
//     <column name="id">{uuid}</column>
//     <column name="name">Jane Smith</column>
//     <column name="client_id" lookupvalue="Acme Corp"></column>
//   </row>
//
// A <row delete="true"> attribute triggers a soft-delete instead of an update.
// Foreign-key columns carry a human-readable lookupvalue attribute instead of a raw
// UUID so that records can be matched correctly even when ids differ between databases.
//
// The function works in four phases:
//   1. Unique-key WHERE clause  — build a SQL condition that can identify the record
//      by its business key (e.g. name = 'Jane Smith') without needing the UUID.
//   2. Column loop              — build the field/value lists for INSERT and UPDATE,
//      and capture the record id if it appears in the XML.
//   3. Existence check          — decide whether this row is new (INSERT) or already
//      exists (UPDATE / soft-delete), handling soft-deleted rows carefully.
//   4. SQL execution            — run the chosen INSERT / UPDATE / DELETE statement
//      and notify the display layer of the change.
bool SqlQueryModel::setData(QDomElement* xmlRow, bool ignoreKey)
{
    if (xmlRow->tagName() != "row")
    {
        //qDebug() << "tag name: " << xmlRow->tagName() << " is not a 'row'";
        return false;
    }

    // A <row delete="true"> means the source system wants this record soft-deleted.
    bool isdelete = (xmlRow->attribute("delete").compare("true", Qt::CaseInsensitive) == 0);

    // SQL fragment accumulators built up during the column loop below.
    QString whereclause;   // WHERE clause used to locate an existing record
    QString fields;        // comma-separated column names for INSERT
    QString updatevalues;  // comma-separated "col = val" pairs for UPDATE SET
    QString insertvalues;  // comma-separated values for INSERT VALUES
    QString keyfield = getColumnName(0);  // the primary-key column name (always column 0)
    QString keyvalue;      // the primary-key value from the XML, if present

    // -----------------------------------------------------------------------
    // Phase 1: Build a WHERE clause from the model's registered unique keys.
    //
    // m_uniqueKeys holds one or more sets of columns whose combined values
    // uniquely identify a row (analogous to a UNIQUE index).  We try to match
    // every column in a key set against the XML; if all columns are present we
    // use that set as the WHERE condition.  This lets us match records even
    // when the UUID differs between the exporting and importing database.
    //
    // Columns that are foreign keys carry a lookupvalue attribute (the human-
    // readable display value).  We resolve that to the local FK UUID with a
    // SELECT before using it in the WHERE clause.
    // -----------------------------------------------------------------------
    for (const QStringList& uk : m_uniqueKeys)
    {
        int found_count = 0;
        QString temp_where;

        // For each column that makes up this unique key, scan the XML for a
        // matching <column name="..."> element.
        for (const QString& kf : uk)
        {
            QDomNode element = xmlRow->firstChild();
            while (!element.isNull())
            {
                if (element.toElement().tagName().compare("column") == 0)
                {
                    QString field_name = element.toElement().attribute("name");
                    QVariant field_value = element.toElement().text();
                    QString lookup_value = element.toElement().attribute("lookupvalue");

                    int colnum = getColumnNumber(field_name);

                    // If this column is a FK with a lookupvalue, resolve the
                    // display string to the local UUID via a SELECT on the
                    // referenced table.
                    if (!lookup_value.isNull() && !m_lookupTable[colnum].isEmpty())
                    {
                        QString sql = QString("select %1 from %2 where %3 = '%4'").arg(m_lookupFkColumnName[colnum], m_lookupTable[colnum], m_lookupValueColumnName[colnum], lookup_value);
#ifdef QT_DEBUG
                        QLog_Debug(DEBUGLOG, QString("EXEC LOOKUP EXISTING: %1").arg(sql));
#endif

                        field_value = getDBOs()->execute(sql);
                    }

                    // Accumulate this column into temp_where if it is one of
                    // the unique-key fields we are currently building.
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

            // Only append temp_where to the final WHERE clause when every
            // column in this unique-key set was found in the XML.
            if (found_count == uk.count())
            {
                if (!whereclause.isEmpty())
                    whereclause += " and ";
                whereclause += temp_where;
            }
        }
    }

    // Save the unique-key WHERE clause before the column loop below potentially
    // replaces whereclause with an id-based condition.  We need it later to
    // detect whether a conflicting active record exists when the id points to
    // a soft-deleted row (see Phase 3).
    QString uniqueKeyWhereclause = whereclause;

    // -----------------------------------------------------------------------
    // Phase 2: Column loop — build INSERT / UPDATE value lists.
    //
    // Walk every <column> child of the <row> element.
    //   - If the column is the primary key (colnum == 0) and has a non-empty
    //     value, capture it in keyvalue and override whereclause with an
    //     id-based condition.  This gives a more precise lookup than the
    //     unique-key clause when both are available.
    //   - For all other editable columns, resolve FK lookupvalues, validate
    //     against any allowed-value lists, apply sqlEscape, and append to
    //     the fields / updatevalues / insertvalues accumulators.
    //   - Skip any required (non-key) column whose value is empty — running
    //     the INSERT would fail a NOT NULL constraint anyway.
    // -----------------------------------------------------------------------
    QDomNode element = xmlRow->firstChild();
    while (!element.isNull())
    {
        if (element.toElement().tagName().compare("column") == 0)
        {
            QString field_name = element.toElement().attribute("name");
            QVariant field_value = element.toElement().text();
            QString lookup_value = element.toElement().attribute("lookupvalue");

            int colnum = getColumnNumber(field_name);

            // When the XML carries the record id, switch to an id-based WHERE
            // clause for the existence check so we target the exact row.
            // An empty id element means the exporting side did not know the id;
            // in that case we keep the unique-key WHERE clause intact.
            if (colnum == 0 && !field_value.toString().isEmpty())
            {
                keyvalue = field_value.toString();
                whereclause = QString(" %1 = '%2'").arg(keyfield, keyvalue);
            }

            // Skip the primary-key column when its value is blank (nothing to
            // insert/update), and skip any column the model marks as non-editable.
            if ( !(colnum == 0 && (field_value.isNull() || field_value.toString().isEmpty())) &&
                 isEditable(colnum) )
            {
                if (!fields.isEmpty())
                    fields += ",";

                fields += field_name;

                // Resolve FK lookupvalue to the local UUID for the value lists.
                if (!lookup_value.isNull() && !m_lookupTable[colnum].isEmpty())
                {
                    QString sql = QString("select %1 from %2 where %3 = '%4'").arg(m_lookupFkColumnName[colnum], m_lookupTable[colnum], m_lookupValueColumnName[colnum], lookup_value);

                    field_value = getDBOs()->execute(sql);
                }

                if (!insertvalues.isEmpty())
                    insertvalues += ",";

                if (!updatevalues.isEmpty())
                    updatevalues += ",";

                // Reject the entire row if the value is not in the column's
                // allowed-values list (e.g. a status enum column).
                if (m_lookupValues[colnum] && !m_lookupValues[colnum]->contains(field_value.toString(), Qt::CaseSensitive))
                {
                    if (m_gui)
                        QMessageBox::critical(nullptr, QObject::tr("Invalid Field Value"), QString("""%1"" is not a valid field value.").arg(field_value.toString()));

                    return false;
                }

                // Convert the value to its SQL-safe form (quotes special chars,
                // converts empty strings to NULL for non-text columns, etc.).
                sqlEscape(field_value, m_columnType[colnum], false);

                // If a required non-key column resolved to NULL, skip this row
                // silently rather than letting the INSERT fail a NOT NULL constraint.
                if (field_value.isNull() && m_columnIsRequired[colnum] == DBRequired && colnum != 0)
                {
                    QLog_Error(ERRORLOG, QString("Import skipped row in %1: required field '%2' is empty.").arg(m_tablename, field_name));
                    return true;
                }

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



    // -----------------------------------------------------------------------
    // Phase 3: Already deleted check- Do Nothing
    //
    // If we have the key value, check to see if the record is a deleted one.
    // If the record was deleted then we do nothing.  We don't want to add a
    // record back in that we have already deleted.

    QString was_deleted_sql = QString("select %2 from %1 where %2 = '%3' AND deleted = 1").arg(m_tablename, getColumnName(0), keyvalue);
    QString deleted_id_field = getDBOs()->execute(was_deleted_sql);

    if (!deleted_id_field.isEmpty())
    {
#ifdef QT_DEBUG
        QLog_Debug(DEBUGLOG, QString("Import skipped row in %1: row with %2 = '%3' is was already deleted.").arg(m_tablename, getColumnName(0), keyvalue));
#endif
        return true;
    }

    // -----------------------------------------------------------------------
    // Phase 4: If we have the key value and it exist we update by the key only

    if (!keyvalue.isNull() && !keyvalue.isEmpty())
    {
        whereclause = QString("%1 = '%2'").arg(keyfield, keyvalue);
    }


    // -----------------------------------------------------------------------
    // Phase 5: Eecide INSERT vs UPDATE / soft-delete
    //
    // We need to handle four cases correctly:
    //
    //   a) Active record found by whereclause (deleted = 0)
    //        → UPDATE or soft-delete that record.
    //
    //   b) No record found at all
    //        → INSERT a new row.  Generate a UUID if the XML did not supply one.
    // -----------------------------------------------------------------------

    QString exists_sql = QString("select %3 from %1 where %2 AND deleted = 0").arg(m_tablename, whereclause, keyfield);
    QString id_field = getDBOs()->execute(exists_sql);



    QString sql;
    KeyColumnChange::OperationType disp_optype;

    if (!id_field.isEmpty())
    {
        // -----------------------------------------------------------------------
        // Phase 4a: Record exists — UPDATE or soft-delete.
        //
        // Use the specific record id in the WHERE clause (not the broader
        // unique-key clause) so exactly one row is affected.
        // -----------------------------------------------------------------------
        QString id_whereclause = QString("%1 = '%2'").arg(keyfield, id_field);

        if (isdelete)
        {
            sql = QString("UPDATE %1 SET deleted = 1 WHERE %2").arg(m_tablename, id_whereclause);
            disp_optype = KeyColumnChange::Delete;
        }
        else
        {
            sql = QString("update %1 set %2, deleted = 0 where %3").arg(m_tablename, updatevalues, id_whereclause);
            disp_optype = KeyColumnChange::Update;
        }
    }
    else
    {
        // -----------------------------------------------------------------------
        // Phase 4b: Record does not exist — INSERT.
        //
        // Append the primary-key column to the field list.  If the XML supplied
        // a UUID use it; otherwise generate a new one so the row has a stable id.
        // -----------------------------------------------------------------------
        if (!fields.isEmpty())
            fields += ",";

        fields += keyfield;

        if (!insertvalues.isEmpty())
            insertvalues += ",";

        if (keyvalue.isNull() || keyvalue.isEmpty())
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

    // Notify the display layer so the relevant table views refresh the changed row.
    getDBOs()->pushRowChange(tablename(), id_field, disp_optype);

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
            where = QString("select count(*) from %1 where ").arg(tablename()) + where + " AND deleted = 0";

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
