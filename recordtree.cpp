// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "recordtree.h"

#include "databaseobjects.h"
#include "sqlquerymodel.h"

#include <QDomDocument>
#include <QDomElement>
#include <QScopedPointer>
#include <QSqlError>
#include <QSqlQuery>

#include "QLogger.h"

using namespace QLogger;

// ── RecordRow ────────────────────────────────────────────────────────────────

const RecordColumn* RecordRow::column(const QString& name) const
{
    for (const RecordColumn& c : columns)
        if (c.name == name)
            return &c;
    return nullptr;
}

QVariant RecordRow::value(const QString& name) const
{
    const RecordColumn* c = column(name);
    return c ? c->value : QVariant();
}

// ── RecordTreeExporter ───────────────────────────────────────────────────────

namespace {

QString quoteList(const QStringList& values)
{
    QStringList quoted;
    quoted.reserve(values.size());
    for (const QString& v : values)
        quoted.append(QLatin1Char('\'') + SqlQueryModel::sqlEscapeLiteral(v) + QLatin1Char('\''));
    return quoted.join(QLatin1Char(','));
}

} // namespace

RecordTable RecordTreeExporter::exportModel(SqlQueryModel* model, const RecordExportSpec& spec)
{
    m_queries = 0;
    if (!model || !model->isExportable())
        return {};
    if (spec.includeTable && !spec.includeTable(model->tablename()))
        return {};
    return exportRowsAt(model, model->m_cache, spec, 0);
}

RecordTable RecordTreeExporter::exportRows(SqlQueryModel* model, const QList<QVector<QVariant>>& rows,
                                           const RecordExportSpec& spec)
{
    m_queries = 0;
    if (!model)
        return {};
    return exportRowsAt(model, rows, spec, 0);
}

QHash<QString, QString> RecordTreeExporter::resolveLookups(SqlQueryModel* model, int column,
                                                           const QList<QVector<QVariant>>& rows)
{
    // One query per foreign-key column for every row at this level. The
    // per-cell form it replaces ran `select value from table where fk = 'x'`
    // and took the first row; keep that rule (first match wins, no deleted
    // filter) so exported lookup values are unchanged.
    QHash<QString, QString> result;
    QStringList keys;
    QSet<QString> seen;
    for (const QVector<QVariant>& row : rows) {
        const QString key = row.value(column).toString();
        if (key.isEmpty() || seen.contains(key))
            continue;
        seen.insert(key);
        keys.append(key);
    }
    if (keys.isEmpty())
        return result;

    const QString fk = model->getLookupFkColumnName(column);
    const QString valueColumn = model->getLookupValueColumnName(column);
    const QString table = model->getLookupTable(column);

    // Chunk to stay well below SQLite's expression-depth and statement limits.
    constexpr int kChunk = 500;
    for (int start = 0; start < keys.size(); start += kChunk) {
        const QString sql = QStringLiteral("select %1, %2 from %3 where %1 in (%4)")
            .arg(fk, valueColumn, table, quoteList(keys.mid(start, kChunk)));
        ++m_queries;
        DB_LOCK;
        QSqlQuery query(m_dbo->getDb());
        query.setForwardOnly(true);
        if (!query.exec(sql)) {
            QLog_Error(ERRORLOG, QStringLiteral("Lookup query failed: %1\nSQL: %2").arg(query.lastError().text(), sql));
        } else {
            while (query.next()) {
                const QString key = query.value(0).toString();
                if (!result.contains(key))
                    result.insert(key, query.value(1).toString());
            }
        }
        DB_UNLOCK;
    }
    return result;
}

RecordTable RecordTreeExporter::exportRowsAt(SqlQueryModel* model, const QList<QVector<QVariant>>& rows,
                                             const RecordExportSpec& spec, int depth)
{
    RecordTable table;
    table.name = model->tablename();

    const int columnCount = model->columnCount();
    QList<int> columns;
    const auto fieldsIt = spec.fields.constFind(table.name);
    for (int c = 0; c < columnCount; ++c) {
        if (fieldsIt != spec.fields.constEnd() && c != 0 && !fieldsIt->contains(model->getColumnName(c)))
            continue;
        columns.append(c);
    }

    // Lookup display values, batched per column.
    QHash<int, QHash<QString, QString>> lookups;
    if (spec.resolveLookups) {
        for (int c : std::as_const(columns))
            if (!model->getLookupTable(c).isEmpty())
                lookups.insert(c, resolveLookups(model, c, rows));
    }

    table.rows.reserve(rows.size());
    for (const QVector<QVariant>& row : rows) {
        RecordRow out;
        out.columns.reserve(columns.size());
        for (int c : std::as_const(columns)) {
            RecordColumn column;
            column.name = model->getColumnName(c);
            column.value = row.value(c);
            column.type = model->getType(c);
            const auto lookup = lookups.constFind(c);
            if (lookup != lookups.constEnd()) {
                const QString display = lookup->value(column.value.toString());
                if (!display.isEmpty())
                    column.lookup = display;
            }
            out.columns.append(column);
        }
        table.rows.append(out);
    }

    if (spec.maxDepth >= 0 && depth >= spec.maxDepth)
        return table;

    // Exportable child relations: one query per relation for all parent rows
    // at this level, then rows are grouped back to their parents in memory.
    for (int i = 0; i < model->m_relatedTable.size(); ++i) {
        if (model->m_relationExportable.at(i) != SqlQueryModel::DBExportable)
            continue;
        const QString childName = model->m_relatedTable.at(i);
        if (spec.includeTable && !spec.includeTable(childName))
            continue;

        QScopedPointer<SqlQueryModel> child(m_dbo->createExportObject(childName));
        if (!child || !child->isExportable())
            continue;

        const QStringList childColumns = model->m_relatedColumns.at(i);
        const QStringList parentColumns = model->m_relatedFkColumns.at(i);
        if (childColumns.size() != 1 || parentColumns.size() != 1) {
            QLog_Error(ERRORLOG, QStringLiteral("Export of multi-column relation %1 -> %2 is not supported.")
                                     .arg(table.name, childName));
            continue;
        }
        const int childColumn = child->getColumnNumber(childColumns.first());
        const int parentColumn = model->getColumnNumber(parentColumns.first());
        if (childColumn < 0 || parentColumn < 0)
            continue;

        QStringList parentKeys;
        QSet<QString> seen;
        for (const QVector<QVariant>& row : rows) {
            const QString key = row.value(parentColumn).toString();
            if (!key.isEmpty() && !seen.contains(key.toLower())) {
                seen.insert(key.toLower());
                parentKeys.append(key);
            }
        }

        QList<QVector<QVariant>> childRows;
        if (!parentKeys.isEmpty()) {
            child->setFilter(childColumn, parentKeys.join(QLatin1Char(',')), SqlQueryModel::In);
            ++m_queries;
            child->refresh();
            childRows = child->m_cache;
        }

        const RecordTable exported = exportRowsAt(child.data(), childRows, spec, depth + 1);

        // Group child rows by the foreign key, preserving the child query order.
        // Keys compare case-insensitively, as the WHERE clause did.
        QHash<QString, QList<int>> byParent;
        for (int r = 0; r < childRows.size(); ++r)
            byParent[childRows.at(r).value(childColumn).toString().toLower()].append(r);

        for (int p = 0; p < rows.size(); ++p) {
            const QString key = rows.at(p).value(parentColumn).toString();
            RecordTable childTable;
            childTable.name = childName;
            childTable.filters.append({childColumns.first(), key});
            for (int r : byParent.value(key.toLower()))
                childTable.rows.append(exported.rows.at(r));
            table.rows[p].children.append(childTable);
        }
    }

    return table;
}

// ── XML codec ────────────────────────────────────────────────────────────────

namespace RecordTreeXml {

QDomElement tableToElement(QDomDocument* doc, const RecordTable& table)
{
    QDomElement xmltable = doc->createElement(QStringLiteral("table"));
    xmltable.setAttribute(QStringLiteral("name"), table.name);
    for (int i = 0; i < table.filters.size(); ++i) {
        xmltable.setAttribute(QStringLiteral("filter_field_%1").arg(i + 1), table.filters.at(i).first);
        xmltable.setAttribute(QStringLiteral("filter_value_%1").arg(i + 1), table.filters.at(i).second);
    }

    for (const RecordRow& row : table.rows) {
        QDomElement xmlrow = doc->createElement(QStringLiteral("row"));
        if (row.deleted)
            xmlrow.setAttribute(QStringLiteral("delete"), QStringLiteral("true"));

        for (const RecordColumn& column : row.columns) {
            QDomElement xmlcolumn = doc->createElement(QStringLiteral("column"));
            xmlcolumn.setAttribute(QStringLiteral("name"), column.name);

            QVariant value = column.value;
            SqlQueryModel::reformatValue(value, static_cast<SqlQueryModel::DBColumnType>(column.type));
            const QString text = SqlQueryModel::removeInvalidXmlCharacters(value.toString());
            if (column.type == SqlQueryModel::DBHtml)
                xmlcolumn.appendChild(doc->createCDATASection(text));
            else
                xmlcolumn.appendChild(doc->createTextNode(text));

            if (!column.lookup.isNull())
                xmlcolumn.setAttribute(QStringLiteral("lookupvalue"), column.lookup);
            xmlrow.appendChild(xmlcolumn);
        }

        for (const RecordTable& child : row.children)
            xmlrow.appendChild(tableToElement(doc, child));

        xmltable.appendChild(xmlrow);
    }
    return xmltable;
}

RecordRow rowFromElement(const QDomElement& rowElement)
{
    RecordRow row;
    row.deleted = rowElement.attribute(QStringLiteral("delete")).compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;

    for (QDomNode node = rowElement.firstChild(); !node.isNull(); node = node.nextSibling()) {
        const QDomElement element = node.toElement();
        if (element.isNull())
            continue;
        if (element.tagName() == QLatin1String("column")) {
            RecordColumn column;
            column.name = element.attribute(QStringLiteral("name"));
            column.value = element.text();
            // attribute() returns a null string when absent; the importer
            // treats null as "no lookup", so keep that distinction.
            column.lookup = element.attribute(QStringLiteral("lookupvalue"));
            row.columns.append(column);
        } else if (element.tagName() == QLatin1String("table")) {
            row.children.append(tableFromNode(element));
        }
    }
    return row;
}

RecordTable tableFromNode(const QDomNode& tableNode)
{
    RecordTable table;
    const QDomElement element = tableNode.toElement();
    table.name = element.attribute(QStringLiteral("name"));
    for (int i = 1;; ++i) {
        const QString field = element.attribute(QStringLiteral("filter_field_%1").arg(i));
        if (field.isEmpty())
            break;
        table.filters.append({field, element.attribute(QStringLiteral("filter_value_%1").arg(i))});
    }
    for (QDomElement row = element.firstChildElement(QStringLiteral("row")); !row.isNull();
         row = row.nextSiblingElement(QStringLiteral("row")))
        table.rows.append(rowFromElement(row));
    return table;
}

RecordTree treeFromDocument(const QDomDocument& document)
{
    RecordTree tree;
    const QDomElement root = document.documentElement();
    const QDomNamedNodeMap attributes = root.attributes();
    for (int i = 0; i < attributes.count(); ++i)
        tree.attributes.append({attributes.item(i).nodeName(), attributes.item(i).nodeValue()});
    for (QDomElement table = root.firstChildElement(QStringLiteral("table")); !table.isNull();
         table = table.nextSiblingElement(QStringLiteral("table")))
        tree.tables.append(tableFromNode(table));
    return tree;
}

} // namespace RecordTreeXml

// ── Collecting tables for import ─────────────────────────────────────────────

namespace {

void collectTables(const QList<RecordTable>& tables, const QString& name, QList<const RecordTable*>& out)
{
    // Document pre-order, matching DatabaseObjects::findTableNodes().
    for (const RecordTable& table : tables) {
        if (table.name == name)
            out.append(&table);
        for (const RecordRow& row : table.rows)
            collectTables(row.children, name, out);
    }
}

} // namespace

QList<const RecordTable*> RecordTreeImport::tablesNamed(const RecordTree& tree, const QString& name)
{
    QList<const RecordTable*> out;
    collectTables(tree.tables, name, out);
    return out;
}

const QStringList& RecordTreeImport::tableOrder()
{
    // Parents before the children that refer to them.
    static const QStringList order {
        QStringLiteral("clients"), QStringLiteral("people"), QStringLiteral("projects"),
        QStringLiteral("project_email_audiences"), QStringLiteral("project_people"),
        QStringLiteral("status_report_items"), QStringLiteral("project_locations"),
        QStringLiteral("project_notes"), QStringLiteral("item_tracker"),
        QStringLiteral("meeting_attendees"), QStringLiteral("item_tracker_updates")
    };
    return order;
}
