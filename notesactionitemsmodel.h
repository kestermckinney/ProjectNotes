// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef NOTESACTIONITEMSMODEL_H
#define NOTESACTIONITEMSMODEL_H

#include "sqlquerymodel.h"

class NotesActionItemsModel : public SqlQueryModel
{
public:
    NotesActionItemsModel(DatabaseObjects* dbo);
    const QModelIndex newRecord(const QVariant* fkValue1 = nullptr, const QVariant* fkValue2 = nullptr) override;
};

#endif // NOTESACTIONITEMSMODEL_H
