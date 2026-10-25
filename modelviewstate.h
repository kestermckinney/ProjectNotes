// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QAbstractItemModel>
#include <QVariant>
class SqlQueryModel;

// Shared desktop/mobile list filtering, sorting, and synced UI state.
namespace PN::ModelViewState {
SqlQueryModel* sourceModelOf(QAbstractItemModel* model);
void restoreSort(QAbstractItemModel* model);
void restoreColumnFilters(SqlQueryModel* src);
QVariantList filterColumns(QAbstractItemModel* model);
QVariantList columnDistinctValues(QAbstractItemModel* model, const QString& field);
void applyColumnFilters(QAbstractItemModel* model, const QVariantList& specs);
// Sets src's user filters from specs without activating, refreshing or saving them.
void applyFilterSpecsToModel(SqlQueryModel* src, const QVariantList& specs);
void clearColumnFilters(QAbstractItemModel* model);
QVariantList activeColumnFilters(QAbstractItemModel* model);
void applyQuickFilter(QAbstractItemModel* model, const QString& field, const QVariantList& values);
bool hasActiveColumnFilters(QAbstractItemModel* model);
QVariantList sortColumns(QAbstractItemModel* model);
void applySort(QAbstractItemModel* model, const QString& field, bool descending);
void clearSort(QAbstractItemModel* model);
QVariantMap activeSort(QAbstractItemModel* model);
}
