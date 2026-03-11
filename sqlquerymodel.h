// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SQLQUERYMODEL_H
#define SQLQUERYMODEL_H

#include "math.h"

#include <QString>
#include <QList>
#include <QStringList>
#include <QAbstractTableModel>
#include <QHash>
#include <QVector>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QDomElement>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


class DatabaseObjects;

class SqlQueryModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    enum DBColumnType {DBBlob, DBReal, DBDate, DBInteger, DBString, DBUSD, DBPercent, DBDateTime, DBBool, DBHtml};
    enum DBCompareType {Equals, GreaterThan, LessThan, NotEqual, Like};
    enum DBColumnRequired {DBRequired, DBNotRequired};
    enum DBColumnSearchable {DBSearchable, DBNotSearchable};
    enum DBColumnUnique {DBUnique, DBNotUnique};
    enum DBColumnEditable {DBEditable, DBReadOnly};
    enum DBRelationExportable {DBExportable, DBNotExportable};

    SqlQueryModel(DatabaseObjects* dbo);
    ~SqlQueryModel();

    DatabaseObjects* getDBOs() { return m_dbo; }

    void refreshImpactedRecordsets(QModelIndex index);

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setCacheData(const QModelIndex &index, const QVariant &value) { m_cache[index.row()][index.column()] = value; }

    bool importXMLNode(const QDomNode& domnode);
    bool setData(QDomElement* xmlRow, bool ignoreKey);

    void clear();
    void refresh();

    void setTableName(const QString &table, const QString &displayName);
    const QString& tablename() { return m_tablename; }
    const QString& displayname() { return m_displayName; }
    void setBaseSql(const QString table);
    const QString& BaseSQL() { return m_baseSql; }
    void setTop(unsigned long top) {m_top=top; }
    void setSkip(unsigned long skip) {m_top=skip; }

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sqlEscape(QVariant& columnValue, DBColumnType columnType, bool noQuote = false) const;
    void reformatValue(QVariant& columnValue, DBColumnType columnType) const;

    void addColumn(const QString& columnName, const QString& displayName, DBColumnType type, DBColumnSearchable searchable,
                   DBColumnRequired required = DBNotRequired, DBColumnEditable editTable = DBEditable, DBColumnUnique unique = DBNotUnique,
                   const QString& lookupTable = QString(), const QString& lookupFkColumnName = QString(), const QString& lookupValueColumnName = QString());
    void addColumn(const QString& columnName, const QString& displayName, DBColumnType type, DBColumnSearchable searchable, DBColumnRequired required,
                                    DBColumnEditable editable, DBColumnUnique unique, QStringList* valuelist);
    void renameColumn(const int columnNumber, const QString& columnName, const QString& displayName);

    void addRelatedTable(const QString& tableName, const QString& columnName, const QString& fkColumnName, const QString& title, const DBRelationExportable exportable = DBNotExportable);
    void addRelatedTable(const QString& tableName, const QStringList& columnNames, const QStringList& fkColumnNames, const QString& title, const DBRelationExportable exportable = DBNotExportable);

    void addUniqueKeys(QStringList uniqueKeys, const QString& name) { m_uniqueKeys[name] = uniqueKeys; }
    void associateLookupValues(int columnNumber, QStringList* lookupValues);
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value,
                       int role = Qt::EditRole) override;

    static QDateTime parseDateTime(QString entrydate);
    virtual const QModelIndex addRecord(QVector<QVariant>& newrecord);
    virtual const QModelIndex copyRecord(QModelIndex index);
    virtual const QModelIndex newRecord(const QVariant* fkValue1 = nullptr, const QVariant* fkValue2 = nullptr);
    virtual bool deleteRecord(QModelIndex index);
    bool copyAndFilterRow(QModelIndex& qmi, SqlQueryModel& pnmodel);
    void deleteRelatedRecords(QVariant& keyval);
    void removeCacheRecord(QModelIndex index);

    int rowCount(const QModelIndex &parent) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    bool isUniqueValue(const QVariant &newValue, const QModelIndex &index);
    bool isNewRecord(const QModelIndex &index) { return m_cache[index.row()].value(0).isNull(); } // new records have blank guid in 0
    bool deleteCheck(const QModelIndex &index);
    bool columnChangeCheck(const QModelIndex &index);

    QVector<QVariant> emptyrecord();
    const QVariant findValue(QVariant& lookupValue, int searchColumn, int returnColumn);
    const QModelIndex findIndex(QVariant& lookupValue, int searchColumn);
    void setShowBlank(bool show = true) { m_showBlank = show; }
    bool reloadRecord(const QModelIndex& index);
    bool loadAndFilterRow(const QVariant& id);


    QString constructWhereClause(bool includeUserFilter = true);
    void setFilter(int columnNumber, const QString& filterValue, DBCompareType compare = DBCompareType::Equals);
    bool setFilter(QDomNode& xmlfilter);

    QVariant getFilter(int columnNumber);
    bool hasFilter(int columnNumber) const { return m_columnIsFiltered[columnNumber];}
    void clearAllFilters();
    void clearFilter(int columnNumber);

    void setUserFilter(int columnNumber, const QVariantList& filterValues);
    const QVariantList& getUserFilter(int columnNumber) { return m_userFilterValues[columnNumber]; }
    void setUserSearchString(int columnNumber, const QVariant& searchValue);
    QVariant& getUserSearchString(int columnNumber) { return m_userSearchString[columnNumber]; }

    void setUserSearchRange(int columnNumber, const QVariant& searchBeginValue, const QVariant& m_Search_end_value );
    void getUserSearchRange(int columnNumber, QVariant& searchBeginValue, QVariant& searchEndValue );
    void clearAllUserSearches();
    void clearUserFilter(int columnNumber);
    void clearUserSearchString(int columnNumber);
    void clearUserSearchRange(int columnNumber);
    bool hasUserFilters(int columnNumber) const;
    bool hasUserFilters() const;
    void activateUserFilter(QString filterName);
    void deactivateUserFilter(QString filterName);
    void loadLastUserFilterState(QString filterName);

    void saveUserFilter( QString filterName);
    void loadUserFilter( QString filterName);
    bool getUserFilterState() { return m_userFilterActive; }

    void setOrderBy(const QString& orderBy) { m_orderBy = orderBy; }
    void clearOrderBy() { m_orderBy.clear(); }
    void setForeignKeyValueColumn(const QString& fkValue) { m_foreignKeyValue = fkValue; }
    const QString& getForeignKeyValueColumn() { return m_foreignKeyValue; }

    void setEditable( int column, DBColumnEditable editable ) { m_columnIsEditable[column] = editable; }
    bool isEditable( int column ) { return (m_columnIsEditable[column] == DBEditable); }
    void setSearchable( int column, DBColumnSearchable searchable ) { m_columnIsSearchable[column] = searchable; }
    bool isSearchable( int column ) { return (m_columnIsSearchable[column] == DBSearchable); }
    void setRequired( int column, DBColumnRequired required ) { m_columnIsRequired[column] = required; }
    bool isRequired( int column ) { return (m_columnIsRequired[column] == DBRequired); }
    DBColumnType getType( const int column ) const { return m_columnType[column]; }
    void setType( const int column, const DBColumnType columnType ) { m_columnType[column] = columnType; }
    QString getColumnName( int column ) { return m_columnName[column]; }
    QString getColumnName( QString& displayName );
    int getColumnNumber(const QString& fieldName );

    bool isReadOnly() { return m_readOnly; }
    bool isUniqueColumn(int column) { return (m_columnIsUnique[column] == DBUnique); }
    bool checkUniqueKeys(const QModelIndex &index, const QVariant &value);
    void setReadOnly() { m_readOnly = true; }
    bool isExportable() { return m_canExport; }
    void setNoExport() { m_canExport = false; }
    void setOrderKey(int key) { m_orderKey = key; }
    int getOrderKey() { return m_orderKey; }

    static QString removeInvalidXmlCharacters(const QString &input);
    QDomElement toQDomElement( QDomDocument* xmlDocument, const QString& filter = QString());

private:
    QString m_tablename;  // the table to write data too, also the table to sync with other models when changed
    QString m_displayName;
    QString m_baseSql;
    bool m_gui; // gui based recordset
    int m_orderKey = 0; // the order key is used to identify record heirarchy - base data is first
    int m_columnCount = 0;

    QHash<int, QString> m_columnName;
    QHash<int, DBColumnType> m_columnType;
    QHash<int, DBColumnRequired> m_columnIsRequired;
    QHash<int, DBColumnSearchable> m_columnIsSearchable;
    QHash<int, DBColumnEditable> m_columnIsEditable;
    QHash<int, DBColumnUnique> m_columnIsUnique;

    QHash<int, bool> m_columnIsFiltered;
    QHash<int, QVariant> m_filterValue;
    QHash<int, DBCompareType> m_filterCompareType;
    unsigned long m_skip = 0;
    unsigned long m_top = 0;

    QHash<int, bool> m_isUserFiltered;
    QHash<int, QVariantList> m_userFilterValues;
    QHash<int, QVariant> m_userSearchString;

    QHash<int, bool> m_isUserRangeFiltered;
    QHash<int, QVariant> m_rangeSearchStart;
    QHash<int, QVariant> m_rangeSearchEnd;

    // setup unique keys to be used
    QHash<QString, QStringList> m_uniqueKeys;

    // track related columns for xml import/export
    QHash<int, QString> m_lookupTable;
    QHash<int, QString> m_lookupValueColumnName;
    QHash<int, QString> m_lookupFkColumnName;
    QHash<int, QStringList*> m_lookupValues;

    // track for deletion checking and exporting
    QVector<QString> m_relatedTable;
    QVector<QStringList> m_relatedColumns;
    QVector<QStringList> m_relatedFkColumns;
    QVector<QString> m_relationTitle;
    QVector<DBRelationExportable> m_relationExportable;

    QVector<QVector<QVariant>> m_cache;
    QVector<QHash<int, QVariant> > m_headers;

    bool m_showBlank = false;

    QString m_orderBy;
    QString m_foreignKeyValue;
    bool m_userFilterActive = false;
    bool m_readOnly = false;
    bool m_canExport = true;

    DatabaseObjects* m_dbo;

signals:
    void callKeySearch();
};

#endif // SQLQUERYMODEL_H
