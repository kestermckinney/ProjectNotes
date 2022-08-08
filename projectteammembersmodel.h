#ifndef PROJECTTEAMMEMBERSMODEL_H
#define PROJECTTEAMMEMBERSMODEL_H

#include "pnsqlquerymodel.h"

class ProjectTeamMembersModel : public PNSqlQueryModel
{
public:
    ProjectTeamMembersModel(QObject* t_parent);

    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;
};

#endif // PROJECTTEAMMEMBERSMODEL_H
