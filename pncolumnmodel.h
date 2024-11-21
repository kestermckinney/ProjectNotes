// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNCOLUMNMODEL_H
#define PNCOLUMNMODEL_H

#include "FilterSaveStructure.h"

#include "pndatabaseobjects.h"
#include <QObject>

class PNColumnModel : public PNSqlQueryModel
{
public:
    PNColumnModel(PNDatabaseObjects* t_dbo, bool t_gui = true);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new PNColumnModel(getDBOs(), false)); };
    void setColumnModel(PNSqlQueryModel* t_columnmodel);
    void setSavedFilters(QHash<QString, FilterSaveStructure>* t_filters) { m_saved_filters = t_filters; };
    void setFilteringModel(PNSqlQueryModel* t_model) { m_filtering_model = t_model; };
    PNSqlQueryModel* columnmodel() { return m_column_model; };
    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;

private:
    PNSqlQueryModel* m_column_model;
    QHash<QString, FilterSaveStructure>* m_saved_filters;
    PNSqlQueryModel* m_filtering_model;
};

#endif // PNCOLUMNMODEL_H
