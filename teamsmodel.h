#ifndef TEAMSMODEL_H
#define TEAMSMODEL_H

#include "pnsqlquerymodel.h"

class TeamsModel : public PNSqlQueryModel
{
public:
    TeamsModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new TeamsModel(this)); };
};

#endif // TEAMSMODEL_H
