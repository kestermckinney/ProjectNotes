#ifndef SEARCHRESULTSMODEL_H
#define SEARCHRESULTSMODEL_H

#include "pnsqlquerymodel.h"

class SearchResultsModel : public PNSqlQueryModel
{
public:
    SearchResultsModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new SearchResultsModel(this)); };
};

#endif // SEARCHRESULTSMODEL_H
