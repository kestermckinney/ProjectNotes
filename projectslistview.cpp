// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectslistview.h"
#include "pndatabaseobjects.h"

ProjectsListView::ProjectsListView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewProjects");
    setHasOpen(true);
}

ProjectsListView::~ProjectsListView()
{
    if (m_unfiltered_people_delegate) delete m_unfiltered_people_delegate;
    if (m_project_clients_delegate) delete m_project_clients_delegate;
    if (m_project_date_delegate) delete m_project_date_delegate;
    if (m_projects_report_period_delegate) delete m_projects_report_period_delegate;
    if (m_project_invoicing_period_delegate) delete m_project_invoicing_period_delegate;
    if (m_project_status_delegate) delete m_project_status_delegate;
    if (m_project_name_delegate) delete m_project_name_delegate;
}

void ProjectsListView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);

        // setup model lists
        m_project_status.setStringList(PNDatabaseObjects::project_status);
        m_status_item_status.setStringList(PNDatabaseObjects::status_item_status);
        m_invoicing_period.setStringList(PNDatabaseObjects::invoicing_period);
        m_status_report_period.setStringList(PNDatabaseObjects::status_report_period);

        PNDatabaseObjects* dbo = qobject_cast<PNSqlQueryModel*>(dynamic_cast<PNSortFilterProxyModel*>(t_model)->sourceModel())->getDBOs();

        // projects list panel delagets
        m_unfiltered_people_delegate = new PNComboBoxDelegate(this, dbo->unfilteredpeoplemodel());
        m_project_clients_delegate = new PNComboBoxDelegate(this, dbo->unfilteredclientsmodel());
        m_project_date_delegate = new PNDateEditDelegate(this);
        m_projects_report_period_delegate = new ComboBoxDelegate(this, &m_status_report_period);
        m_project_invoicing_period_delegate = new ComboBoxDelegate(this, &m_invoicing_period);
        m_project_status_delegate = new ComboBoxDelegate(this, &m_project_status);

        m_project_name_delegate = new PNPlainTextEditDelegate(this);

        setItemDelegateForColumn(2, m_project_name_delegate);
        setItemDelegateForColumn(5, m_unfiltered_people_delegate);
        setItemDelegateForColumn(3, m_project_date_delegate);
        setItemDelegateForColumn(4, m_project_date_delegate);
        setItemDelegateForColumn(11, m_project_invoicing_period_delegate);
        setItemDelegateForColumn(12, m_projects_report_period_delegate);
        setItemDelegateForColumn(13, m_project_clients_delegate);
        setItemDelegateForColumn(14, m_project_status_delegate);

    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

