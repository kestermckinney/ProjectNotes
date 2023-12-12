// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "clientslistview.h"

ClientsListView::ClientsListView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewClients");
}

ClientsListView::~ClientsListView()
{
    if (m_client_name_delegate) delete m_client_name_delegate;
}

void ClientsListView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        m_client_name_delegate = new PNPlainTextEditDelegate(this);

        setColumnHidden(0, true);

        setItemDelegateForColumn(1, m_client_name_delegate);

    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

