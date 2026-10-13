// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Format-neutral record graph shared by XML import/export and the plugin data
// API. One normalization path: every writer (XML import, plugins) goes through
// SqlQueryModel::importRecordRow(), and every reader goes through
// RecordTreeExporter, so unique-key matching, lookup resolution, soft delete
// and owned-child cascade behave the same no matter which format is used.

#ifndef RECORDTREE_H
#define RECORDTREE_H

#include <QHash>
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <functional>

class QDomDocument;
class QDomElement;
class QDomNode;
class DatabaseObjects;
class SqlQueryModel;

struct RecordTable;

// One column of a row. `value` is the raw database value when exported, and
// the export text form (the same text the XML format carries, e.g. dates as
// MM/dd/yyyy) when imported. `lookup` carries the display value of a foreign
// key; on import a non-null lookup is resolved to the local key.
struct RecordColumn {
    QString name;
    QVariant value;
    QString lookup;          // null QString = no lookup
    int type = 0;            // SqlQueryModel::DBColumnType when exported
};

struct RecordRow {
    QList<RecordColumn> columns;
    QList<RecordTable> children;
    bool deleted = false;

    const RecordColumn* column(const QString& name) const;
    QVariant value(const QString& name) const;
};

struct RecordTable {
    QString name;
    // filter_field_N / filter_value_N pairs describing how a child table was
    // selected from its parent row (kept for XML compatibility).
    QList<QPair<QString, QString>> filters;
    QList<RecordRow> rows;
};

struct RecordTree {
    QList<QPair<QString, QString>> attributes;   // root attributes, in order
    QList<RecordTable> tables;
};

// What an import did with one row. The XML path ignores it; the plugin API
// reports it back to scripts.
struct RecordImportOutcome {
    enum Kind { Inserted, Updated, Deleted, Unchanged, SkippedPreviouslyDeleted,
                SkippedRequiredEmpty, SkippedReferenced, Rejected };
    Kind kind = Rejected;
    QString key;             // primary key of the affected row
    QString column;          // offending column for SkippedRequiredEmpty / Rejected
    QString message;
};

// Options for RecordTreeExporter.
struct RecordExportSpec {
    // Decides whether an exportable child relation is followed. The table name
    // is the stored table name. Empty = follow every exportable relation.
    std::function<bool(const QString& table)> includeTable;
    // Columns to emit per table (stored names). Missing table = all columns.
    // The key column and columns needed for joins and lookups are always read.
    QHash<QString, QStringList> fields;
    // Resolve lookup display values for foreign-key columns.
    bool resolveLookups = true;
    // Maximum nesting depth of child relations below the top table (-1 = all).
    int maxDepth = -1;
};

// Exports rows of a model (and its exportable children) with batched queries:
// one lookup query per foreign-key column per level, and one query per child
// relation per level, instead of one query per cell and per parent row.
class RecordTreeExporter {
public:
    explicit RecordTreeExporter(DatabaseObjects* dbo) : m_dbo(dbo) {}

    // Exports the rows currently loaded in `model` (after the caller set its
    // filters and called refresh()).
    RecordTable exportModel(SqlQueryModel* model, const RecordExportSpec& spec = {});

    // Exports caller-supplied rows of `model` (in the model's column order),
    // e.g. rows a plugin query selected with its own WHERE clause.
    RecordTable exportRows(SqlQueryModel* model, const QList<QVector<QVariant>>& rows,
                           const RecordExportSpec& spec = {});

    // Number of SQL statements issued by the last export (for benchmarks).
    int queryCount() const { return m_queries; }

private:
    RecordTable exportRowsAt(SqlQueryModel* model, const QList<QVector<QVariant>>& rows,
                             const RecordExportSpec& spec, int depth);
    QHash<QString, QString> resolveLookups(SqlQueryModel* model, int column,
                                           const QList<QVector<QVariant>>& rows);

    DatabaseObjects* m_dbo;
    int m_queries = 0;
};

// XML <-> RecordTree. The XML format is unchanged; see
// docs/PluginsOverview/ProjectNotesXML.md.
namespace RecordTreeXml {
    // Builds the <table> element for an exported table, formatting values by
    // column type (dates, money, percentages) and writing HTML as CDATA.
    QDomElement tableToElement(QDomDocument* doc, const RecordTable& table);
    RecordRow rowFromElement(const QDomElement& rowElement);
    RecordTable tableFromNode(const QDomNode& tableNode);
    RecordTree treeFromDocument(const QDomDocument& document);
}

namespace RecordTreeImport {
    // Every table named `name` anywhere in the tree, in document order.
    QList<const RecordTable*> tablesNamed(const RecordTree& tree, const QString& name);
    // Stored table names in the order imports must apply them.
    const QStringList& tableOrder();
}

#endif // RECORDTREE_H
