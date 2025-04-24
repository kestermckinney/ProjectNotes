// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PEOPLEMODEL_H
#define PEOPLEMODEL_H

#include "pnsqlquerymodel.h"

class PeopleModel : public PNSqlQueryModel
{
public:
    PeopleModel(PNDatabaseObjects* t_dbo);
};

#endif // PEOPLEMODEL_H
