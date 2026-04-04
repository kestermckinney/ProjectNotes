// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ACTIONITEMSDETAILSMEETINGSMODEL_H
#define ACTIONITEMSDETAILSMEETINGSMODEL_H

#include "sqlquerymodel.h"

class ActionItemsDetailsMeetingsModel : public SqlQueryModel
{
public:
    ActionItemsDetailsMeetingsModel(DatabaseObjects* dbo);
};

#endif // ACTIONITEMSDETAILSMEETINGSMODEL_H
