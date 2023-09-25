// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SEARCHRESULTSVIEW_H
#define SEARCHRESULTSVIEW_H

#include "pntableview.h"
#include "pntexteditdelegate.h"

#include <QObject>

class SearchResultsView : public PNTableView
{
public:

    SearchResultsView(QWidget* t_parent = nullptr);
    ~SearchResultsView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    // search view delegates
    PNTextEditDelegate* m_text_edit_delegate = nullptr;
};

#endif // SEARCHRESULTSVIEW_H
