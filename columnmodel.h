// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef COLUMNMODEL_H
#define COLUMNMODEL_H

#include "filtersavestructure.h"

#include "databaseobjects.h"
#include <QObject>

class ColumnModel : public SqlQueryModel
{
public:
    ColumnModel(DatabaseObjects* dbo);
    void setColumnModel(SqlQueryModel* columnmodel);
    void setSavedFilters(QHash<QString, FilterSaveStructure>* filters) { m_savedFilters = filters; };
    void setFilteringModel(SqlQueryModel* model) { m_filteringModel = model; };
    SqlQueryModel* columnmodel() { return m_columnModel; };
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    SqlQueryModel* m_columnModel;
    QHash<QString, FilterSaveStructure>* m_savedFilters;
    SqlQueryModel* m_filteringModel;
};

#endif // COLUMNMODEL_H
