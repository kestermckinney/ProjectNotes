// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ACTIONITEMSDETAILSMEETINGSMODEL_H
#define ACTIONITEMSDETAILSMEETINGSMODEL_H

#include "pnsqlquerymodel.h"

class ActionItemsDetailsMeetingsModel : public PNSqlQueryModel
{
public:
    ActionItemsDetailsMeetingsModel(PNDatabaseObjects* t_dbo, bool t_gui = true);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ActionItemsDetailsMeetingsModel(getDBOs(), false)); }
};

#endif // ACTIONITEMSDETAILSMEETINGSMODEL_H
