// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ITEMDETAILTEAMLISTMODEL_H
#define ITEMDETAILTEAMLISTMODEL_H

#include "sqlquerymodel.h"

class ItemDetailTeamListModel : public SqlQueryModel
{
public:
    ItemDetailTeamListModel(DatabaseObjects* dbo);
};

#endif // ITEMDETAILTEAMLISTMODEL_H
