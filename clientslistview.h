// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CLIENTSLISTVIEW_H
#define CLIENTSLISTVIEW_H

#include "pntableview.h"
#include "pnplaintexteditdelegate.h"

class ClientsListView : public PNTableView
{
public:
    ClientsListView(QWidget* t_parent = nullptr);
    ~ClientsListView();
    void setModel(QAbstractItemModel *t_model) override;

private:
    PNPlainTextEditDelegate* m_client_name_delegate;
};

#endif // CLIENTSLISTVIEW_H
