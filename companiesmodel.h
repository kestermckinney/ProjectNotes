// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef COMPANIESMODEL_H
#define COMPANIESMODEL_H

#include "sqlquerymodel.h"

class CompaniesModel : public SqlQueryModel
{
public:
    CompaniesModel(DatabaseObjects* dbo);
    const QModelIndex newRecord(const QVariant* fkValue1 = nullptr, const QVariant* fkValue2 = nullptr) override;
};

#endif // COMPANIESMODEL_H
