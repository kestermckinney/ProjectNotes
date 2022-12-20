#ifndef PROJECTLOCATIONSMODEL_H
#define PROJECTLOCATIONSMODEL_H

#include "pnsqlquerymodel.h"

class ProjectLocationsModel : public PNSqlQueryModel
{
public:
    ProjectLocationsModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ProjectLocationsModel(this)); };
    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
    bool setData(const QModelIndex &t_index, const QVariant &t_value, int t_role) override;

    bool openRecord(QModelIndex t_index) override;
};

#endif // PROJECTLOCATIONSMODEL_H
