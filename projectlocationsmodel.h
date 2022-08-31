#ifndef PROJECTLOCATIONSMODEL_H
#define PROJECTLOCATIONSMODEL_H

#include "pnsqlquerymodel.h"

class ProjectLocationsModel : public PNSqlQueryModel
{
public:
    ProjectLocationsModel(QObject* t_parent);
    bool newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2);
};

#endif // PROJECTLOCATIONSMODEL_H
