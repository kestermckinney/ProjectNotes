// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTNOTESMODEL_H
#define PROJECTNOTESMODEL_H

#include "sqlquerymodel.h"

class ProjectNotesModel : public SqlQueryModel
{
public:
    ProjectNotesModel(DatabaseObjects* dbo);
    const QModelIndex newRecord(const QVariant* fkValue1 = nullptr, const QVariant* fkValue2 = nullptr) override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    const QModelIndex copyRecord(QModelIndex index) override;
};

#endif // PROJECTNOTESMODEL_H
