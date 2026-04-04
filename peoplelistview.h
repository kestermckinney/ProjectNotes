// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PEOPLELISTVIEW_H
#define PEOPLELISTVIEW_H

#include "tableview.h"
#include "sqlcomboboxdelegate.h"
#include "plaintexteditdelegate.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

class PeopleListView : public TableView
{
public:
    PeopleListView(QWidget* parent = nullptr);
    ~PeopleListView();

    void setModel(QAbstractItemModel *model) override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    SqlComboBoxDelegate* m_unfilteredClientsDelegate =  nullptr;
    PlainTextEditDelegate* m_nameDelegate = nullptr;
    PlainTextEditDelegate* m_roleDelegate = nullptr;
};

#endif // PEOPLELISTVIEW_H
