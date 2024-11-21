// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TRACKERITEMSMODEL_H
#define TRACKERITEMSMODEL_H

#include "pnsqlquerymodel.h"

class TrackerItemsModel : public PNSqlQueryModel
{
public:
    TrackerItemsModel(PNDatabaseObjects* t_dbo, bool t_gui = true);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new TrackerItemsModel(getDBOs(), false)); }
    const QModelIndex newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
    bool setData(const QModelIndex &t_index, const QVariant &t_value, int t_role) override;
    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;

    //bool openRecord(QModelIndex t_index) override;
    const QModelIndex copyRecord(QModelIndex t_index) override;

    QVariant getNextItemNumber(const QVariant& t_project_id);
};

#endif // TRACKERITEMSMODEL_H
