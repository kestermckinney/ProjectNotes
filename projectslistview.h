// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTSLISTVIEW_H
#define PROJECTSLISTVIEW_H

#include "sqlcomboboxdelegate.h"
#include "dateeditdelegate.h"
#include "plaintexteditdelegate.h"
#include "comboboxdelegate.h"
#include "tableview.h"

#include <QObject>

class ProjectsListView : public TableView
{
public:

    ProjectsListView(QWidget* parent = nullptr);
    ~ProjectsListView();

    void setModel(QAbstractItemModel *model) override;

private:

    QStringListModel m_projectStatus;//DatabaseObjects::project_status;
    QStringListModel m_statusItemStatus;//DatabaseObjects::status_item_status;
    QStringListModel m_invoicingPeriod;//DatabaseObjects::invoicing_period;
    QStringListModel m_statusReportPeriod; //DatabaseObjects::status_report_period;

    // projects list panel delegates
    SqlComboBoxDelegate* m_unfilteredPeopleDelegate = nullptr;
    SqlComboBoxDelegate* m_projectClientsDelegate = nullptr;
    DateEditDelegate* m_projectDateDelegate = nullptr;

    ComboBoxDelegate* m_projectInvoicingPeriodDelegate = nullptr;
    ComboBoxDelegate* m_projectStatusDelegate = nullptr;
    ComboBoxDelegate* m_projectsReportPeriodDelegate = nullptr;

    PlainTextEditDelegate* m_projectNameDelegate = nullptr;

};

#endif // PROJECTSLISTVIEW_H
