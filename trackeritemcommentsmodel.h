// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TRACKERITEMCOMMENTSMODEL_H
#define TRACKERITEMCOMMENTSMODEL_H

#include "pnsqlquerymodel.h"

class TrackerItemCommentsModel : public PNSqlQueryModel
{
public:
    TrackerItemCommentsModel(PNDatabaseObjects* t_dbo);
    const QModelIndex newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
    bool setData(const QModelIndex &t_index, const QVariant &t_value, int t_role) override;
};

#endif // TRACKERITEMCOMMENTSMODEL_H
