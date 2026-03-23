// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "sortfilterproxymodel.h"
#include "databaseobjects.h"
#include <QSqlQuery>

SortFilterProxyModel::SortFilterProxyModel(QObject *parent): QSortFilterProxyModel(parent)
{

}

bool SortFilterProxyModel::filterAcceptsRow(int source_row,
                                  const QModelIndex &source_t_parent) const{
    Q_UNUSED(source_row);
    Q_UNUSED(source_t_parent);

    /*
    QModelIndex indG = sourceModel()->index(source_row,
                                               1, source_t_parent);
    QModelIndex indD = sourceModel()->index(source_row,
                                               2, source_t_parent);
    if(sourceModel()->data(indG).toDouble() < m_minGravity ||
            sourceModel()->data(indD).toDouble() < m_minDensity)
        return false;
        */
    return true;
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
