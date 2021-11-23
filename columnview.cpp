#include "columnview.h"

ColumnView::ColumnView(QWidget *parent) : PNTableView(parent)
{
    valuesModel = nullptr;
}

void ColumnView::dataRowSelected(const QModelIndex &index)
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    valuesModel->setValuesColumn(currentmodel->data(index).toString());
    // translate the diplay name to the database field name
    QString displaycolname = currentmodel->data(index).toString();
    QString dbcolname = filteredModel->getColumnName(displaycolname);

    // set all of the selected values
    for (int i = 0; i < valuesModel->rowCount(QModelIndex()); i++)
    {
        QVariant val = valuesModel->data(valuesModel->index(i, 0));

        if ( (*savedFilters)[dbcolname].ColumnValues.contains(val.toString()) )
            valuesView->selectRow(i);
    }
// STOPPED HERE this code doesn't select all the rows

    // set all of the search parameters
    ParentUI->setEndValue((*savedFilters)[dbcolname].SearchEndValue);
    ParentUI->setBeginValue((*savedFilters)[dbcolname].SearchBeginValue);
    ParentUI->setSearchText((*savedFilters)[dbcolname].SearchString);
}

// TODO: don't list duplicate values and blank rows in the selected values
