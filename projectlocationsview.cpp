// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectlocationsview.h"
#include "pndatabaseobjects.h"

#include <QFileInfo>
#include <QUrl>

ProjectLocationsView::ProjectLocationsView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewProjectLocations");
    setHasOpen(true);
}

ProjectLocationsView::~ProjectLocationsView()
{
    if (m_file_type_delegate) delete m_file_type_delegate;
    if (m_file_button_delegate) delete m_file_button_delegate;
    if (m_description_delegate) delete m_description_delegate;
}

void ProjectLocationsView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);

        // see setbuttonitems for visible columns

        // setup model lists
        m_file_types.setStringList(PNDatabaseObjects::file_types);

        // projects list panel delagets
        m_file_type_delegate = new ComboBoxDelegate(this, &m_file_types);
        m_file_button_delegate = new PNLineEditFileButtonDelegate(this);
        m_description_delegate = new PNPlainTextEditDelegate(this);

        // assign delegates to columns
        setItemDelegateForColumn(2, m_file_type_delegate);
        setItemDelegateForColumn(4, m_file_button_delegate);
        setItemDelegateForColumn(3, m_description_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

void ProjectLocationsView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<ProjectLocationsModel*>(currentmodel)->getFilter(1); // get the project id

    dynamic_cast<ProjectLocationsModel*>(currentmodel)->newRecord(&fk_value1);
}
