// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTSLISTMODEL_H
#define PROJECTSLISTMODEL_H

#include "projectsmodel.h"

class ProjectsListModel : public ProjectsModel
{
public:
    ProjectsListModel(PNDatabaseObjects* t_dbo, bool t_gui = true);
};

#endif // PROJECTSMODEL_H
