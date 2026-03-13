// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseobjects.h"
#include "columnview.h"
#include "filterdatadialog.h"

#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


ColumnView::ColumnView(QWidget *parent) : TableView(parent)
{
    setObjectName("tableViewColumnName");

    m_valuesModel = nullptr;
}

void ColumnView::dataRowSelected(const QModelIndex &index)
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());

    m_valuesModel->setValuesColumn(sortmodel->data(index).toString());
    // translate the diplay name to the database field name
    QString displaycolname = sortmodel->data(index).toString();
    QString dbcolname = m_filteredModel->getColumnName(displaycolname);

    m_valuesModel->renameColumn(0, dbcolname, displaycolname);

    // determine column delegate set in the source view
    int col = m_filteredModel->getColumnNumber(dbcolname);

    if (m_valuesView->columnWidth(0) <= 0)
        m_valuesView->resizeColumnToContents(0);

    if ( col < 0 )  // if for some reason no column is selected bail out
        return;

    QAbstractItemDelegate* delegate = m_sourceView->itemDelegateForColumn(col);
    m_valuesView->setItemDelegateForColumn(0, delegate);

    // set all of the selected values
    for (int i = 0; i < m_valuesView->model()->rowCount(QModelIndex()); i++)
    {
        QVariant val = m_valuesView->model()->data(m_valuesView->model()->index(i, 0));

        if ( (*m_savedFilters)[dbcolname].ColumnValues.contains(val.toString()) )
        {
            m_valuesView->selectRow(i);
        }

    }

    // set all of the search parameters
    m_parentUi->setEndValue((*m_savedFilters)[dbcolname].SearchEndValue);
    m_parentUi->setBeginValue((*m_savedFilters)[dbcolname].SearchBeginValue);
    m_parentUi->setSearchText((*m_savedFilters)[dbcolname].SearchString);

    if ( m_filteredModel->getType(col) == SqlQueryModel::DBString ||  m_filteredModel->getType(col) == SqlQueryModel::DBHtml )
        m_parentUi->setSearchTextEnabled(true);
    else
        m_parentUi->setSearchTextEnabled(false);
}

