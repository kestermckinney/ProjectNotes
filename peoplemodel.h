// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PEOPLEMODEL_H
#define PEOPLEMODEL_H

#include "sqlquerymodel.h"

class PeopleModel : public SqlQueryModel
{
public:
    PeopleModel(DatabaseObjects* dbo);
};

#endif // PEOPLEMODEL_H
