#ifndef TRACKERITEMCOMMENTSMODEL_H
#define TRACKERITEMCOMMENTSMODEL_H

#include "pnsqlquerymodel.h"

class TrackerItemCommentsModel : public PNSqlQueryModel
{
public:
    TrackerItemCommentsModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new TrackerItemCommentsModel(this)); };
    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // TRACKERITEMCOMMENTSMODEL_H
