#ifndef SEARCHRESULTSMODEL_H
#define SEARCHRESULTSMODEL_H

#include "pnsqlquerymodel.h"

class SearchResultsModel : public PNSqlQueryModel
{
public:
    SearchResultsModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new SearchResultsModel(this)); };
    bool openRecord(QModelIndex t_index) override;
    void PerformSearch(const QString& t_search_value);
    void PerformKeySearch(const QString& t_search_value);
};

#endif // SEARCHRESULTSMODEL_H
