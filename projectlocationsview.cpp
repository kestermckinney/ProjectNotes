// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectlocationsview.h"
#include "pndatabaseobjects.h"

#include <QFileInfo>
#include <QDesktopServices>
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

void ProjectLocationsView::slotOpenRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    QModelIndexList qil = this->selectionModel()->selectedRows();
    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);

    QVariant location = model()->data(model()->index(qq.row(), 4));
    QVariant location_type = model()->data(model()->index(qq.row(), 2));

    if ( location_type == "Web Link" )
    {
        QDesktopServices::openUrl(QUrl(location.toString(), QUrl::TolerantMode));
    }
    else
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(location.toString()));
    }
}
