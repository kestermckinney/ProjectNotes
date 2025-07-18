// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pndatabaseobjects.h"
#include "columnview.h"
#include "filterdatadialog.h"

#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


ColumnView::ColumnView(QWidget *t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewColumnName");

    m_values_model = nullptr;
}

void ColumnView::dataRowSelected(const QModelIndex &t_index)
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());

    m_values_model->setValuesColumn(sortmodel->data(t_index).toString());
    // translate the diplay name to the database field name
    QString displaycolname = sortmodel->data(t_index).toString();
    QString dbcolname = m_filtered_model->getColumnName(displaycolname);

    // determine column delegate set in the source view
    int col = m_filtered_model->getColumnNumber(dbcolname);

    if (m_values_view->columnWidth(0) <= 0)
        m_values_view->resizeColumnToContents(0);

    if ( col < 0 )  // if for some reason no column is selected bail out
        return;

    QAbstractItemDelegate* delegate = m_source_view->itemDelegateForColumn(col);
    m_values_view->setItemDelegateForColumn(0, delegate);

    // set all of the selected values
    for (int i = 0; i < m_values_view->model()->rowCount(QModelIndex()); i++)
    {
        QVariant val = m_values_view->model()->data(m_values_view->model()->index(i, 0));

        if ( (*m_saved_filters)[dbcolname].ColumnValues.contains(val.toString()) )
        {
            m_values_view->selectRow(i);
        }

    }

    // set all of the search parameters
    m_parent_ui->setEndValue((*m_saved_filters)[dbcolname].SearchEndValue);
    m_parent_ui->setBeginValue((*m_saved_filters)[dbcolname].SearchBeginValue);
    m_parent_ui->setSearchText((*m_saved_filters)[dbcolname].SearchString);

    if ( m_filtered_model->getType(col) == PNSqlQueryModel::DBString ||  m_filtered_model->getType(col) == PNSqlQueryModel::DBHtml )
        m_parent_ui->setSearchTextEnabled(true);
    else
        m_parent_ui->setSearchTextEnabled(false);
}

