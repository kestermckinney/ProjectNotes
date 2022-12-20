#ifndef ACTIONITEMSDETAILSMEETINGSMODEL_H
#define ACTIONITEMSDETAILSMEETINGSMODEL_H

#include "pnsqlquerymodel.h"

class ActionItemsDetailsMeetingsModel : public PNSqlQueryModel
{
public:
    ActionItemsDetailsMeetingsModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ActionItemsDetailsMeetingsModel(this)); };
};

#endif // ACTIONITEMSDETAILSMEETINGSMODEL_H
