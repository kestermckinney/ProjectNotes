// SPDX-License-Identifier: GPL-3.0-only
#include "modelviewstate.h"
#include "databaseobjects.h"
#include "sortfilterproxymodel.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QSet>
#include <QDebug>
#include <algorithm>

namespace PN::ModelViewState {
SqlQueryModel* sourceModelOf(QAbstractItemModel* model)
{
    if (auto* proxy = qobject_cast<SortFilterProxyModel*>(model))
        return qobject_cast<SqlQueryModel*>(proxy->sourceModel());
    return nullptr;
}

QVariantList filterColumns(QAbstractItemModel* model)
{
    QVariantList out;
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return out;

    const int cols = src->columnCount();
    for (int c = 1; c < cols; ++c) {          // column 0 is the hidden id
        if (!src->isSearchable(c)) continue;
        const SqlQueryModel::DBColumnType t = src->getType(c);
        QVariantMap m;
        m["field"]  = src->getColumnName(c);
        m["label"]  = src->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
        m["isDate"] = (t == SqlQueryModel::DBDate || t == SqlQueryModel::DBDateTime);
        out.append(m);
    }
    return out;
}

QVariantList columnDistinctValues(QAbstractItemModel* model,
                                                       const QString& field)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return {};
    const int col = src->getColumnNumber(field);
    if (col < 0) return {};

    // Pull the distinct values from the base data rather than from the model's
    // currently-loaded rows. If we read the live rows, then any filter the user
    // has already applied has narrowed the model, so reopening the Filter dialog
    // would only offer the values that survived the filter — you could no longer
    // pick a different value. constructWhereClause(false) keeps the model's
    // built-in filters (context filter, deleted-row filter) but drops the
    // interactive user filter.
    const QString dbcol = src->getColumnName(col);
    QString where = src->constructWhereClause(false);   // "" or " WHERE ... "
    where = where.isEmpty() ? QStringLiteral(" WHERE ") : (where + QStringLiteral(" AND "));

    const QString sql = "SELECT DISTINCT " + dbcol + " FROM ( " + src->BaseSQL()
                        + where + dbcol + " IS NOT NULL )";

    const SqlQueryModel::DBColumnType type = src->getType(col);

    // Foreign-key columns (e.g. client_id) store the id as the filterable
    // value, but the Widgets dialog shows the id resolved through its lookup
    // table's delegate (e.g. client_name) — reproduce that here explicitly
    // since QML has no per-column paint delegate to fall back on.
    const QString lookupTable  = src->getLookupTable(col);
    const QString lookupFkCol  = src->getLookupFkColumnName(col);
    const QString lookupValCol = src->getLookupValueColumnName(col);

    QVariantList values;
    QSet<QString> seen;
    QSqlQuery q(src->getDBOs()->getDb());
    if (q.exec(sql)) {
        while (q.next() && values.size() < 500) {
            QVariant raw = q.value(0);
            src->reformatValue(raw, type);   // match the grid's display formatting
            const QString v = raw.toString().trimmed();
            if (v.isEmpty() || seen.contains(v)) continue;
            seen.insert(v);

            QString label = v;
            if (!lookupTable.isEmpty()) {
                QSqlQuery lq(src->getDBOs()->getDb());
                lq.prepare(QString("SELECT %1 FROM %2 WHERE %3 = ?")
                               .arg(lookupValCol, lookupTable, lookupFkCol));
                lq.addBindValue(v);
                if (lq.exec() && lq.next())
                    label = lq.value(0).toString();
            }

            QVariantMap m;
            m["value"] = v;
            m["label"] = label;
            values.append(m);
        }
    } else {
        qWarning() << "columnDistinctValues query failed:" << q.lastError().text() << sql;
    }

    std::sort(values.begin(), values.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("label").toString().compare(
                   b.toMap().value("label").toString(), Qt::CaseInsensitive) < 0;
    });
    return values;
}

static QString settingsScopeForModel(SqlQueryModel* src)
{
    QString scope = src->tablename();
    if (src == global_DBObjects.trackeritemsmodel())
        scope += ":project";
    return scope;
}

static QString columnFilterSettingKey(SqlQueryModel* src)
{
    return "UI:ColumnFilters:" + settingsScopeForModel(src);
}

static void applyFilterSpecsToModel(SqlQueryModel* src, const QVariantList& specs)
{
    src->clearAllUserSearches();

    for (const QVariant& sv : specs) {
        const QVariantMap spec = sv.toMap();
        const int col = src->getColumnNumber(spec.value("field").toString());
        if (col < 0) continue;

        const QVariantList values = spec.value("values").toList();
        if (!values.isEmpty())
            src->setUserFilter(col, values);

        const QString search = spec.value("search").toString().trimmed();
        if (!search.isEmpty())
            src->setUserSearchString(col, search);

        const QString start = spec.value("rangeStart").toString().trimmed();
        const QString end   = spec.value("rangeEnd").toString().trimmed();
        if (!start.isEmpty() || !end.isEmpty())
            src->setUserSearchRange(col, start, end);
    }
}

void applyColumnFilters(QAbstractItemModel* model,
                                              const QVariantList& specs)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return;

    applyFilterSpecsToModel(src, specs);
    src->activateUserFilter();

    const QByteArray json = QJsonDocument(QJsonArray::fromVariantList(specs)).toJson(QJsonDocument::Compact);
    global_DBObjects.saveParameter(columnFilterSettingKey(src), QString::fromUtf8(json));
}

void clearColumnFilters(QAbstractItemModel* model)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return;
    src->clearAllUserSearches();
    src->deactivateUserFilter();
    global_DBObjects.saveParameter(columnFilterSettingKey(src), QString());
}

QVariantList activeColumnFilters(QAbstractItemModel* model)
{
    QVariantList specs;
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return specs;

    const int cols = src->columnCount();
    for (int c = 0; c < cols; ++c) {
        if (!src->hasUserFilters(c)) continue;

        QVariantMap spec;
        spec["field"] = src->getColumnName(c);
        spec["values"] = src->getUserFilter(c);
        const QVariant search = src->getUserSearchString(c);
        spec["search"] = search.isValid() ? search.toString() : QString();

        QVariant start, end;
        src->getUserSearchRange(c, start, end);
        spec["rangeStart"] = start.isValid() ? start.toString() : QString();
        spec["rangeEnd"]   = end.isValid()   ? end.toString()   : QString();

        specs.append(spec);
    }
    return specs;
}

void applyQuickFilter(QAbstractItemModel* model, const QString& field,
                                            const QVariantList& values)
{
    QVariantList specs = activeColumnFilters(model);

    // Remove whatever's currently set for this field (if any) — either to
    // replace it below, or, if it already matched exactly, to leave it
    // removed (toggle-off: clicking an already-active Quick Filter again
    // clears just that one).
    bool wasSameValues = false;
    for (int i = specs.size() - 1; i >= 0; --i) {
        const QVariantMap spec = specs.at(i).toMap();
        if (spec.value("field").toString() != field) continue;
        if (spec.value("values").toList() == values)
            wasSameValues = true;
        specs.removeAt(i);
    }

    if (!wasSameValues) {
        QVariantMap spec;
        spec["field"] = field;
        spec["values"] = values;
        spec["search"] = QString();
        spec["rangeStart"] = QString();
        spec["rangeEnd"] = QString();
        specs.append(spec);
    }

    applyColumnFilters(model, specs);   // also bumps filterRev + persists
}

bool hasActiveColumnFilters(QAbstractItemModel* model)
{
    SqlQueryModel* src = sourceModelOf(model);
    return src && src->hasUserFilters();
}

QVariantList sortColumns(QAbstractItemModel* model)
{
    QVariantList out;
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return out;

    const int cols = src->columnCount();
    for (int c = 1; c < cols; ++c) {          // column 0 is the hidden id
        if (src->getType(c) == SqlQueryModel::DBHtml) continue;   // long free text
        QVariantMap m;
        m["field"] = src->getColumnName(c);
        m["label"] = src->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
        out.append(m);
    }
    return out;
}

static QString sortSettingKey(SqlQueryModel* src)
{
    return "UI:SortOrder:" + settingsScopeForModel(src);
}

void applySort(QAbstractItemModel* model, const QString& field, bool descending)
{
    auto* proxy = qobject_cast<SortFilterProxyModel*>(model);
    SqlQueryModel* src = sourceModelOf(model);
    if (!proxy || !src) return;

    const int col = src->getColumnNumber(field);
    if (col < 0) return;

    proxy->sort(col, descending ? Qt::DescendingOrder : Qt::AscendingOrder);

    QVariantMap spec;
    spec["field"] = field;
    spec["descending"] = descending;
    const QByteArray json = QJsonDocument(QJsonObject::fromVariantMap(spec)).toJson(QJsonDocument::Compact);
    global_DBObjects.saveParameter(sortSettingKey(src), QString::fromUtf8(json));
}

void clearSort(QAbstractItemModel* model)
{
    auto* proxy = qobject_cast<SortFilterProxyModel*>(model);
    SqlQueryModel* src = sourceModelOf(model);
    if (!proxy || !src) return;

    proxy->sort(-1);   // restores the model's base ORDER BY
    global_DBObjects.saveParameter(sortSettingKey(src), QString());
}

QVariantMap activeSort(QAbstractItemModel* model)
{
    SqlQueryModel* src = sourceModelOf(model);
    if (!src) return {};
    const QString json = global_DBObjects.loadParameter(sortSettingKey(src));
    if (json.isEmpty()) return {};
    return QJsonDocument::fromJson(json.toUtf8()).object().toVariantMap();
}

void restoreSort(QAbstractItemModel* model)
{
    auto* proxy = qobject_cast<SortFilterProxyModel*>(model);
    SqlQueryModel* src = sourceModelOf(model);
    if (!proxy || !src) return;

    const QString json = global_DBObjects.loadParameter(sortSettingKey(src));
    if (json.isEmpty()) return;

    const QVariantMap spec = QJsonDocument::fromJson(json.toUtf8()).object().toVariantMap();
    const int col = src->getColumnNumber(spec.value("field").toString());
    if (col < 0) return;

    proxy->sort(col, spec.value("descending").toBool() ? Qt::DescendingOrder : Qt::AscendingOrder);
}

void restoreColumnFilters(SqlQueryModel* src)
{
    const QString json = global_DBObjects.loadParameter(columnFilterSettingKey(src));
    if (json.isEmpty())
        return;

    const QVariantList specs = QJsonDocument::fromJson(json.toUtf8()).array().toVariantList();
    if (specs.isEmpty())
        return;

    applyFilterSpecsToModel(src, specs);
    src->activateUserFilter();
}
}
