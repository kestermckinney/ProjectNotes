// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTNOTESMODEL_H
#define PROJECTNOTESMODEL_H

#include "pnsqlquerymodel.h"

class ProjectNotesModel : public PNSqlQueryModel
{
public:
    ProjectNotesModel(PNDatabaseObjects* t_dbo);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ProjectNotesModel(getDBOs())); }
    const QModelIndex newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
    bool setData(const QModelIndex &t_index, const QVariant &t_value, int t_role) override;
    const QModelIndex copyRecord(QModelIndex t_index) override;
};

#endif // PROJECTNOTESMODEL_H
