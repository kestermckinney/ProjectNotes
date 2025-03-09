// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CLIENTSMODEL_H
#define CLIENTSMODEL_H

#include "pnsqlquerymodel.h"

class ClientsModel : public PNSqlQueryModel
{
public:
    ClientsModel(PNDatabaseObjects* t_dbo);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ClientsModel(getDBOs())); }
    const QModelIndex newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // CLIENTSMODEL_H
