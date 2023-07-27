#ifndef PROJECTNOTESMODEL_H
#define PROJECTNOTESMODEL_H

#include "pnsqlquerymodel.h"

class ProjectNotesModel : public PNSqlQueryModel
{
public:
    ProjectNotesModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ProjectNotesModel(this)); };
    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
    bool setData(const QModelIndex &t_index, const QVariant &t_value, int t_role) override;
    bool copyRecord(QModelIndex t_index) override;

    bool openRecord(QModelIndex t_index) override;
};

#endif // PROJECTNOTESMODEL_H
