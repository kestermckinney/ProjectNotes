// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "notesactionitemsview.h"
#include "pndatabaseobjects.h"

NotesActionItemsView::NotesActionItemsView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewMeetingItems");
    setHasOpen(true);
}

NotesActionItemsView::~NotesActionItemsView()
{
    if (m_action_item_type_delegate) delete m_action_item_type_delegate;
    if (m_identified_by_delegate) delete m_identified_by_delegate;
    if (m_date_identified_delegate) delete m_date_identified_delegate;
    if (m_assigned_to_delegate) delete m_assigned_to_delegate;
    if (m_priority_delegate) delete m_priority_delegate;
    if (m_status_delegate) delete m_status_delegate;
    if (m_date_due_delegate) delete m_date_due_delegate;
    if (m_date_date_updated_delagate) delete m_date_date_updated_delagate;
    if (m_date_resolved_delegate) delete m_date_resolved_delegate;
    if (m_meeting_delegate) delete m_meeting_delegate;
    if (m_project_delegate) delete m_project_delegate;
    if (m_internal_delegate) delete m_internal_delegate;
    if (m_client_delegate) delete m_client_delegate;
    if (m_item_name_delegate) delete m_item_name_delegate;
    if (m_item_description_delegate) delete m_item_description_delegate;
}

void NotesActionItemsView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        // see setbuttonitems for visible columns
        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(2, true);
        setColumnHidden(4, true);
        setColumnHidden(5, true);
        setColumnHidden(6, true);
        setColumnHidden(8, true);
        setColumnHidden(9, true);
        setColumnHidden(11, true);
        setColumnHidden(12, true);
        setColumnHidden(13, true);
        setColumnHidden(14, true);
        setColumnHidden(18, true);

        PNDatabaseObjects* dbo = qobject_cast<PNSqlQueryModel*>(dynamic_cast<PNSortFilterProxyModel*>(t_model)->sourceModel())->getDBOs();

        if (dbo->getShowInternalItems())
        {
           setColumnHidden(15, false);
           resizeColumnToContents(15);
        }
        else
           setColumnHidden(15, true);

        // setup model lists
        m_item_priority.setStringList(PNDatabaseObjects::item_priority);
        m_item_type.setStringList(PNDatabaseObjects::item_type);
        m_item_status.setStringList(PNDatabaseObjects::item_status);

        // projects list panel delagets
        m_action_item_type_delegate = new ComboBoxDelegate(this, &m_item_type);
        m_identified_by_delegate = new PNComboBoxDelegate(this, dbo->teamsmodel(), 1, 3);
        m_date_identified_delegate = new PNDateEditDelegate(this);
        m_assigned_to_delegate = new PNComboBoxDelegate(this, dbo->teamsmodel(), 1, 3);
        m_priority_delegate = new ComboBoxDelegate(this, &m_item_priority);
        m_status_delegate = new ComboBoxDelegate(this, &m_item_status);
        m_date_due_delegate = new PNDateEditDelegate(this);
        m_date_date_updated_delagate = new PNDateEditDelegate(this);
        m_date_resolved_delegate = new PNDateEditDelegate(this);
        m_meeting_delegate = new PNComboBoxDelegate(this, dbo->actionitemsdetailsmeetingsmodel(), 2, 0);
        m_project_delegate = new PNComboBoxDelegate(this, dbo->projectslistmodel());
        m_internal_delegate = new PNCheckBoxDelegate(this);
        m_client_delegate = new PNComboBoxDelegate(this, dbo->unfilteredclientsmodel());
        m_item_name_delegate = new PNPlainTextEditDelegate(this);
        m_item_description_delegate = new PNPlainTextEditDelegate(this);

        // assign delegates to columns
        setItemDelegateForColumn(2, m_action_item_type_delegate);
        setItemDelegateForColumn(3, m_item_name_delegate);
        setItemDelegateForColumn(4, m_identified_by_delegate);
        setItemDelegateForColumn(5, m_date_identified_delegate);
        setItemDelegateForColumn(6, m_item_description_delegate);
        setItemDelegateForColumn(7, m_assigned_to_delegate);
        setItemDelegateForColumn(8, m_priority_delegate);
        setItemDelegateForColumn(9, m_status_delegate);
        setItemDelegateForColumn(10, m_date_due_delegate);
        setItemDelegateForColumn(11, m_date_date_updated_delagate);
        setItemDelegateForColumn(12, m_date_resolved_delegate);
        setItemDelegateForColumn(13, m_meeting_delegate);
        setItemDelegateForColumn(14, m_project_delegate);
        setItemDelegateForColumn(15, m_internal_delegate);
        setItemDelegateForColumn(18, m_client_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

void NotesActionItemsView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<NotesActionItemsModel*>(currentmodel)->getFilter(14);  // project id
    QVariant fk_value2 = dynamic_cast<NotesActionItemsModel*>(currentmodel)->getFilter(13); // notet id

    dynamic_cast<NotesActionItemsModel*>(currentmodel)->newRecord(&fk_value1, &fk_value2);
}
