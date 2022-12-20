#ifndef ACTIONITEMPROJECTNOTESMODEL_H
#define ACTIONITEMPROJECTNOTESMODEL_H

#include "pnsqlquerymodel.h"

class ActionItemProjectNotesModel : public PNSqlQueryModel
{
public:
    ActionItemProjectNotesModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ActionItemProjectNotesModel(this)); };
};

#endif // ACTIONITEMPROJECTNOTESMODEL_H
