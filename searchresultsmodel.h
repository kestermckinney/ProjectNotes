#ifndef SEARCHRESULTSMODEL_H
#define SEARCHRESULTSMODEL_H

#include "pnsqlquerymodel.h"

class SearchResultsModel : public PNSqlQueryModel
{
public:
    SearchResultsModel(QObject* t_parent);
    bool openRecord(QModelIndex t_index) override;
    void PerformSearch(const QString& t_search_value);
};

#endif // SEARCHRESULTSMODEL_H
