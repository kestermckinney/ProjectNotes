#include "projectslistmodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>
#include <QApplication>
#include "mainwindow.h"

ProjectsListModel::ProjectsListModel(QObject* t_parent) : ProjectsModel(t_parent)
{
    setObjectName("ProjectsListModel");

    setEditable(5, false); // cannot edit the the primary contact when viewing all projects
}

bool ProjectsListModel::openRecord(QModelIndex t_index)
{
    QVariant record_id = data(index(t_index.row(), 0));

    // only select the records another event will be fired to open the window to show them
    global_DBObjects.projectinformationmodel()->setFilter(0, record_id.toString());
    global_DBObjects.projectinformationmodel()->refresh();

    // filter team members by project
    global_DBObjects.projectteammembersmodel()->setFilter(1, record_id.toString());
    global_DBObjects.projectteammembersmodel()->refresh();

    return true;
}
