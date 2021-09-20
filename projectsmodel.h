#ifndef PROJECTSMODEL_H
#define PROJECTSMODEL_H

#include "pnsqlquerymodel.h"

class ProjectsModel : public PNSqlQueryModel
{
public:
    ProjectsModel(QObject* parent);
    bool NewRecord();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

#endif // PROJECTSMODEL_H
