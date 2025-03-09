// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ACTIONITEMSDETAILSMEETINGSMODEL_H
#define ACTIONITEMSDETAILSMEETINGSMODEL_H

#include "pnsqlquerymodel.h"

class ActionItemsDetailsMeetingsModel : public PNSqlQueryModel
{
public:
    ActionItemsDetailsMeetingsModel(PNDatabaseObjects* t_dbo);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ActionItemsDetailsMeetingsModel(getDBOs())); }
};

#endif // ACTIONITEMSDETAILSMEETINGSMODEL_H
