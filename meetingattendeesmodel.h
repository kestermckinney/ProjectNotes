// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MEETINGATTENDEESMODEL_H
#define MEETINGATTENDEESMODEL_H

#include "pnsqlquerymodel.h"

class MeetingAttendeesModel : public PNSqlQueryModel
{
public:
    MeetingAttendeesModel(QObject* m_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new MeetingAttendeesModel(this)); };
    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // MEETINGATTENDEESMODEL_H
