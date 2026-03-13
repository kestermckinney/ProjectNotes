// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CLIENTSLISTVIEW_H
#define CLIENTSLISTVIEW_H

#include "tableview.h"
#include "plaintexteditdelegate.h"

class ClientsListView : public TableView
{
public:
    ClientsListView(QWidget* parent = nullptr);
    ~ClientsListView();
    void setModel(QAbstractItemModel *model) override;

private:
    PlainTextEditDelegate* m_clientNameDelegate;
};

#endif // CLIENTSLISTVIEW_H
