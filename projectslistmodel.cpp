// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectslistmodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>
#include <QApplication>

ProjectsListModel::ProjectsListModel(QObject* t_parent) : ProjectsModel(t_parent)
{
    setObjectName("ProjectsListModel");
    setOrderKey(110);
    setTableName("projects", "Projects");

    setEditable(5, DBReadOnly); // cannot edit the the primary contact when viewing all projects
}

bool ProjectsListModel::openRecord(QModelIndex t_index)
{
    QVariant record_id = data(index(t_index.row(), 0));

    // filter team members by project
    global_DBObjects.projectteammembersmodel()->setFilter(1, record_id.toString());
    global_DBObjects.projectteammembersmodel()->refresh();

    // filter project status items
    global_DBObjects.statusreportitemsmodel()->setFilter(1, record_id.toString());
    global_DBObjects.statusreportitemsmodel()->refresh();

    // filter team members by project for members list
    global_DBObjects.teamsmodel()->setFilter(2, record_id.toString());
    global_DBObjects.teamsmodel()->refresh();

    // filter tracker items by project
    global_DBObjects.trackeritemsmodel()->setFilter(14, record_id.toString());
    global_DBObjects.trackeritemsmodel()->refresh();

    // filter tracker items by project
    global_DBObjects.trackeritemsmodel()->setFilter(14, record_id.toString());
    global_DBObjects.trackeritemsmodel()->refresh();

    global_DBObjects.trackeritemsmeetingsmodel()->setFilter(1, record_id.toString());
    global_DBObjects.trackeritemsmeetingsmodel()->refresh();

    global_DBObjects.projectlocationsmodel()->setFilter(1, record_id.toString());
    global_DBObjects.projectlocationsmodel()->refresh();

    global_DBObjects.projectnotesmodel()->setFilter(1, record_id.toString());
    global_DBObjects.projectnotesmodel()->refresh();

    // only select the records another event will be fired to open the window to show them
    // order is important this needs to be last
    global_DBObjects.projectinformationmodel()->setFilter(0, record_id.toString());
    global_DBObjects.projectinformationmodel()->refresh();

    return true;
}
