// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TRACKERITEMSMODEL_H
#define TRACKERITEMSMODEL_H

#include "sqlquerymodel.h"

class TrackerItemsModel : public SqlQueryModel
{
public:
    TrackerItemsModel(DatabaseObjects* dbo);
    const QModelIndex newRecord(const QVariant* fkValue1 = nullptr, const QVariant* fkValue2 = nullptr) override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    const QModelIndex copyRecord(QModelIndex index) override;
    void prepareCopiedRecord(QVector<QVariant>& newrecord, const QModelIndex& sourceIndex) override;

    QVariant getNextItemNumber(const QVariant& projectId);
};

#endif // TRACKERITEMSMODEL_H
