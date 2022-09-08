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
