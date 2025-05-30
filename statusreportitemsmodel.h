// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef STATUSREPORTITEMSMODEL_H
#define STATUSREPORTITEMSMODEL_H

#include "pnsqlquerymodel.h"

class StatusReportItemsModel : public PNSqlQueryModel
{
public:
    StatusReportItemsModel(PNDatabaseObjects* t_dbo);
    const QModelIndex newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // STATUSREPORTITEMSMODEL_H
