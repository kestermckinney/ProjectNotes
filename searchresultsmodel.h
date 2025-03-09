// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SEARCHRESULTSMODEL_H
#define SEARCHRESULTSMODEL_H

#include "pnsqlquerymodel.h"

class SearchResultsModel : public PNSqlQueryModel
{
public:
    SearchResultsModel(PNDatabaseObjects* t_dbo);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new SearchResultsModel(getDBOs())); }
    void PerformSearch(const QString& t_search_value);
    void PerformKeySearch(const QStringList& t_search_fields, const QStringList& t_search_values);
};

#endif // SEARCHRESULTSMODEL_H
