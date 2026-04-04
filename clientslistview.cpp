// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "clientslistview.h"

ClientsListView::ClientsListView(QWidget* parent) : TableView(parent)
{
    setObjectName("tableViewClients");
}

ClientsListView::~ClientsListView()
{
    if (m_clientNameDelegate) delete m_clientNameDelegate;
}

void ClientsListView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        m_clientNameDelegate = new PlainTextEditDelegate(this);

        setColumnHidden(0, true);

        setItemDelegateForColumn(1, m_clientNameDelegate);

    }
    else
    {
        TableView::setModel(model);
    }
}

