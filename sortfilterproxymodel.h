// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QHash>
#include <QObject>
#include <QSet>
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

    // Drop cached display values sourced from the given lookup table so the
    // next sort/filter pass re-reads them. Called for every table write (app
    // edits, plugins, sync) — without this a renamed person/client keeps its
    // old name in sort/quick-search for the life of the session.
    void invalidateLookupTable(const QString& table);

private slots:
    void onSourceDataChanged(const QModelIndex& topLeft,
                             const QModelIndex& bottomRight,
                             const QList<int>& roles);

private:
    // Cache for lookup display values — avoids repeated DB queries during sort.
    // Key: "table\x1Ffkcol\x1Fvalcol\x1FfkValue", Value: display string.
    mutable QHash<QString, QString> m_sortLookupCache;
    // Tables already bulk-loaded into m_sortLookupCache via preloadLookupTable(),
    // keyed by "table\x1Ffkcol\x1Fvalcol". Avoids re-querying row-by-row: the first
    // lookup against a given (table, fkCol, valCol) pulls every row in one query
    // instead of one query per distinct value hit during a sort/filter pass.
    mutable QSet<QString> m_preloadedLookupTables;
    void preloadLookupTable(const QString& lookupTable, const QString& fkCol,
                            const QString& valCol, class SqlQueryModel* srcModel) const;
    // Resolves one FK value's display text: preloads the whole table on first
    // use, then falls back to a single-row query for any value added after
    // that (e.g. a project/person created later in the session) so results
    // stay correct instead of caching a permanent blank.
    QString resolveLookupValue(const QString& lookupTable, const QString& fkCol,
                               const QString& valCol, const QString& fkVal,
                               class SqlQueryModel* srcModel) const;
    // Bulk-loads m_sortColumn's lookup table before sorting. No-op when it isn't
    // a lookup column or the table is already cached.
    void preloadCurrentSortColumn() const;
    QString m_quickSearch;
    // Coalesces bursts of setQuickSearch() calls (one per keystroke) into a
    // single invalidateRowsFilter() so we don't re-scan every row of the
    // source model on every keystroke. Empty text invalidates immediately so
    // clearing the field stays instant.
    QTimer m_quickSearchDebounce;
    // Coalesces source dataChanged bursts into one re-sort per event-loop turn
    // (see onSourceDataChanged).
    QTimer m_resortDebounce;

    int           m_sortColumn      = -1;
    Qt::SortOrder m_sortOrder       = Qt::AscendingOrder;
    int           m_pinnedSourceRow = -1;
    bool          m_pendingSort     = false;
};

#endif // SORTFILTERPROXYMODEL_H
