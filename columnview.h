// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef COLUMNVIEW_H
#define COLUMNVIEW_H

#include "filterdatadialog.h"
#include "pntableview.h"
#include "valueselectmodel.h"
#include <QObject>

class ColumnView : public PNTableView
{
public:
    ColumnView(QWidget* t_parent = nullptr);
    void setColumnValuesModel( ValueSelectModel* t_model ) { m_values_model = t_model; };
    void setFilteredModel( PNSqlQueryModel* t_model ) { m_filtered_model = t_model; };
    void setValuesView( PNTableView* t_view, QHash<QString, FilterSaveStructure>* t_savedfilters ) { m_values_view = t_view; m_saved_filters = t_savedfilters; };
    void setSourceView( PNTableView* t_view ) { m_source_view = t_view; };
    void setUI( FilterDataDialog* t_parent_ui ) { m_parent_ui = t_parent_ui; };

private:
    ValueSelectModel* m_values_model;
    PNSqlQueryModel* m_filtered_model;
    FilterDataDialog *m_parent_ui;

    PNTableView* m_values_view;
    PNTableView* m_source_view;
    QHash<QString, FilterSaveStructure>* m_saved_filters;

public slots:
    void dataRowSelected(const QModelIndex &t_index) override;
};

#endif // COLUMNVIEW_H
