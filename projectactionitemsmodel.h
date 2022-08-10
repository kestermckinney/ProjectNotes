#ifndef PROJECTACTIONITEMSMODEL_H
#define PROJECTACTIONITEMSMODEL_H

#include "pnsqlquerymodel.h"

class ProjectActionItemsModel : public PNSqlQueryModel
{
public:
    ProjectActionItemsModel(QObject* t_parent);

    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // PROJECTACTIONITEMSMODEL_H
