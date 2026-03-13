// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTSMODEL_H
#define PROJECTSMODEL_H

#include "sqlquerymodel.h"

class ProjectsModel : public SqlQueryModel
{
public:
    ProjectsModel(DatabaseObjects* dbo);
    const QModelIndex  newRecord(const QVariant* fkValue1 = nullptr, const QVariant* fkValue2 = nullptr) override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    const QModelIndex copyRecord(QModelIndex index) override;
};

#endif // PROJECTSMODEL_H
