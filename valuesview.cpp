#include "valuesview.h"

ValuesView::ValuesView(QWidget *parent) : PNTableView(parent)
{

}

void ValuesView::dataRowSelected(const QModelIndex &index)
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    QString column = currentmodel->getColumnName(0);

    // if the item is selected add it to the column values
    if (this->selectionModel()->isRowSelected(index.row()) )
    {
        QVariant val = currentmodel->data(currentmodel->index(index.row(), 0));

        if (  !(*m_SavedFilters)[column].ColumnValues.contains(val.toString()) )
        {
            (*m_SavedFilters)[column].ColumnValues.append(val.toString());
        }
    }
    // if the item is removed make sure it isn't in the list
    else
    {
        QVariant val = currentmodel->data(currentmodel->index(index.row(), 0));

        if (  (*m_SavedFilters)[column].ColumnValues.contains(val.toString()) )
        {
            (*m_SavedFilters)[column].ColumnValues.removeAll(val.toString());
        }
    }
}
