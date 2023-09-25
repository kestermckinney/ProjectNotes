// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTSLISTVIEW_H
#define PROJECTSLISTVIEW_H

#include "pncomboboxdelegate.h"
#include "pndateeditdelegate.h"
#include "comboboxdelegate.h"
#include "pntableview.h"

#include <QObject>

class ProjectsListView : public PNTableView
{
public:

    ProjectsListView(QWidget* t_parent = nullptr);
    ~ProjectsListView();

    void setModel(QAbstractItemModel *t_model) override;

private:

    QStringListModel m_project_status;//PNDatabaseObjects::project_status;
    QStringListModel m_status_item_status;//PNDatabaseObjects::status_item_status;
    QStringListModel m_invoicing_period;//PNDatabaseObjects::invoicing_period;
    QStringListModel m_status_report_period; //PNDatabaseObjects::status_report_period;

    // projects list panel delegates
    PNComboBoxDelegate* m_unfiltered_people_delegate = nullptr;
    PNComboBoxDelegate* m_project_clients_delegate = nullptr;
    PNDateEditDelegate* m_project_date_delegate = nullptr;
    ComboBoxDelegate* m_project_invoicing_period_delegate = nullptr;
    ComboBoxDelegate* m_project_status_delegate = nullptr;
    ComboBoxDelegate* m_projects_report_period_delegate = nullptr;

};

#endif // PROJECTSLISTVIEW_H
