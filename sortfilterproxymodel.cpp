// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "sortfilterproxymodel.h"
#include "sqlquerymodel.h"
#include "databaseobjects.h"
#include <QSqlQuery>

SortFilterProxyModel::SortFilterProxyModel(QObject *parent): QSortFilterProxyModel(parent)
{
    m_quickSearchDebounce.setSingleShot(true);
    m_quickSearchDebounce.setInterval(250);
    QObject::connect(&m_quickSearchDebounce, &QTimer::timeout, this, [this]() {
        invalidateRowsFilter();
    });
}

bool SortFilterProxyModel::filterAcceptsRow(int source_row,
                                  const QModelIndex &source_t_parent) const
{
    // Quick search: show row if any column value contains the search text.
    // Column 0 is always the record UUID — skip it to avoid UUID false-positives.
    if (!m_quickSearch.isEmpty()) {
        SqlQueryModel* src = static_cast<SqlQueryModel*>(sourceModel());
        const int colCount = src->columnCount();
        for (int col = 1; col < colCount; ++col) {
            const QModelIndex idx = src->index(source_row, col, source_t_parent);

            // For lookup columns resolve the display value (the FK stored in the
            // model is a UUID/ID — not what the delegate shows).
            const QString lookupTable = src->getLookupTable(col);
            QString displayVal;
            if (!lookupTable.isEmpty()) {
                const QString fkCol  = src->getLookupFkColumnName(col);
                const QString valCol = src->getLookupValueColumnName(col);
                const QString fkVal  = src->data(idx).toString();
                if (!fkVal.isEmpty()) {
                    const QString cacheKey = lookupTable + '\x1F' + fkCol + '\x1F' + valCol + '\x1F' + fkVal;
                    auto it = m_sortLookupCache.constFind(cacheKey);
                    if (it != m_sortLookupCache.constEnd()) {
                        displayVal = it.value();
                    } else {
                        const QString sql = QString("SELECT %1 FROM %2 WHERE %3 = '%4'")
                                                .arg(valCol, lookupTable, fkCol, fkVal);
                        QSqlQuery query(src->getDBOs()->getDb());
                        if (query.exec(sql) && query.next())
                            displayVal = query.value(0).toString();
                        m_sortLookupCache[cacheKey] = displayVal;
                    }
                }
            } else {
                displayVal = src->data(idx).toString();
            }

            if (displayVal.contains(m_quickSearch, Qt::CaseInsensitive))
                return true;
        }
        return false;
    }
    return true;
}

void SortFilterProxyModel::setQuickSearch(const QString& text)
{
    if (m_quickSearch == text)
        return;
    m_quickSearch = text;
    if (text.isEmpty()) {
        // Field cleared — show all rows immediately, no debounce.
        m_quickSearchDebounce.stop();
        invalidateRowsFilter();
    } else {
        m_quickSearchDebounce.start();
    }
}

QVariant SortFilterProxyModel::headerData(int section, Qt::Orientation orientation,
                                int role) const {
    return sourceModel()->headerData(section, orientation,
                                     role);
}

bool SortFilterProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    // get source models
    SqlQueryModel *sourcemodel_left = (SqlQueryModel*) sourceLeft.model();
    SqlQueryModel *sourcemodel_right = (SqlQueryModel*) sourceRight.model();

    SqlQueryModel::DBColumnType type_left = sourcemodel_left->getType(sourceLeft.column());

    // For lookup columns, resolve the display value and compare as strings (case-insensitive).
    const QString lookupTable = sourcemodel_left->getLookupTable(sourceLeft.column());
    if (!lookupTable.isEmpty())
    {
        const QString fkCol  = sourcemodel_left->getLookupFkColumnName(sourceLeft.column());
        const QString valCol = sourcemodel_left->getLookupValueColumnName(sourceLeft.column());

        auto resolveLookup = [&](SqlQueryModel *mdl, const QModelIndex &idx) -> QString {
            const QString fkVal = mdl->data(idx).toString();
            if (fkVal.isEmpty())
                return QString();
            // Use a cache to avoid repeated DB queries during sort.
            const QString cacheKey = lookupTable + '\x1F' + fkCol + '\x1F' + valCol + '\x1F' + fkVal;
            auto it = m_sortLookupCache.constFind(cacheKey);
            if (it != m_sortLookupCache.constEnd())
                return it.value();
            // Query the display value directly — avoids the write-lock in execute().
            const QString sql = QString("SELECT %1 FROM %2 WHERE %3 = '%4'")
                                    .arg(valCol, lookupTable, fkCol, fkVal);
            QSqlQuery query(mdl->getDBOs()->getDb());
            QString displayVal;
            if (query.exec(sql) && query.next())
                displayVal = query.value(0).toString();
            m_sortLookupCache[cacheKey] = displayVal;
            return displayVal;
        };

        const QString left_display  = resolveLookup(sourcemodel_left,  sourceLeft);
        const QString right_display = resolveLookup(sourcemodel_right, sourceRight);
        return QString::compare(left_display, right_display, Qt::CaseInsensitive) < 0;
    }

    // get raw values
    QVariant value_left  = sourcemodel_left->data(sourceLeft);
    QVariant value_right = sourcemodel_right->data(sourceRight);

    // convert to sort_table items
    sourcemodel_left->sqlEscape(value_left, type_left);
    sourcemodel_right->sqlEscape(value_right, type_left);

    // compare items
    if (type_left == SqlQueryModel::DBInteger ||
            type_left == SqlQueryModel::DBBool ||
            type_left == SqlQueryModel::DBPercent ||
            type_left == SqlQueryModel::DBReal ||
            type_left == SqlQueryModel::DBUSD)
        return value_left.toDouble() < value_right.toDouble();
    else if (type_left == SqlQueryModel::DBDate ||
             type_left == SqlQueryModel::DBDateTime)
        return value_left.toDouble() < value_right.toDouble();
    else
        return QString::compare(value_left.toString(), value_right.toString(), Qt::CaseInsensitive) < 0;
}
