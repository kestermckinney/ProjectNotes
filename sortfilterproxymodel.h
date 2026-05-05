// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QHash>
#include <QObject>
#include <QTimer>

class SortFilterProxyModel : public QSortFilterProxyModel
{
public:
    SortFilterProxyModel(QObject* parent = nullptr);
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int tRole) const override;

    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

    void setQuickSearch(const QString& text);
    QString quickSearch() const { return m_quickSearch; }

private:
    // Cache for lookup display values — avoids repeated DB queries during sort.
    // Key: "table\x1Ffkcol\x1Fvalcol\x1FfkValue", Value: display string.
    mutable QHash<QString, QString> m_sortLookupCache;
    QString m_quickSearch;
    // Coalesces bursts of setQuickSearch() calls (one per keystroke) into a
    // single invalidateRowsFilter() so we don't re-scan every row of the
    // source model on every keystroke. Empty text invalidates immediately so
    // clearing the field stays instant.
    QTimer m_quickSearchDebounce;
};

#endif // SORTFILTERPROXYMODEL_H
