// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QHash>
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

private:
    // Cache for lookup display values — avoids repeated DB queries during sort.
    // Key: "table\x1Ffkcol\x1Fvalcol\x1FfkValue", Value: display string.
    mutable QHash<QString, QString> m_sortLookupCache;
};

#endif // SORTFILTERPROXYMODEL_H
