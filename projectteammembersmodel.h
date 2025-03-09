// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTTEAMMEMBERSMODEL_H
#define PROJECTTEAMMEMBERSMODEL_H

#include "pnsqlquerymodel.h"

class ProjectTeamMembersModel : public PNSqlQueryModel
{
public:
    ProjectTeamMembersModel(PNDatabaseObjects* t_dbo);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ProjectTeamMembersModel(getDBOs())); }
    const QModelIndex newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;

    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &t_index, const QVariant &t_value, int t_role) override;
};

#endif // PROJECTTEAMMEMBERSMODEL_H
