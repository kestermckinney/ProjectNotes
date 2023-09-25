// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "clientslistview.h"

ClientsListView::ClientsListView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewClients");
}

void ClientsListView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);

    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

