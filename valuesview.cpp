#include "valuesview.h"
#include <QMouseEvent>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


ValuesView::ValuesView(QWidget *parent) : TableView(parent)
{
    setObjectName("tableViewFilterValues");  // this will need set differently for every instance of filterdialog

    setSelectionMode(QAbstractItemView::MultiSelection);
}

ValuesView::~ValuesView()
{

}

void ValuesView::dataRowSelected(const QModelIndex &index)
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QString column = currentmodel->getColumnName(0);

    // if the item is selected add it to the column values
    if (this->selectionModel()->isRowSelected(index.row()) )
    {
        QVariant val = sortmodel->data(sortmodel->index(index.row(), 0));

        if (  !(*m_savedFilters)[column].ColumnValues.contains(val.toString()) )
        {
            (*m_savedFilters)[column].ColumnValues.append(val.toString());
            // qDebug() << val << " was added to the selected items for column " << column;
        }
    }
    // if the item is removed make sure it isn't in the list
    else
    {
        QVariant val = sortmodel->data(sortmodel->index(index.row(), 0));

        if (  (*m_savedFilters)[column].ColumnValues.contains(val.toString()) )
        {
            (*m_savedFilters)[column].ColumnValues.removeAll(val.toString());
            // qDebug() << val << " was removed to the selected items for column " << column;
        }
    }
}
