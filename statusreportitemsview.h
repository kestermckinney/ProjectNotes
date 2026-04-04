// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef STATUSREPORTITEMSVIEW_H
#define STATUSREPORTITEMSVIEW_H

#include "comboboxdelegate.h"
#include "plaintexteditdelegate.h"
#include "tableview.h"

#include <QObject>

class StatusReportItemsView : public TableView
{
public:
    StatusReportItemsView(QWidget* parent = nullptr);
    ~StatusReportItemsView();

    void setModel(QAbstractItemModel *model) override;

private:
    QStringListModel m_statusItemsStatus; // (DatabaseObjects::status_item_status);

    // projects list panel delegates
    ComboBoxDelegate* m_statusItemsStatusDelegate = nullptr;
    PlainTextEditDelegate* m_statusItemDescription = nullptr;

public slots:
    void slotNewRecord() override;
};

#endif // STATUSREPORTITEMSVIEW_H
