// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectslistview.h"
#include "databaseobjects.h"

ProjectsListView::ProjectsListView(QWidget* parent) : TableView(parent)
{
    setObjectName("tableViewProjects");
    setHasOpen(true);
}

ProjectsListView::~ProjectsListView()
{
    if (m_unfilteredPeopleDelegate) delete m_unfilteredPeopleDelegate;
    if (m_projectClientsDelegate) delete m_projectClientsDelegate;
    if (m_projectDateDelegate) delete m_projectDateDelegate;
    if (m_projectsReportPeriodDelegate) delete m_projectsReportPeriodDelegate;
    if (m_projectInvoicingPeriodDelegate) delete m_projectInvoicingPeriodDelegate;
    if (m_projectStatusDelegate) delete m_projectStatusDelegate;
    if (m_projectNameDelegate) delete m_projectNameDelegate;
}

void ProjectsListView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);

        // setup model lists
        m_projectStatus.setStringList(DatabaseObjects::project_status);
        m_statusItemStatus.setStringList(DatabaseObjects::status_item_status);
        m_invoicingPeriod.setStringList(DatabaseObjects::invoicing_period);
        m_statusReportPeriod.setStringList(DatabaseObjects::status_report_period);

        DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(dynamic_cast<SortFilterProxyModel*>(model)->sourceModel())->getDBOs();

        // projects list panel delagets
        m_unfilteredPeopleDelegate = new SqlComboBoxDelegate(this, dbo->unfilteredpeoplemodelproxy());
        m_projectClientsDelegate = new SqlComboBoxDelegate(this, dbo->unfilteredclientsmodelproxy());
        m_projectDateDelegate = new DateEditDelegate(this);
        m_projectsReportPeriodDelegate = new ComboBoxDelegate(this, &m_statusReportPeriod);
        m_projectInvoicingPeriodDelegate = new ComboBoxDelegate(this, &m_invoicingPeriod);
        m_projectStatusDelegate = new ComboBoxDelegate(this, &m_projectStatus);

        m_projectNameDelegate = new PlainTextEditDelegate(this);

        setItemDelegateForColumn(2, m_projectNameDelegate);
        setItemDelegateForColumn(5, m_unfilteredPeopleDelegate);
        setItemDelegateForColumn(3, m_projectDateDelegate);
        setItemDelegateForColumn(4, m_projectDateDelegate);
        setItemDelegateForColumn(11, m_projectInvoicingPeriodDelegate);
        setItemDelegateForColumn(12, m_projectsReportPeriodDelegate);
        setItemDelegateForColumn(13, m_projectClientsDelegate);
        setItemDelegateForColumn(14, m_projectStatusDelegate);

    }
    else
    {
        TableView::setModel(model);
    }
}

