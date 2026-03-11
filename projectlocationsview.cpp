// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectlocationsview.h"
#include "databaseobjects.h"

#include <QFileInfo>
#include <QUrl>

ProjectLocationsView::ProjectLocationsView(QWidget* parent) : TableView(parent)
{
    setObjectName("tableViewProjectLocations");
    setHasOpen(true);
}

ProjectLocationsView::~ProjectLocationsView()
{
    if (m_fileTypeDelegate) delete m_fileTypeDelegate;
    if (m_fileButtonDelegate) delete m_fileButtonDelegate;
    if (m_descriptionDelegate) delete m_descriptionDelegate;
}

void ProjectLocationsView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(5, true);
        setColumnHidden(6, true);

        // see setbuttonitems for visible columns

        // setup model lists
        m_fileTypes.setStringList(DatabaseObjects::file_types);

        // projects list panel delagets
        m_fileTypeDelegate = new ComboBoxDelegate(this, &m_fileTypes);
        m_fileButtonDelegate = new LineEditFileButtonDelegate(this);
        m_descriptionDelegate = new PlainTextEditDelegate(this);

        // assign delegates to columns
        setItemDelegateForColumn(2, m_fileTypeDelegate);
        setItemDelegateForColumn(4, m_fileButtonDelegate);
        setItemDelegateForColumn(3, m_descriptionDelegate);
    }
    else
    {
        TableView::setModel(model);
    }
}

void ProjectLocationsView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<ProjectLocationsModel*>(currentmodel)->getFilter(1); // get the project id

    dynamic_cast<ProjectLocationsModel*>(currentmodel)->newRecord(&fk_value1);
}
