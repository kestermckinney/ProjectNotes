// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TEAMSMODEL_H
#define TEAMSMODEL_H


#include "sqlquerymodel.h"

class TeamsModel : public SqlQueryModel
{
public:
    TeamsModel(DatabaseObjects* dbo);
};

#endif // TEAMSMODEL_H
