// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PEOPLELISTVIEW_H
#define PEOPLELISTVIEW_H

#include "tableview.h"
#include "sqlcomboboxdelegate.h"
#include "plaintexteditdelegate.h"

class PeopleListView : public TableView
{
public:
    PeopleListView(QWidget* parent = nullptr);
    ~PeopleListView();

    void setModel(QAbstractItemModel *model) override;

private:
    SqlComboBoxDelegate* m_unfilteredClientsDelegate =  nullptr;
    PlainTextEditDelegate* m_nameDelegate = nullptr;
    PlainTextEditDelegate* m_roleDelegate = nullptr;
};

#endif // PEOPLELISTVIEW_H
