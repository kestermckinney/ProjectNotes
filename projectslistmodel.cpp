// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectslistmodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>
#include <QApplication>

ProjectsListModel::ProjectsListModel(PNDatabaseObjects* t_dbo, bool t_gui) : ProjectsModel(t_dbo, t_gui)
{
    setObjectName("ProjectsListModel");
    setOrderKey(110);
    setTableName("projects", "Projects");

    setEditable(5, DBReadOnly); // cannot edit the the primary contact when viewing all projects
}
