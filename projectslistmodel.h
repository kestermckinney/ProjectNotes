// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTSLISTMODEL_H
#define PROJECTSLISTMODEL_H

//#include "pnsqlquerymodel.h"
#include "projectsmodel.h"

class ProjectsListModel : public ProjectsModel
{
public:
    ProjectsListModel(QObject* t_parent);

    bool openRecord(QModelIndex t_index) override;
};

#endif // PROJECTSMODEL_H
