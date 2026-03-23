// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectslistmodel.h"
#include "databaseobjects.h"

#include <QRegularExpression>
#include <QApplication>

ProjectsListModel::ProjectsListModel(DatabaseObjects* dbo) : ProjectsModel(dbo)
{
    setObjectName("ProjectsListModel");
    setTableName("projects", "Projects");

    setEditable(5, DBReadOnly); // cannot edit the the primary contact when viewing all projects
}
