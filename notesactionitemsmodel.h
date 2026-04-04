// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef NOTESACTIONITEMSMODEL_H
#define NOTESACTIONITEMSMODEL_H

#include "sqlquerymodel.h"

class NotesActionItemsModel : public SqlQueryModel
{
public:
    NotesActionItemsModel(DatabaseObjects* dbo);
    const QModelIndex newRecord(const QVariant* fkValue1 = nullptr, const QVariant* fkValue2 = nullptr) override;
    void prepareCopiedRecord(QVector<QVariant>& newrecord, const QModelIndex& sourceIndex) override;
    QVariant getNextItemNumber(const QVariant& projectId);
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
};

#endif // NOTESACTIONITEMSMODEL_H
