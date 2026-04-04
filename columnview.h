// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef COLUMNVIEW_H
#define COLUMNVIEW_H

#include "filterdatadialog.h"
#include "tableview.h"
#include "valueselectmodel.h"
#include <QObject>

class ColumnView : public TableView
{
public:
    ColumnView(QWidget* parent = nullptr);
    void setColumnValuesModel( ValueSelectModel* model ) { m_valuesModel = model; };
    void setFilteredModel( SqlQueryModel* model ) { m_filteredModel = model; };
    void setValuesView( TableView* view, QHash<QString, FilterSaveStructure>* savedfilters ) { m_valuesView = view; m_savedFilters = savedfilters; };
    void setSourceView( TableView* view ) { m_sourceView = view; };
    void setUI( FilterDataDialog* parentUi ) { m_parentUi = parentUi; };

private:
    ValueSelectModel* m_valuesModel;
    SqlQueryModel* m_filteredModel;
    FilterDataDialog *m_parentUi;

    TableView* m_valuesView;
    TableView* m_sourceView;
    QHash<QString, FilterSaveStructure>* m_savedFilters;

public slots:
    void dataRowSelected(const QModelIndex &index) override;
};

#endif // COLUMNVIEW_H
