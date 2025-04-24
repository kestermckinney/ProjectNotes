// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ACTIONITEMPROJECTNOTESMODEL_H
#define ACTIONITEMPROJECTNOTESMODEL_H

#include "pnsqlquerymodel.h"

class ActionItemProjectNotesModel : public PNSqlQueryModel
{
public:
    ActionItemProjectNotesModel(PNDatabaseObjects* t_dbo);
};

#endif // ACTIONITEMPROJECTNOTESMODEL_H
