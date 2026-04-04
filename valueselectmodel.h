// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef VALUESELECTMODEL_H
#define VALUESELECTMODEL_H

#include "databaseobjects.h"
#include <QObject>

class ValueSelectModel : public SqlQueryModel
{
public:
    ValueSelectModel(DatabaseObjects* dbo);
    void setValuesColumn(QString column);

    void setFilteringModel(SqlQueryModel* model) { m_filteringModel = model; };

private:
    SqlQueryModel* m_filteringModel;
};

#endif // VALUESELECTMODEL_H
