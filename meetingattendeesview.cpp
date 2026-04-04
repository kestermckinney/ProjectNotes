// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "meetingattendeesview.h"
#include "databaseobjects.h"

MeetingAttendeesView::MeetingAttendeesView(QWidget* parent) : TableView(parent)
{
    setHasOpen(true);
    setKeyToOpenField(2);
}

MeetingAttendeesView::~MeetingAttendeesView()
{
    if (m_unfilteredPeopleDelegate) delete m_unfilteredPeopleDelegate;
}

void MeetingAttendeesView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(3, true);
        setColumnHidden(4, true);
        setColumnHidden(5, true);
        setColumnHidden(6, true);
        setColumnHidden(7, true);
        setColumnHidden(8, true);

        // setup model lists

        DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(dynamic_cast<SortFilterProxyModel*>(model)->sourceModel())->getDBOs();

        // projects list panel delagets
        m_unfilteredPeopleDelegate = new SqlComboBoxDelegate(this, dbo->teamsmodelproxy(), 1, 3);

        setItemDelegateForColumn(2, m_unfilteredPeopleDelegate);
    }
    else
    {
        TableView::setModel(model);
    }
}

void MeetingAttendeesView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<MeetingAttendeesModel*>(currentmodel)->getFilter(1); // get the project id

    dynamic_cast<MeetingAttendeesModel*>(currentmodel)->newRecord(&fk_value1);
}
