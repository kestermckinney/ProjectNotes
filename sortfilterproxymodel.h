// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QObject>

class SortFilterProxyModel : public QSortFilterProxyModel
{
public:
    SortFilterProxyModel(QObject* parent = 0);
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int tRole) const override;

    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;
};

#endif // SORTFILTERPROXYMODEL_H
