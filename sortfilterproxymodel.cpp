// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "sortfilterproxymodel.h"
#include "databaseobjects.h"

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

    // get base values
    QVariant value_left;
    QVariant value_right;

    SqlQueryModel::DBColumnType type_left = sourcemodel_left->getType(sourceLeft.column());
    SqlQueryModel::DBColumnType type_right = sourcemodel_right->getType(sourceRight.column());

    // if it wasn't a lookup value then use the data out of the model
    if (!value_left.isValid())
        value_left = sourcemodel_left->data(sourceLeft);
    else
        type_left = SqlQueryModel::DBString;

    if (!value_right.isValid())
        value_right = sourcemodel_right->data(sourceRight);
    else
        type_right = SqlQueryModel::DBString;

    // convert to sort_table items
    sourcemodel_left->sqlEscape(value_left, type_left);
    sourcemodel_right->sqlEscape(value_right, type_right);

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
        return value_left.toString() < value_right.toString();
}
