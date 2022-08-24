#ifndef STATUSREPORTITEMSMODEL_H
#define STATUSREPORTITEMSMODEL_H

#include "pnsqlquerymodel.h"

class StatusReportItemsModel : public PNSqlQueryModel
{
public:
    StatusReportItemsModel(QObject* t_parent);
    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // STATUSREPORTITEMSMODEL_H
