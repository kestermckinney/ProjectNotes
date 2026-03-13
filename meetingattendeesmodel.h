// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MEETINGATTENDEESMODEL_H
#define MEETINGATTENDEESMODEL_H

#include "sqlquerymodel.h"

class MeetingAttendeesModel : public SqlQueryModel
{
public:
    MeetingAttendeesModel(DatabaseObjects* dbo);
    const QModelIndex newRecord(const QVariant* fkValue1 = nullptr, const QVariant* fkValue2 = nullptr) override;
};

#endif // MEETINGATTENDEESMODEL_H
