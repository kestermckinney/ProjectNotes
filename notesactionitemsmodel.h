// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef NOTESACTIONITEMSMODEL_H
#define NOTESACTIONITEMSMODEL_H

#include "pnsqlquerymodel.h"

class NotesActionItemsModel : public PNSqlQueryModel
{
public:
    NotesActionItemsModel(PNDatabaseObjects* t_dbo, bool t_gui = true);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new NotesActionItemsModel(getDBOs(), false)); }
    const QModelIndex newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // NOTESACTIONITEMSMODEL_H
