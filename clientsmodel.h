// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CLIENTSMODEL_H
#define CLIENTSMODEL_H

#include "sqlquerymodel.h"

class ClientsModel : public SqlQueryModel
{
public:
    ClientsModel(DatabaseObjects* dbo);
    const QModelIndex newRecord(const QVariant* fkValue1 = nullptr, const QVariant* fkValue2 = nullptr) override;
};

#endif // CLIENTSMODEL_H
