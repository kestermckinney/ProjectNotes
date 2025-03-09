// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TEAMSMODEL_H
#define TEAMSMODEL_H


#include "pnsqlquerymodel.h"

class TeamsModel : public PNSqlQueryModel
{
public:
    TeamsModel(PNDatabaseObjects* t_dbo);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new TeamsModel(getDBOs())); };
};

#endif // TEAMSMODEL_H
