#ifndef TRACKERITEMSMODEL_H
#define TRACKERITEMSMODEL_H

#include "pnsqlquerymodel.h"

class TrackerItemsModel : public PNSqlQueryModel
{
public:
    TrackerItemsModel(QObject* t_parent);

    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
    bool setData(const QModelIndex &t_index, const QVariant &t_value, int t_role) override;
    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;

    bool openRecord(QModelIndex t_index) override;
};

#endif // TRACKERITEMSMODEL_H
