// Copyright (C) 2022, 2023 Paul McKinney
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
};

#endif // SEARCHRESULTSMODEL_H
