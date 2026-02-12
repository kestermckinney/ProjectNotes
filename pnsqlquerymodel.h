// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

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
#include <QDomElement>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


class PNDatabaseObjects;

class PNSqlQueryModel : public QAbstractTableModel
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

    PNSqlQueryModel(PNDatabaseObjects* t_dbo);
    ~PNSqlQueryModel();

    PNDatabaseObjects* getDBOs() { return m_dbo; }

    void refreshImpactedRecordsets(QModelIndex t_index);

    bool setData(const QModelIndex &t_index, const QVariant &t_value, int t_role) override;
    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;

    void setCacheData(const QModelIndex &t_index, const QVariant &t_value) { m_cache[t_index.row()][t_index.column()] = t_value; }

    bool importXMLNode(const QDomNode& t_domnode);
    bool setData(QDomElement* t_xml_row, bool t_ignore_key);

    void clear();
    void refresh();

    void setTableName(const QString &t_table, const QString &t_display_name);
    const QString& tablename() { return m_tablename; }
    const QString& displayname() { return m_display_name; }
    void setBaseSql(const QString t_table);
    const QString& BaseSQL() { return m_base_sql; }
    void setTop(unsigned long t_top) {m_top=t_top; }
    void setSkip(unsigned long t_skip) {m_top=t_skip; }

    Qt::ItemFlags flags(const QModelIndex &t_index) const override;

    void sqlEscape(QVariant& t_column_value, DBColumnType t_column_type, bool t_no_quote = false) const;
    void reformatValue(QVariant& t_column_value, DBColumnType t_column_type) const;

    void addColumn(const QString& t_column_name, const QString& t_display_name, DBColumnType t_type, DBColumnSearchable t_searchable,
                   DBColumnRequired t_required = DBNotRequired, DBColumnEditable t_edit_table = DBEditable, DBColumnUnique t_unique = DBNotUnique,
                   const QString& t_lookup_table = QString(), const QString& t_lookup_fk_column_name = QString(), const QString& t_lookup_value_column_name = QString());
    void addColumn(const QString& t_column_name, const QString& t_display_name, DBColumnType t_type, DBColumnSearchable t_searchable, DBColumnRequired t_required,
                                    DBColumnEditable t_editable, DBColumnUnique t_unique, QStringList* t_valuelist);
    void addRelatedTable(const QString& t_table_name, const QString& t_column_name, const QString& t_fk_column_name, const QString& t_title, const DBRelationExportable t_exportable = DBNotExportable);
    void addRelatedTable(const QString& t_table_name, const QStringList& t_column_names, const QStringList& t_fk_column_names, const QString& t_title, const DBRelationExportable t_exportable = DBNotExportable);

    void addUniqueKeys(QStringList t_unique_keys, const QString& t_name) { m_unique_keys[t_name] = t_unique_keys; }
    void associateLookupValues(int t_column_number, QStringList* t_lookup_values);
    QVariant headerData(int t_section, Qt::Orientation t_orientation,
                        int t_role = Qt::DisplayRole) const override;
    bool setHeaderData(int t_section, Qt::Orientation t_orientation, const QVariant &t_value,
                       int t_role = Qt::EditRole) override;

    static QDateTime parseDateTime(QString t_entrydate);
    virtual const QModelIndex addRecord(QVector<QVariant>& t_newrecord);
    virtual const QModelIndex copyRecord(QModelIndex t_index);
    virtual const QModelIndex newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr);
    virtual bool deleteRecord(QModelIndex t_index);
    bool copyAndFilterRow(QModelIndex& t_qmi, PNSqlQueryModel& t_pnmodel);
    void deleteRelatedRecords(QVariant& t_keyval);
    void removeCacheRecord(QModelIndex t_index);

    int rowCount(const QModelIndex &t_parent) const override;

    int columnCount(const QModelIndex &t_parent = QModelIndex()) const override;

    bool isUniqueValue(const QVariant &t_new_value, const QModelIndex &t_index);
    bool isNewRecord(const QModelIndex &t_index) { return m_cache[t_index.row()].value(0).isNull(); } // new records have blank guid in 0
    bool deleteCheck(const QModelIndex &t_index);
    bool columnChangeCheck(const QModelIndex &t_index);

    QVector<QVariant> emptyrecord();
    const QVariant findValue(QVariant& t_lookup_value, int t_search_column, int t_return_column);
    const QModelIndex findIndex(QVariant& t_lookup_value, int t_search_column);
    void setShowBlank(bool t_show = true) { m_show_blank = t_show; }
    bool reloadRecord(const QModelIndex& t_index);

    QString constructWhereClause(bool t_include_user_filter = true);
    void setFilter(int t_column_number, const QString& t_filter_value, DBCompareType t_compare = DBCompareType::Equals);
    bool setFilter(QDomNode& t_xmlfilter);

    QVariant getFilter(int t_column_number);
    bool hasFilter(int t_column_number) const { return m_column_is_filtered[t_column_number];}
    void clearAllFilters();
    void clearFilter(int t_column_number);

    void setUserFilter(int t_column_number, const QVariantList& t_ilter_values);
    const QVariantList& getUserFilter(int t_column_number) { return m_user_filter_values[t_column_number]; }
    void setUserSearchString(int t_column_number, const QVariant& t_search_value);
    QVariant& getUserSearchString(int t_column_number) { return m_user_search_string[t_column_number]; }

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
    bool getUserFilterState() { return m_user_filter_active; }

    void setOrderBy(const QString& t_order_by) { m_order_by = t_order_by; }
    void clearOrderBy() { m_order_by.clear(); }
    void setForeignKeyValueColumn(const QString& t_fk_value) { m_ForeignKeyValue = t_fk_value; }
    const QString& getForeignKeyValueColumn() { return m_ForeignKeyValue; }

    void setEditable( int t_column, DBColumnEditable t_editable ) { m_column_is_editable[t_column] = t_editable; }
    bool isEditable( int t_column ) { return (m_column_is_editable[t_column] == DBEditable); }
    void setSearchable( int t_column, DBColumnSearchable t_searchable ) { m_column_is_searchable[t_column] = t_searchable; }
    bool isSearchable( int t_column ) { return (m_column_is_searchable[t_column] == DBSearchable); }
    void setRequired( int t_column, DBColumnRequired t_required ) { m_column_is_required[t_column] = t_required; }
    bool isRequired( int t_column ) { return (m_column_is_required[t_column] == DBRequired); }
    DBColumnType getType( const int t_column ) const { return m_column_type[t_column]; }
    void setType( const int t_column, const DBColumnType t_column_type ) { m_column_type[t_column] = t_column_type; }
    QString getColumnName( int t_column ) { return m_column_name[t_column]; }
    QString getColumnName( QString& t_display_name );
    int getColumnNumber(const QString& t_field_name );

    bool isReadOnly() { return m_read_only; }
    bool isUniqueColumn(int t_column) { return (m_column_is_unique[t_column] == DBUnique); }
    bool checkUniqueKeys(const QModelIndex &t_index, const QVariant &t_value);
    void setReadOnly() { m_read_only = true; }
    bool isExportable() { return m_can_export; }
    void setNoExport() { m_can_export = false; }
    void setOrderKey(int t_key) { m_order_key = t_key; }
    int getOrderKey() { return m_order_key; }

    static QString removeInvalidXmlCharacters(const QString &t_input);
    QDomElement toQDomElement( QDomDocument* t_xml_document, const QString& t_filter = QString());

private:
    QString m_tablename;  // the t_table to write data too, also the t_table to sync with other models when changed
    QString m_display_name;
    QString m_base_sql;
    bool m_gui; // gui based recordset
    int m_order_key = 0; // the order key is used to identify record heirarchy - base data is first
    int m_column_count = 0;

    QHash<int, QString> m_column_name;
    QHash<int, DBColumnType> m_column_type;
    QHash<int, DBColumnRequired> m_column_is_required;
    QHash<int, DBColumnSearchable> m_column_is_searchable;
    QHash<int, DBColumnEditable> m_column_is_editable;
    QHash<int, DBColumnUnique> m_column_is_unique;

    QHash<int, bool> m_column_is_filtered;
    QHash<int, QVariant> m_filter_value;
    QHash<int, DBCompareType> m_filter_compare_type;
    unsigned long m_skip = 0;
    unsigned long m_top = 0;

    QHash<int, bool> m_is_user_filtered;
    QHash<int, QVariantList> m_user_filter_values;
    QHash<int, QVariant> m_user_search_string;

    QHash<int, bool> m_is_user_range_filtered;
    QHash<int, QVariant> m_range_search_start;
    QHash<int, QVariant> m_range_search_end;

    // setup unique keys to be used
    QHash<QString, QStringList> m_unique_keys;

    // track related columns for xml import/export
    QHash<int, QString> m_lookup_table;
    QHash<int, QString> m_lookup_value_column_name;
    QHash<int, QString> m_lookup_fk_column_name;
    QHash<int, QStringList*> m_lookup_values;

    // track for deletion checking and exporting
    QVector<QString> m_related_table;
    QVector<QStringList> m_related_columns;
    QVector<QStringList> m_related_fk_columns;
    QVector<QString> m_relation_title;
    QVector<DBRelationExportable> m_relation_exportable;

    QVector<QVector<QVariant>> m_cache;
    QVector<QHash<int, QVariant> > m_headers;

    bool m_show_blank = false;

    QString m_order_by;
    QString m_ForeignKeyValue;
    bool m_user_filter_active = false;
    bool m_read_only = false;
    bool m_can_export = true;
    bool m_is_dirty = false; // only set to true when related query models have changed

    PNDatabaseObjects* m_dbo;

signals:
    void callKeySearch();
};

#endif // PNSQLQUERYMODEL_H
