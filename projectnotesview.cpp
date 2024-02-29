// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectnotesview.h"
#include "projectnotesmodel.h"

ProjectNotesView::ProjectNotesView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewProjectNotes");
    setHasOpen(true);
}

ProjectNotesView::~ProjectNotesView()
{
    if (m_meeting_date_delegate) delete m_meeting_date_delegate;
    if (m_internal_item_delegate) delete m_internal_item_delegate;
    if (m_title_delegate) delete m_title_delegate;
}

void ProjectNotesView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(4, true);
        setColumnHidden(6, true);
        setColumnHidden(7, true);

        // see setbuttonitems for visible columns

        // projects list panel delagets
        m_meeting_date_delegate = new PNDateEditDelegate(this);
        m_internal_item_delegate = new PNCheckBoxDelegate(this);
        m_title_delegate = new PNPlainTextEditDelegate(this);

        // assign delegates to columns
        setItemDelegateForColumn(2, m_title_delegate);
        setItemDelegateForColumn(3, m_meeting_date_delegate);
        setItemDelegateForColumn(5, m_internal_item_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

void ProjectNotesView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<ProjectNotesModel*>(currentmodel)->getFilter(1); // get the project id

    dynamic_cast<ProjectNotesModel*>(currentmodel)->newRecord(&fk_value1);
}

