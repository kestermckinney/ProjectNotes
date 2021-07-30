#ifndef PROJECTSMODEL_H
#define PROJECTSMODEL_H

#include "pnsqlquerymodel.h"

class ProjectsModel : public PNSqlQueryModel
{
public:
    ProjectsModel(QObject* parent);
    bool NewRecord();
};

#endif // PROJECTSMODEL_H
