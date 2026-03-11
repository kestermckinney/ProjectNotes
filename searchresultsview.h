// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SEARCHRESULTSVIEW_H
#define SEARCHRESULTSVIEW_H

#include "tableview.h"
#include "texteditdelegate.h"

#include <QObject>

class SearchResultsView : public TableView
{
public:

    SearchResultsView(QWidget* parent = nullptr);
    ~SearchResultsView();
    void setModel(QAbstractItemModel *model) override;

private:
    // search view delegates
    TextEditDelegate* m_textEditDelegate = nullptr;
};

#endif // SEARCHRESULTSVIEW_H
