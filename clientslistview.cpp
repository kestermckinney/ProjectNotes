#include "clientslistview.h"

ClientsListView::ClientsListView(QWidget* t_parent) : PNTableView(t_parent)
{

}

void ClientsListView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);

    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

