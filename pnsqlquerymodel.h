#ifndef PNSQLQUERYMODEL_H
#define PNSQLQUERYMODEL_H

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

class PNSqlQueryModel : public QAbstractTableModel
{
public:

    enum DBColumnType {DB_BLOB, DB_REAL, DB_DATE, DB_INTEGER, DB_STRING, DB_USD, DB_PERCENT, DB_DATETIME, DB_BOOL};

    PNSqlQueryModel(QObject *parent);

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void clear();
    void Refresh();

    void setTableName(const QString &table, const QString &DisplayName) { m_tablename = table; m_DisplayName = DisplayName; };
    const QString& tablename() { return m_tablename; };
    void setBaseSql(const QString table) { m_BaseSQL = table;};
    const QString& BaseSQL() { return m_BaseSQL; };

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void SQLEscape(QVariant& ColumnValue, DBColumnType ColumnType) const;
    void ReformatValue(QVariant& ColumnValue, DBColumnType ColumnType) const;

    void AddColumn(int ColumnNumber, const QString& DisplayName, DBColumnType Type, bool Searchable, bool Required = false, bool Editable = true, bool Unique = false);
    void AddRelatedTable(const QString& TableName, const QString& ColumnName, const QString& Title);
    void AssociateLookupValues(int ColumnNumber, QStringList* LookupValues);
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value,
                       int role = Qt::EditRole) override;

    static QDateTime ParseDateTime(QString entrydate);
    virtual bool AddRecord(QSqlRecord& newrecord);
    virtual bool NewRecord();
    bool DeleteRecord(QModelIndex index);

    int rowCount(const QModelIndex &parent) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    bool isUniqueValue(const QVariant &NewValue, const QModelIndex &index);
    bool DeleteCheck(const QModelIndex &index);
    QSqlRecord emptyrecord();
    const QVariant FindValue(QVariant& LookupValue, int SearchColumn, int ReturnColumn);
    void setShowBlank(bool show = true) { m_ShowBlank = show; };
    bool ReloadRecord(const QModelIndex& index);

    QString ConstructWhereClause(); // TODO: Add where clause function
    void SetFilter(int ColumnNumber, const QString& FilterValue);
    void ClearAllFilters();
    void ClearFilter(int ColumnNumber);

    void SetUserFilter(int ColumnNumber, const QStringList& FilterValues);
    const QStringList& GetUserFilter(int ColumnNumber) { return m_UserFilterValues[ColumnNumber]; };
    void SetUserSearchString(int ColumnNumber, const QString& SearchValue);
    QVariant& GetUserSearchString(int ColumnNumber) { return m_UserSearchString[ColumnNumber]; };

    void SetUserSearchRange(int ColumnNumber, const QString& SearchBeginValue, const QString& SearchEndValue );
    void GetUserSearchRange(int ColumnNumber, QVariant& SearchBeginValue, QVariant& SearchEndValue );
    void ClearAllUserSearches();
    void ClearUserFilter(int ColumnNumber);
    void ClearUserSearchString(int ColumnNumber);
    void ClearUserSearchRange(int ColumnNumber);
    bool HasUserFilters(int ColumnNumber);
    bool HasUserFilters();
    void ActivateUserFilter(QString FilterName);
    void DeactivateUserFilter(QString FilterName);
    void LoadLastUserFilterState(QString FilterName);

    void SaveUserFilter( QString FilterName);
    void LoadUserFilter( QString FilterName);
    bool GetUserFilterState() { return m_UserFilterActive; };

    void SetOrderBy(const QString& OrderBy) { m_OrderBy = OrderBy; };
    void ClearOrderBy() { m_OrderBy.clear(); };

    void setEditable( int Column, bool Editable ) { m_ColumnIsEditable[Column] = Editable; };
    bool isEditable( int Column ) { return m_ColumnIsEditable[Column]; }
    void setSearchable( int Column, bool Searchable ) { m_ColumnSearchable[Column] = Searchable; };
    bool isSearchable( int Column ) { return m_ColumnSearchable[Column]; };
    void setRequired( int Column, bool Required ) { m_ColumnRequired[Column] = Required; };
    bool isRequired( int Column ) { return m_ColumnRequired[Column]; };
    DBColumnType getType( const int Column ) const { return m_ColumnType[Column]; };
    void setType( const int Column, const DBColumnType ColumnType ) { m_ColumnType[Column] = ColumnType; };
    void setLookup(int Column, PNSqlQueryModel* lookup, int LookupFK, int LookupValue);
    void setLookup(int Column, QStringList* lookup);
    QVariant getLookupValue( const QModelIndex& index);
    QString getColumnName( int Col ) { return m_SqlQuery.record().fieldName(Col); };
    QString getColumnName( QString& DisplayName );
    int getColumnNumber( QString& FieldName );

    bool isReadOnly() { return m_ReadOnly; };
    void setReadOnly() { m_ReadOnly = true; };

private:
    QString m_tablename;  // the table to write data too, also the table to sync with other models when changed
    QString m_DisplayName;
    QString m_BaseSQL;

    QHash<int, DBColumnType> m_ColumnType;
    QHash<int, bool> m_ColumnRequired;
    QHash<int, bool> m_ColumnSearchable;
    QHash<int, bool> m_ColumnIsEditable;

    QHash<int, bool> m_ColumnIsUnique;

    // TODO: setup filters and lookup views
    QHash<int, bool> m_IsFiltered;
    QHash<int, QVariant> m_FilterValue;

    QHash<int, bool> m_IsUserFiltered;
    QHash<int, QStringList> m_UserFilterValues;
    QHash<int, QVariant> m_UserSearchString;

    QHash<int, bool> m_IsUserRangeFiltered;
    QHash<int, QVariant> m_RangeSearchStart;
    QHash<int, QVariant> m_RangeSearchEnd;

    QHash<int, PNSqlQueryModel*> m_LookupView;
    QHash<int, QStringList*> m_LookupValues;
    QHash<int, int> m_LookupValue;
    QHash<int, int> m_LookupFK;


    // track for deletion checking
    QVector<QString> m_RelatedTable;
    QVector<QString> m_RelatedColumn;
    QVector<QString> m_RelationTitle;

    QSqlQuery m_SqlQuery;
    QVector<QSqlRecord> m_cache;
    QVector<QHash<int, QVariant> > m_headers;

    bool m_ShowBlank = false;

    QString m_OrderBy; // TODO: Add OrderBy
    bool m_UserFilterActive = false; // TODO: Add User filter
    bool m_ReadOnly = false;

};

#endif // PNSQLQUERYMODEL_H
