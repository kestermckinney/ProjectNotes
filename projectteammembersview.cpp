// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectteammembersview.h"
#include "pndatabaseobjects.h"

ProjectTeamMembersView::ProjectTeamMembersView(QWidget* t_parent) : PNTableView(t_parent)
{
    setHasOpen(true);
}

ProjectTeamMembersView::~ProjectTeamMembersView()
{
    if (m_unfiltered_people_delegate) delete m_unfiltered_people_delegate;
    if (m_receive_status_delegate) delete m_receive_status_delegate;
    if (m_role_delegate) delete m_role_delegate;
}

void ProjectTeamMembersView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(3, true);
        setColumnHidden(6, true);
        setColumnHidden(7, true);
        setColumnHidden(8, true);
        setColumnHidden(9, true);
        // setup model lists

        // projects list panel delagets
        m_unfiltered_people_delegate = new PNComboBoxDelegate(this, global_DBObjects.unfilteredpeoplemodel());
        m_receive_status_delegate = new PNCheckBoxDelegate(this);
        m_role_delegate = new PNPlainTextEditDelegate(this);

        setItemDelegateForColumn(2, m_unfiltered_people_delegate);
        setItemDelegateForColumn(5, m_role_delegate);
        setItemDelegateForColumn(4, m_receive_status_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

void ProjectTeamMembersView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<ProjectTeamMembersModel*>(currentmodel)->getFilter(1); // get the project id

    dynamic_cast<ProjectTeamMembersModel*>(currentmodel)->newRecord(&fk_value1);
}
