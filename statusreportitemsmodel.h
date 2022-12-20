#ifndef STATUSREPORTITEMSMODEL_H
#define STATUSREPORTITEMSMODEL_H

#include "pnsqlquerymodel.h"

class StatusReportItemsModel : public PNSqlQueryModel
{
public:
    StatusReportItemsModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new StatusReportItemsModel(this)); };
    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // STATUSREPORTITEMSMODEL_H
