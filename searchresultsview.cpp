// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "searchresultsview.h"
#include "databaseobjects.h"

SearchResultsView::SearchResultsView(QWidget* parent) : TableView(parent)
{
    setObjectName("tableViewSearchResults");
    setHasOpen(true);
}

SearchResultsView::~SearchResultsView()
{
    if (m_textEditDelegate) delete m_textEditDelegate;
}

void SearchResultsView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);
        setColumnHidden(4, true);
        setColumnHidden(5, true);
        setColumnHidden(6, true);
        setColumnHidden(13, true);
        setColumnHidden(14, true);

        // search view delagets
        m_textEditDelegate = new TextEditDelegate(this);

        setItemDelegateForColumn(3, m_textEditDelegate);
    }
    else
    {
        TableView::setModel(model);
    }
}
