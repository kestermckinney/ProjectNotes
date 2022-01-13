#include "columnview.h"

ColumnView::ColumnView(QWidget *t_parent) : PNTableView(t_parent)
{
    m_values_model = nullptr;
}

void ColumnView::dataRowSelected(const QModelIndex &t_index)
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    m_values_model->setValuesColumn(currentmodel->data(t_index).toString());
    // translate the diplay name to the database field name
    QString displaycolname = currentmodel->data(t_index).toString();
    QString dbcolname = m_filtered_model->getColumnName(displaycolname);

    // set all of the selected values
    for (int i = 0; i < m_values_model->rowCount(QModelIndex()); i++)
    {
        QVariant val = m_values_model->data(m_values_model->index(i, 0));

        if ( (*m_saved_filters)[dbcolname].ColumnValues.contains(val.toString()) )
            m_values_view->selectRow(i);
    }

    // set all of the search parameters
    m_parent_ui->setEndValue((*m_saved_filters)[dbcolname].t_search_end_value);
    m_parent_ui->setBeginValue((*m_saved_filters)[dbcolname].t_search_begin_value);
    m_parent_ui->setSearchText((*m_saved_filters)[dbcolname].SearchString);
}

