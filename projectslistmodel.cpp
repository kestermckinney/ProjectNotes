#include "projectslistmodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>

ProjectsListModel::ProjectsListModel(QObject* t_parent) : ProjectsModel(t_parent)
{
    setObjectName("ProjectsListModel");

    setEdit_table(5, false); // cannot edit the the primary contact when viewing all projects
}

