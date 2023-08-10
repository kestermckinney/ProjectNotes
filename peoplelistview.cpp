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
}

void PeopleListView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);


        // setup model lists
        m_unfiltered_clients_delegate = new PNComboBoxDelegate(this, global_DBObjects.unfilteredclientsmodel());;

        setItemDelegateForColumn(5, m_unfiltered_clients_delegate);

    }
    else
    {
        PNTableView::setModel(t_model);
    }
}
