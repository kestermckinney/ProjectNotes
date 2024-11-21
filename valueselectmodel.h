// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef VALUESELECTMODEL_H
#define VALUESELECTMODEL_H

#include "pndatabaseobjects.h"
#include <QObject>

class ValueSelectModel : public PNSqlQueryModel
{
public:
    ValueSelectModel(PNDatabaseObjects* t_dbo, bool t_gui = true);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ValueSelectModel(getDBOs(), false)); };
    void setValuesColumn(QString t_column);

    void setFilteringModel(PNSqlQueryModel* t_model) { m_filtering_model = t_model; };

private:
    PNSqlQueryModel* m_filtering_model;
};

#endif // VALUESELECTMODEL_H
