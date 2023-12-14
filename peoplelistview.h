// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PEOPLELISTVIEW_H
#define PEOPLELISTVIEW_H

#include "pntableview.h"
#include "pncomboboxdelegate.h"
#include "pnplaintexteditdelegate.h"

class PeopleListView : public PNTableView
{
public:
    PeopleListView(QWidget* t_parent = nullptr);
    ~PeopleListView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    PNComboBoxDelegate* m_unfiltered_clients_delegate =  nullptr;
    PNPlainTextEditDelegate* m_name_delegate = nullptr;
    PNPlainTextEditDelegate* m_role_delegate = nullptr;
};

#endif // PEOPLELISTVIEW_H
