// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTSMODEL_H
#define PROJECTSMODEL_H

#include "pnsqlquerymodel.h"

class ProjectsModel : public PNSqlQueryModel
{
public:
    ProjectsModel(PNDatabaseObjects* t_dbo);
    const QModelIndex  newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;

    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &t_index, const QVariant &t_value, int t_role) override;
    const QModelIndex copyRecord(QModelIndex t_index) override;
};

#endif // PROJECTSMODEL_H
