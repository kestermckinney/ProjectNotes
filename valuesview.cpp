#include "valuesview.h"
#include <QMouseEvent>

ValuesView::ValuesView(QWidget *parent) : PNTableView(parent)
{
    setSelectionMode(QAbstractItemView::MultiSelection);

    this->installEventFilter(this);
}

ValuesView::~ValuesView()
{
    this->removeEventFilter(this);
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

bool ValuesView::eventFilter(QObject* watched, QEvent *event)
{
    Q_UNUSED(watched);

    switch (event->type()) {
    case QEvent::FocusAboutToChange:
    {

        QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
        PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();
        QString column = currentmodel->getColumnName(0);

        QModelIndexList qil = this->selectionModel()->selectedRows();

        (*m_SavedFilters)[column].ColumnValues.clear();

        for (QModelIndexList::iterator qi = qil.begin(); qi != qil.end(); qi++)
        {
            QVariant val = currentmodel->data(currentmodel->index((*qi).row(), 0));
            (*m_SavedFilters)[column].ColumnValues.append(val.toString());
        }

        return true;
    }
    case QEvent::MouseButtonRelease:
    {

        QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
        PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();
        QString column = currentmodel->getColumnName(0);

        QModelIndexList qil = this->selectionModel()->selectedRows();

        (*m_SavedFilters)[column].ColumnValues.clear();

        for (QModelIndexList::iterator qi = qil.begin(); qi != qil.end(); qi++)
        {
            QVariant val = currentmodel->data(currentmodel->index((*qi).row(), 0));
            (*m_SavedFilters)[column].ColumnValues.append(val.toString());
        }

        return true;
    }
    default:
        break;
    }
    return false;
}
