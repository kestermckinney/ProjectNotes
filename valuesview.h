// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef VALUESVIEW_H
#define VALUESVIEW_H

//#include "filterdatadialog.h"
#include "pntableview.h"
#include <QObject>

class ValuesView : public PNTableView
{
public:
    ValuesView(QWidget* t_parent = nullptr);
    ~ValuesView();
    void setSavedFilters( QHash<QString, FilterSaveStructure>* t_saved_filters ) {m_saved_filters = t_saved_filters; };

private:
    QHash<QString, FilterSaveStructure>* m_saved_filters;

public slots:
    void dataRowSelected(const QModelIndex &t_index) override;
};

#endif // VALUESVIEW_H

