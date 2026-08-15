// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "sortfilterproxymodel.h"
#include "sqlquerymodel.h"
#include "databaseobjects.h"
#include <QSqlQuery>

SortFilterProxyModel::SortFilterProxyModel(QObject *parent): QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(false);

    m_quickSearchDebounce.setSingleShot(true);
    m_quickSearchDebounce.setInterval(250);
    QObject::connect(&m_quickSearchDebounce, &QTimer::timeout, this, [this]() {
        invalidateRowsFilter();
    });

    // Coalesces a burst of source dataChanged signals (multi-cell saves, a sync
    // cycle) into a single re-sort per event-loop turn instead of one whole-
    // model sort per changed cell.
    m_resortDebounce.setSingleShot(true);
    m_resortDebounce.setInterval(0);
    QObject::connect(&m_resortDebounce, &QTimer::timeout, this, [this]() {
        if (m_sortColumn >= 0) {
            preloadCurrentSortColumn();
            QSortFilterProxyModel::sort(m_sortColumn, m_sortOrder);
        }
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
                    displayVal = resolveLookupValue(lookupTable, fkCol, valCol, fkVal, src);
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

void SortFilterProxyModel::preloadLookupTable(const QString& lookupTable, const QString& fkCol,
                                              const QString& valCol, SqlQueryModel* srcModel) const
{
    // Bulk-load an entire FK->display mapping in one query instead of firing one
    // query per distinct value encountered while sorting/filtering a large list
    // (e.g. every unique assigned_to/project on the master item list).
    const QString tableKey = lookupTable + '\x1F' + fkCol + '\x1F' + valCol;
    if (m_preloadedLookupTables.contains(tableKey))
        return;
    m_preloadedLookupTables.insert(tableKey);

    const QString sql = QString("SELECT %1, %2 FROM %3").arg(fkCol, valCol, lookupTable);
    QSqlQuery query(srcModel->getDBOs()->getDb());
    if (query.exec(sql)) {
        while (query.next()) {
            const QString fkVal = query.value(0).toString();
            const QString cacheKey = tableKey + '\x1F' + fkVal;
            m_sortLookupCache[cacheKey] = query.value(1).toString();
        }
    }
}

QString SortFilterProxyModel::resolveLookupValue(const QString& lookupTable, const QString& fkCol,
                                                  const QString& valCol, const QString& fkVal,
                                                  SqlQueryModel* srcModel) const
{
    preloadLookupTable(lookupTable, fkCol, valCol, srcModel);

    const QString cacheKey = lookupTable + '\x1F' + fkCol + '\x1F' + valCol + '\x1F' + fkVal;
    auto it = m_sortLookupCache.constFind(cacheKey);
    if (it != m_sortLookupCache.constEnd())
        return it.value();

    // Not in the bulk-loaded snapshot — value was added after the preload
    // (or the snapshot missed it). Fall back to a single-row lookup and cache it.
    const QString sql = QString("SELECT %1 FROM %2 WHERE %3 = '%4'")
                            .arg(valCol, lookupTable, fkCol, fkVal);
    QSqlQuery query(srcModel->getDBOs()->getDb());
    QString displayVal;
    if (query.exec(sql) && query.next())
        displayVal = query.value(0).toString();
    m_sortLookupCache[cacheKey] = displayVal;
    return displayVal;
}

void SortFilterProxyModel::preloadCurrentSortColumn() const
{
    if (m_sortColumn < 0)
        return;
    SqlQueryModel* src = static_cast<SqlQueryModel*>(sourceModel());
    if (!src)
        return;
    const QString lookupTable = src->getLookupTable(m_sortColumn);
    if (lookupTable.isEmpty())
        return;   // not a lookup column — lessThan() compares raw values, no DB access needed
    preloadLookupTable(lookupTable, src->getLookupFkColumnName(m_sortColumn),
                        src->getLookupValueColumnName(m_sortColumn), src);
}

void SortFilterProxyModel::invalidateLookupTable(const QString& table)
{
    const QString prefix = table + '\x1F';

    bool hadTable = false;
    for (auto it = m_preloadedLookupTables.begin(); it != m_preloadedLookupTables.end(); ) {
        if (it->startsWith(prefix)) {
            it = m_preloadedLookupTables.erase(it);
            hadTable = true;
        } else {
            ++it;
        }
    }
    if (!hadTable)
        return;

    for (auto it = m_sortLookupCache.begin(); it != m_sortLookupCache.end(); ) {
        if (it.key().startsWith(prefix))
            it = m_sortLookupCache.erase(it);
        else
            ++it;
    }
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

void SortFilterProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    if (QAbstractItemModel* old = this->sourceModel())
        disconnect(old, &QAbstractItemModel::dataChanged,
                   this, &SortFilterProxyModel::onSourceDataChanged);
    QSortFilterProxyModel::setSourceModel(sourceModel);
    if (sourceModel)
        connect(sourceModel, &QAbstractItemModel::dataChanged,
                this, &SortFilterProxyModel::onSourceDataChanged,
                Qt::UniqueConnection);
}

void SortFilterProxyModel::sort(int column, Qt::SortOrder order)
{
    m_sortColumn  = column;
    m_sortOrder   = order;
    m_pendingSort = false;
    preloadCurrentSortColumn();
    QSortFilterProxyModel::sort(column, order);
}

void SortFilterProxyModel::onSourceDataChanged(const QModelIndex& topLeft,
                                               const QModelIndex& bottomRight,
                                               const QList<int>& roles)
{
    Q_UNUSED(roles)
    if (m_sortColumn < 0)
        return;

    // Only a change in the sort column can reorder rows — skip the re-sort for
    // edits to other columns (setData emits single-cell ranges; full-row
    // reloads span every column and still fall through).
    if (topLeft.column() > m_sortColumn || bottomRight.column() < m_sortColumn)
        return;

    if (m_pinnedSourceRow >= 0
        && m_pinnedSourceRow >= topLeft.row()
        && m_pinnedSourceRow <= bottomRight.row())
    {
        m_pendingSort = true;
        return;
    }
    m_resortDebounce.start();
}

void SortFilterProxyModel::setPinnedRow(int sourceRow)
{
    if (m_pinnedSourceRow >= 0 && m_pendingSort) {
        m_pendingSort = false;
        preloadCurrentSortColumn();
        QSortFilterProxyModel::sort(m_sortColumn, m_sortOrder);
    }
    m_pinnedSourceRow = sourceRow;
}

void SortFilterProxyModel::releasePinnedRow()
{
    m_pinnedSourceRow = -1;
    if (m_pendingSort && m_sortColumn >= 0) {
        m_pendingSort = false;
        preloadCurrentSortColumn();
        QSortFilterProxyModel::sort(m_sortColumn, m_sortOrder);
    }
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
            return resolveLookupValue(lookupTable, fkCol, valCol, fkVal, mdl);
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
