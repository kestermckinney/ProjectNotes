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
    Q_OBJECT
public:
    SortFilterProxyModel(QObject* parent = nullptr);
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int tRole) const override;

    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    void setSourceModel(QAbstractItemModel* sourceModel) override;

    void setQuickSearch(const QString& text);
    QString quickSearch() const { return m_quickSearch; }

    void setPinnedRow(int sourceRow);
    void releasePinnedRow();

private slots:
    void onSourceDataChanged(const QModelIndex& topLeft,
                             const QModelIndex& bottomRight,
                             const QList<int>& roles);

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

    int           m_sortColumn      = -1;
    Qt::SortOrder m_sortOrder       = Qt::AscendingOrder;
    int           m_pinnedSourceRow = -1;
    bool          m_pendingSort     = false;
};

#endif // SORTFILTERPROXYMODEL_H
