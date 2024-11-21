// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pnsortfilterproxymodel.h"
#include "pndatabaseobjects.h"

PNSortFilterProxyModel::PNSortFilterProxyModel(QObject *t_parent): QSortFilterProxyModel(t_parent)
{

}

bool PNSortFilterProxyModel::filterAcceptsRow(int source_row,
                                  const QModelIndex &source_t_parent) const{
    Q_UNUSED(source_row);
    Q_UNUSED(source_t_parent);

    /*
    QModelIndex indG = sourceModel()->t_index(source_row,
                                               1, source_t_parent);
    QModelIndex indD = sourceModel()->t_index(source_row,
                                               2, source_t_parent);
    if(sourceModel()->data(indG).toDouble() < m_minGravity ||
            sourceModel()->data(indD).toDouble() < m_minDensity)
        return false;
        */
    return true;
}

QVariant PNSortFilterProxyModel::headerData(int t_section, Qt::Orientation t_orientation,
                                int t_role) const {
    return sourceModel()->headerData(t_section, t_orientation,
                                     t_role);
}

bool PNSortFilterProxyModel::lessThan(const QModelIndex &t_source_left, const QModelIndex &t_source_right) const
{
    // get source models
    PNSqlQueryModel *sourcemodel_left = (PNSqlQueryModel*) t_source_left.model();
    PNSqlQueryModel *sourcemodel_right = (PNSqlQueryModel*) t_source_right.model();

    // get base values
    QVariant value_left;
    QVariant value_right;

    PNSqlQueryModel::DBColumnType type_left = sourcemodel_left->getType(t_source_left.column());
    PNSqlQueryModel::DBColumnType type_right = sourcemodel_right->getType(t_source_right.column());

    // if it wasn't a lookup t_value then use the data out of the model
    if (!value_left.isValid())
        value_left = sourcemodel_left->data(t_source_left);
    else
        type_left = PNSqlQueryModel::DBString;

    if (!value_right.isValid())
        value_right = sourcemodel_right->data(t_source_right);
    else
        type_right = PNSqlQueryModel::DBString;

    // convert to sort_table items
    sourcemodel_left->sqlEscape(value_left, type_left);
    sourcemodel_right->sqlEscape(value_right, type_right);

    // compare items
    if (type_left == PNSqlQueryModel::DBInteger ||
            type_left == PNSqlQueryModel::DBBool ||
            type_left == PNSqlQueryModel::DBPercent ||
            type_left == PNSqlQueryModel::DBReal ||
            type_left == PNSqlQueryModel::DBUSD)
        return value_left.toDouble() < value_right.toDouble();
    else if (type_left == PNSqlQueryModel::DBDate ||
             type_left == PNSqlQueryModel::DBDateTime)
        return value_left.toDouble() < value_right.toDouble();
    else
        return value_left.toString() < value_right.toString();
}
