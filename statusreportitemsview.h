// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef STATUSREPORTITEMSVIEW_H
#define STATUSREPORTITEMSVIEW_H

#include "comboboxdelegate.h"
#include "pnplaintexteditdelegate.h"
#include "pntableview.h"

#include <QObject>

class StatusReportItemsView : public PNTableView
{
public:
    StatusReportItemsView(QWidget* t_parent = nullptr);
    ~StatusReportItemsView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    QStringListModel m_status_items_status; // (PNDatabaseObjects::status_item_status);

    // projects list panel delegates
    ComboBoxDelegate* m_status_items_status_delegate = nullptr;
    PNPlainTextEditDelegate* m_status_item_description = nullptr;

public slots:
    void slotNewRecord() override;
};

#endif // STATUSREPORTITEMSVIEW_H
