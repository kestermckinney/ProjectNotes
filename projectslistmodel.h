// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTSLISTMODEL_H
#define PROJECTSLISTMODEL_H

#include "projectsmodel.h"

class ProjectsListModel : public ProjectsModel
{
public:
    ProjectsListModel(DatabaseObjects* dbo);
};

#endif // PROJECTSMODEL_H
