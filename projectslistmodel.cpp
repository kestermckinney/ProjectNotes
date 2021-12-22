#include "projectslistmodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>

ProjectsListModel::ProjectsListModel(QObject* parent) : ProjectsModel(parent)
{
    setObjectName("ProjectsListModel");

    setEditable(5, false); // cannot edit the the primary contact when viewing all projects
}

