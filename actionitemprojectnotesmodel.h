// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ACTIONITEMPROJECTNOTESMODEL_H
#define ACTIONITEMPROJECTNOTESMODEL_H

#include "sqlquerymodel.h"

class ActionItemProjectNotesModel : public SqlQueryModel
{
public:
    ActionItemProjectNotesModel(DatabaseObjects* dbo);
};

#endif // ACTIONITEMPROJECTNOTESMODEL_H
