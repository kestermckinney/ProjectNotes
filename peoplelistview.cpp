// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "peoplelistview.h"
#include "databaseobjects.h"

PeopleListView::PeopleListView(QWidget* parent) : TableView(parent)
{
    setObjectName("tableViewPeople");
}

PeopleListView::~PeopleListView()
{
    if (m_unfilteredClientsDelegate) delete m_unfilteredClientsDelegate;
    if (m_nameDelegate) delete m_nameDelegate;
    if (m_roleDelegate) delete m_roleDelegate;
}

void PeopleListView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);

        DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(dynamic_cast<SortFilterProxyModel*>(model)->sourceModel())->getDBOs();

        // setup model lists
        m_unfilteredClientsDelegate = new SqlComboBoxDelegate(this, dbo->unfilteredclientsmodelproxy());
        m_nameDelegate = new PlainTextEditDelegate(this);
        m_roleDelegate = new PlainTextEditDelegate(this);

        setItemDelegateForColumn(5, m_unfilteredClientsDelegate);
        setItemDelegateForColumn(1, m_nameDelegate);
        setItemDelegateForColumn(6, m_roleDelegate);
    }
    else
    {
        TableView::setModel(model);
    }
}
