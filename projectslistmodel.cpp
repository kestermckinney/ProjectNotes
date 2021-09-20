#include "projectslistmodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>

ProjectsListModel::ProjectsListModel(QObject* parent) : ProjectsModel(parent)
{  
    setEditable(5, false); // cannot edit the the primary contact when viewing all projects
}

