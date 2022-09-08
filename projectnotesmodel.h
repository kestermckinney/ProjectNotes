#ifndef PROJECTNOTESMODEL_H
#define PROJECTNOTESMODEL_H

#include "pnsqlquerymodel.h"

class ProjectNotesModel : public PNSqlQueryModel
{
public:
    ProjectNotesModel(QObject* t_parent);

    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;

    bool openRecord(QModelIndex t_index) override;
};

#endif // PROJECTNOTESMODEL_H
