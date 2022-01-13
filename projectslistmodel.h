#ifndef PROJECTSLISTMODEL_H
#define PROJECTSLISTMODEL_H

#include "pnsqlquerymodel.h"
#include "projectsmodel.h"

class ProjectsListModel : public ProjectsModel
{
public:
    ProjectsListModel(QObject* t_parent);
};

#endif // PROJECTSMODEL_H
