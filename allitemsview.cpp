// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "allitemsview.h"
#include "databaseobjects.h"
#include <QHeaderView>

AllItemsView::AllItemsView(QWidget* parent) : TrackerItemsView(parent)
{
    setObjectName("tableViewAllItems");
}

AllItemsView::~AllItemsView()
{
    delete m_allIdentifiedByDelegate;
    delete m_allAssignedToDelegate;
    delete m_allProjectDelegate;
    delete m_allUpdatedDelegate;
}

void AllItemsView::setModel(QAbstractItemModel *model)
{
    TrackerItemsView::setModel(model);

    if (model)
    {
        DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(
            dynamic_cast<SortFilterProxyModel*>(model)->sourceModel())->getDBOs();

        // Replace the project-filtered people delegates with unfiltered ones
        // so identified_by and assigned_to display correctly for all projects.
        // unfilteredpeoplemodelproxy: col 0 = id, col 1 = name
        delete m_allIdentifiedByDelegate;
        m_allIdentifiedByDelegate = new SqlComboBoxDelegate(this, dbo->unfilteredpeoplemodelproxy(), 1, 0);
        m_allIdentifiedByDelegate->setReadOnly(true);
        setItemDelegateForColumn(4, m_allIdentifiedByDelegate);

        delete m_allAssignedToDelegate;
        m_allAssignedToDelegate = new SqlComboBoxDelegate(this, dbo->unfilteredpeoplemodelproxy(), 1, 0);
        m_allAssignedToDelegate->setReadOnly(true);
        setItemDelegateForColumn(7, m_allAssignedToDelegate);

        delete m_allUpdatedDelegate;
        m_allUpdatedDelegate = new DateEditDelegate(this);
        m_allUpdatedDelegate->setReadOnly(true);
        setItemDelegateForColumn(11, m_allUpdatedDelegate);

        // Replace the status-filtered projects delegate with an unfiltered one
        // so project_number shows for closed projects too.
        // unfilteredprojectslistmodelproxy: col 0 = id, col 1 = project_number
        delete m_allProjectDelegate;
        m_allProjectDelegate = new SqlComboBoxDelegate(this, dbo->unfilteredprojectslistmodelproxy(), 1, 0);
        m_allProjectDelegate->setReadOnly(true);
        setItemDelegateForColumn(14, m_allProjectDelegate);

        // Hide the meeting column — not meaningful across all projects
        setColumnHidden(13, true);

        // Hide project_status and the duplicate read-only project_number columns
        setColumnHidden(17, true);
        setColumnHidden(20, true);

        // Show project_name (hidden by base class TrackerItemsView)
        setColumnHidden(19, false);

        // Move the project column (logical 14) to the first visual position,
        // then project_name (logical 19) immediately after it
        int fromVisual = horizontalHeader()->visualIndex(14);
        horizontalHeader()->moveSection(fromVisual, 0);

        fromVisual = horizontalHeader()->visualIndex(19);
        horizontalHeader()->moveSection(fromVisual, 1);
    }
}

void AllItemsView::slotNewRecord()
{
    // New records are not supported from the master item list
    // since a project context is required to generate item numbers
}
