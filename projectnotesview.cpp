// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectnotesview.h"
#include "projectnotesmodel.h"

ProjectNotesView::ProjectNotesView(QWidget* parent) : TableView(parent)
{
    setObjectName("tableViewProjectNotes");
    setHasOpen(true);
}

ProjectNotesView::~ProjectNotesView()
{
    if (m_meetingDateDelegate) delete m_meetingDateDelegate;
    if (m_internalItemDelegate) delete m_internalItemDelegate;
    if (m_titleDelegate) delete m_titleDelegate;
}

void ProjectNotesView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(4, true);
        setColumnHidden(6, true);
        setColumnHidden(7, true);

        // see setbuttonitems for visible columns

        // projects list panel delagets
        m_meetingDateDelegate = new DateEditDelegate(this);
        m_internalItemDelegate = new CheckBoxDelegate(this);
        m_titleDelegate = new PlainTextEditDelegate(this);

        // assign delegates to columns
        setItemDelegateForColumn(2, m_titleDelegate);
        setItemDelegateForColumn(3, m_meetingDateDelegate);
        setItemDelegateForColumn(5, m_internalItemDelegate);
    }
    else
    {
        TableView::setModel(model);
    }
}

void ProjectNotesView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<ProjectNotesModel*>(currentmodel)->getFilter(1); // get the project id

    dynamic_cast<ProjectNotesModel*>(currentmodel)->newRecord(&fk_value1);
}

