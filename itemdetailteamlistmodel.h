// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ITEMDETAILTEAMLISTMODEL_H
#define ITEMDETAILTEAMLISTMODEL_H

#include "pnsqlquerymodel.h"

class ItemDetailTeamListModel : public PNSqlQueryModel
{
public:
    ItemDetailTeamListModel(PNDatabaseObjects* t_dbo);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ItemDetailTeamListModel(getDBOs())); };
};

#endif // ITEMDETAILTEAMLISTMODEL_H
