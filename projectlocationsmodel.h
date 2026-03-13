// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTLOCATIONSMODEL_H
#define PROJECTLOCATIONSMODEL_H

#include "sqlquerymodel.h"

class ProjectLocationsModel : public SqlQueryModel
{
public:
    ProjectLocationsModel(DatabaseObjects* dbo);
    const QModelIndex newRecord(const QVariant* fkValue1 = nullptr, const QVariant* fkValue2 = nullptr) override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
};

#endif // PROJECTLOCATIONSMODEL_H
