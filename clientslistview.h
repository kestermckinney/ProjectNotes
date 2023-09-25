// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CLIENTSLISTVIEW_H
#define CLIENTSLISTVIEW_H

#include "pntableview.h"

class ClientsListView : public PNTableView
{
public:
    ClientsListView(QWidget* t_parent = nullptr);
    void setModel(QAbstractItemModel *t_model) override;
};

#endif // CLIENTSLISTVIEW_H
