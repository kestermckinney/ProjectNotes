#ifndef PEOPLEMODEL_H
#define PEOPLEMODEL_H

#include "pnsqlquerymodel.h"

class PeopleModel : public PNSqlQueryModel
{
public:
    PeopleModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new PeopleModel(this)); };
};

#endif // PEOPLEMODEL_H
