#include "valuesview.h"
#include <QMouseEvent>
#include <QDebug>

ValuesView::ValuesView(QWidget *t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewFilterValues");  // this will need set differently for every instance of filterdialog

    setSelectionMode(QAbstractItemView::MultiSelection);
}

ValuesView::~ValuesView()
{

}

void ValuesView::dataRowSelected(const QModelIndex &t_index)
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    QString column = currentmodel->getColumnName(0);

    // if the item is selected add it to the column values
    if (this->selectionModel()->isRowSelected(t_index.row()) )
    {
        QVariant val = sortmodel->data(sortmodel->index(t_index.row(), 0));

        if (  !(*m_saved_filters)[column].ColumnValues.contains(val.toString()) )
        {
            (*m_saved_filters)[column].ColumnValues.append(val.toString());
            //qDebug() << val << " was added to the selected items for column " << column;
        }
    }
    // if the item is removed make sure it isn't in the list
    else
    {
        QVariant val = sortmodel->data(sortmodel->index(t_index.row(), 0));

        if (  (*m_saved_filters)[column].ColumnValues.contains(val.toString()) )
        {
            (*m_saved_filters)[column].ColumnValues.removeAll(val.toString());
            //qDebug() << val << " was removed to the selected items for column " << column;
        }
    }
}
