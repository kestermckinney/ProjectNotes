// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "searchresultsview.h"
#include "pndatabaseobjects.h"

SearchResultsView::SearchResultsView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewSearchResults");
    setHasOpen(true);
}

SearchResultsView::~SearchResultsView()
{
    if (m_text_edit_delegate) delete m_text_edit_delegate;
}

void SearchResultsView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(4, true);
        setColumnHidden(5, true);
        setColumnHidden(6, true);
        setColumnHidden(13, true);
        setColumnHidden(14, true);

        // search view delagets
        m_text_edit_delegate = new PNTextEditDelegate(this);

        setItemDelegateForColumn(3, m_text_edit_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}
