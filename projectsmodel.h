#ifndef PROJECTSMODEL_H
#define PROJECTSMODEL_H

#include "pnsqlquerymodel.h"

class ProjectsModel : public PNSqlQueryModel
{
public:
    ProjectsModel(QObject* t_parent);
    bool newRecord() override;

    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;
};

#endif // PROJECTSMODEL_H
