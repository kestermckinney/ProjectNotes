#ifndef PROJECTTEAMMEMBERSMODEL_H
#define PROJECTTEAMMEMBERSMODEL_H

#include "pnsqlquerymodel.h"

class ProjectTeamMembersModel : public PNSqlQueryModel
{
public:
    ProjectTeamMembersModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ProjectTeamMembersModel(this)); };
    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;

    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;
};

#endif // PROJECTTEAMMEMBERSMODEL_H
