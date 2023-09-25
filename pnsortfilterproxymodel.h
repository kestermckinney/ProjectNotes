// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNSORTFILTERPROXYMODEL_H
#define PNSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QObject>

class PNSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    PNSortFilterProxyModel(QObject* t_parent = 0);
    bool filterAcceptsRow(int t_source_row,
                          const QModelIndex &t_source_parent) const override;
    QVariant headerData(int t_section, Qt::Orientation t_orientation,
                        int t_t_role) const override;

    bool lessThan(const QModelIndex &t_source_left, const QModelIndex &t_source_right) const override;
};

#endif // PNSORTFILTERPROXYMODEL_H
