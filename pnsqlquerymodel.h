#ifndef PNSQLQUERYMODEL_H
#define PNSQLQUERYMODEL_H

#include "math.h"
//#include "importexport.h"

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

class PNSqlQueryModel : public QAbstractTableModel
{
public:

    enum DBColumnType {DBBlob, DBReal, DBDate, DBInteger, DBString, DBUSD, DBPercent, DBDateTime, DBBool};
    enum DBCompareType {Equals, GreaterThan, LessThan, NotEqual};
    enum DBColumnRequired {DBRequired, DBNotRequired};
    enum DBColumnSearchable {DBSearchable, DBNotSearchable};
    enum DBColumnUnique {DBUnique, DBNotUnique};
    enum DBColumnEditable {DBEditable, DBReadOnly};
    enum DBRelationExportable {DBExportable, DBNotExportable};

    PNSqlQueryModel(QObject *parent);
    ~PNSqlQueryModel();

    void refreshImpactedRecordsets(QModelIndex t_index);

    bool setData(const QModelIndex &t_index, const QVariant &t_value, int t_role) override;
    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;

    void clear();
    void refresh();

    void setTableName(const QString &t_table, const QString &t_display_name) { m_tablename = t_table; m_display_name = t_display_name; };
    const QString& tablename() { return m_tablename; };
    void setBaseSql(const QString t_table);
    const QString& BaseSQL() { return m_base_sql; };

    Qt::ItemFlags flags(const QModelIndex &t_index) const override;

    void sqlEscape(QVariant& t_column_value, DBColumnType t_column_type, bool t_no_quote = false) const;
    void reformatValue(QVariant& t_column_value, DBColumnType t_column_type) const;

    void addColumn(int t_column_number, const QString& t_display_name, DBColumnType t_type, DBColumnSearchable t_searchable, DBColumnRequired t_required = DBNotRequired, DBColumnEditable t_edit_table = DBEditable, DBColumnUnique t_unique = DBNotUnique);
    void addRelatedTable(const QString& t_table_name, const QString& t_colum_name, const QString& t_title, const DBRelationExportable exportable = DBNotExportable);
    void associateLookupValues(int t_column_number, QStringList* t_lookup_values);
    QVariant headerData(int t_section, Qt::Orientation t_orientation,
                        int t_role = Qt::DisplayRole) const override;
    bool setHeaderData(int t_section, Qt::Orientation t_orientation, const QVariant &t_value,
                       int t_role = Qt::EditRole) override;

    static QDateTime parseDateTime(QString t_entrydate);
    virtual bool addRecord(QSqlRecord& t_newrecord);
    virtual bool copyRecord(QModelIndex t_index);
    virtual bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr);
    virtual bool deleteRecord(QModelIndex t_index);
    virtual bool openRecord(QModelIndex t_index);

    int rowCount(const QModelIndex &t_parent) const override;

    int columnCount(const QModelIndex &t_parent = QModelIndex()) const override;

    bool isUniqueValue(const QVariant &t_new_value, const QModelIndex &t_index);
    bool deleteCheck(const QModelIndex &t_index);
    QSqlRecord emptyrecord();
    const QVariant findValue(QVariant& t_lookup_value, int t_search_column, int t_return_column);
    void setShowBlank(bool t_show = true) { m_show_blank = t_show; };
    bool reloadRecord(const QModelIndex& t_index);

    QString constructWhereClause(bool t_include_user_filter = true);
    void setFilter(int t_column_number, const QString& t_filter_value, DBCompareType t_compare = DBCompareType::Equals);
    bool hasFilter(int t_column_number) const { return m_column_is_filtered[t_column_number];};
    void clearAllFilters();
    void clearFilter(int t_column_number);

    void setUserFilter(int t_column_number, const QVariantList& t_ilter_values);
    const QVariantList& getUserFilter(int t_column_number) { return m_user_filter_values[t_column_number]; };
    void setUserSearchString(int t_column_number, const QVariant& t_search_value);
    QVariant& getUserSearchString(int t_column_number) { return m_user_search_string[t_column_number]; };

    void setUserSearchRange(int t_column_number, const QVariant& t_search_begin_value, const QVariant& m_Search_end_value );
    void getUserSearchRange(int t_column_number, QVariant& t_earch_begin_value, QVariant& t_search_end_value );
    void clearAllUserSearches();
    void clearUserFilter(int t_column_number);
    void clearUserSearchString(int t_column_number);
    void clearUserSearchRange(int t_column_number);
    bool hasUserFilters(int t_column_number) const;
    bool hasUserFilters() const;
    void activateUserFilter(QString t_filter_name);
    void deactivateUserFilter(QString t_filter_name);
    void loadLastUserFilterState(QString t_filter_name);

    void saveUserFilter( QString t_filter_name);
    void loadUserFilter( QString t_filter_name);
    bool getUserFilterState() { return m_user_filter_active; };

    void setOrderBy(const QString& t_order_by) { m_order_by = t_order_by; };
    void clearOrderBy() { m_order_by.clear(); };

    void setEditable( int t_column, DBColumnEditable t_editable ) { m_column_is_editable[t_column] = t_editable; };
    bool isEditable( int t_column ) { return (m_column_is_editable[t_column] == DBEditable); }
    void setSearchable( int t_column, DBColumnSearchable t_searchable ) { m_column_is_searchable[t_column] = t_searchable; };
    bool isSearchable( int t_column ) { return (m_column_is_searchable[t_column] == DBSearchable); };
    void setRequired( int t_column, DBColumnRequired t_required ) { m_column_is_required[t_column] = t_required; };
    bool isRequired( int t_column ) { return (m_column_is_required[t_column] == DBRequired); };
    DBColumnType getType( const int t_column ) const { return m_column_type[t_column]; };
    void setType( const int t_column, const DBColumnType t_column_type ) { m_column_type[t_column] = t_column_type; };
    void setLookup(int t_column, PNSqlQueryModel* t_lookup, int t_lookup_fk_column, int t_lookup_value_column);
    void setLookup(int t_column, QStringList* t_lookup);
    QVariant getLookupValue( const QModelIndex& t_index);
    QString getColumnName( int t_column ) {
        return m_sql_query.record().fieldName(t_column);
    };
    QString getColumnName( QString& t_display_name );
    int getColumnNumber( QString& t_field_name );
    int getUniqueColumnCount();

    bool isReadOnly() { return m_read_only; };
    bool isUniqueColumn(int t_column) { return m_column_is_unique[t_column]; };
    void setReadOnly() { m_read_only = true; };

//    const QVector<QString> getRelatedTables() { return m_related_table; };
//    const QString getRelatedColumn(int t_col) { return m_related_column[t_col]; };
    QDomElement toQDomElement( QDomDocument& t_xml_document );
    // use this to allow for different filters from the original
    virtual PNSqlQueryModel* createExportVersion();


//    QList<PNSqlQueryModel*> childRecordsets();

private:
    QString m_tablename;  // the t_table to write data too, also the t_table to sync with other models when changed
    QString m_display_name;
    QString m_base_sql;

    QHash<int, DBColumnType> m_column_type;
    QHash<int, DBColumnRequired> m_column_is_required;
    QHash<int, DBColumnSearchable> m_column_is_searchable;
    QHash<int, DBColumnEditable> m_column_is_editable;
    QHash<int, DBColumnUnique> m_column_is_unique;

    QHash<int, bool> m_column_is_filtered;
    QHash<int, QVariant> m_filter_value;
    QHash<int, DBCompareType> m_filter_compare_type;

    QHash<int, bool> m_is_user_filtered;
    QHash<int, QVariantList> m_user_filter_values;
    QHash<int, QVariant> m_user_search_string;

    QHash<int, bool> m_is_user_range_filtered;
    QHash<int, QVariant> m_range_search_start;
    QHash<int, QVariant> m_range_search_end;

    QHash<int, PNSqlQueryModel*> m_lookup_view;
    QHash<int, QStringList*> m_lookup_values;
    QHash<int, int> m_lookup_value_column;
    QHash<int, int> m_lookup_fk_column;


    // track for deletion checking and exporting
    QVector<QString> m_related_table;
    QVector<QString> m_related_column;
    QVector<QString> m_relation_title;
    QVector<DBRelationExportable> m_relation_exportable;

    QSqlQuery m_sql_query;
    QVector<QSqlRecord> m_cache;
    QVector<QHash<int, QVariant> > m_headers;

    bool m_show_blank = false;

    QString m_order_by;
    bool m_user_filter_active = false;
    bool m_read_only = false;

    // list of created models
    static QList<PNSqlQueryModel*> m_open_recordsets;
};

#endif // PNSQLQUERYMODEL_H
