// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "peoplelistview.h"
#include "pndatabaseobjects.h"

PeopleListView::PeopleListView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewPeople");
}

PeopleListView::~PeopleListView()
{
    if (m_unfiltered_clients_delegate) delete m_unfiltered_clients_delegate;
    if (m_name_delegate) delete m_name_delegate;
    if (m_role_delegate) delete m_role_delegate;
}

void PeopleListView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);

        // setup model lists
        m_unfiltered_clients_delegate = new PNComboBoxDelegate(this, global_DBObjects.unfilteredclientsmodel());
        m_name_delegate = new PNPlainTextEditDelegate(this);
        m_role_delegate = new PNPlainTextEditDelegate(this);

        setItemDelegateForColumn(5, m_unfiltered_clients_delegate);
        setItemDelegateForColumn(1, m_name_delegate);
        setItemDelegateForColumn(6, m_role_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}
