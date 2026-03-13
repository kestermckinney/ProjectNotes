// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef VALUESVIEW_H
#define VALUESVIEW_H

//#include "filterdatadialog.h"
#include "tableview.h"
#include <QObject>

class ValuesView : public TableView
{
public:
    ValuesView(QWidget* parent = nullptr);
    ~ValuesView();
    void setSavedFilters( QHash<QString, FilterSaveStructure>* savedFilters ) {m_savedFilters = savedFilters; };

private:
    QHash<QString, FilterSaveStructure>* m_savedFilters;

public slots:
    void dataRowSelected(const QModelIndex &index) override;
};

#endif // VALUESVIEW_H

