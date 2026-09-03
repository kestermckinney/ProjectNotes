// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SEARCHRESULTSMODEL_H
#define SEARCHRESULTSMODEL_H

#include "sqlquerymodel.h"

class SearchResultsModel : public SqlQueryModel
{
public:
    SearchResultsModel(DatabaseObjects* dbo);
    void PerformSearch(const QString& searchValue);
    void PerformKeySearch(const QStringList& searchFields, const QStringList& searchValues);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    // The most recent free-text search string, used to clip the Content column
    // to a window around the match instead of showing the whole note.
    QString m_lastSearchValue;
};

#endif // SEARCHRESULTSMODEL_H
