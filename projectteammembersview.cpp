// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectteammembersview.h"
#include "databaseobjects.h"

ProjectTeamMembersView::ProjectTeamMembersView(QWidget* parent) : TableView(parent)
{
    setHasOpen(true);
}

ProjectTeamMembersView::~ProjectTeamMembersView()
{
    if (m_unfilteredPeopleDelegate) delete m_unfilteredPeopleDelegate;
    if (m_receiveStatusDelegate) delete m_receiveStatusDelegate;
    if (m_roleDelegate) delete m_roleDelegate;
}

void ProjectTeamMembersView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(3, true);
        setColumnHidden(6, true);
        setColumnHidden(7, true);
        setColumnHidden(8, true);
        setColumnHidden(9, true);
        // setup model lists

        DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(dynamic_cast<SortFilterProxyModel*>(model)->sourceModel())->getDBOs();

        // projects list panel delagets
        m_unfilteredPeopleDelegate = new SqlComboBoxDelegate(this, dbo->unfilteredpeoplemodelproxy());
        m_receiveStatusDelegate = new CheckBoxDelegate(this);
        m_roleDelegate = new PlainTextEditDelegate(this);

        setItemDelegateForColumn(2, m_unfilteredPeopleDelegate);
        setItemDelegateForColumn(5, m_roleDelegate);
        setItemDelegateForColumn(4, m_receiveStatusDelegate);
    }
    else
    {
        TableView::setModel(model);
    }
}

void ProjectTeamMembersView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<ProjectTeamMembersModel*>(currentmodel)->getFilter(1); // get the project id

    dynamic_cast<ProjectTeamMembersModel*>(currentmodel)->newRecord(&fk_value1);
}
