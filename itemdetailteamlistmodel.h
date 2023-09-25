// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ITEMDETAILTEAMLISTMODEL_H
#define ITEMDETAILTEAMLISTMODEL_H

#include "pnsqlquerymodel.h"

class ItemDetailTeamListModel : public PNSqlQueryModel
{
public:
    ItemDetailTeamListModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ItemDetailTeamListModel(this)); };
};

#endif // ITEMDETAILTEAMLISTMODEL_H
