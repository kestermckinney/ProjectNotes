#include "statusreportitemsview.h"
#include "pndatabaseobjects.h"

StatusReportItemsView::StatusReportItemsView(QWidget* t_parent) : PNTableView(t_parent)
{

}


StatusReportItemsView::~StatusReportItemsView()
{
    if (m_status_items_status_delegate) delete m_status_items_status_delegate;
}

void StatusReportItemsView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);

        // setup model lists
        m_status_items_status.setStringList(PNDatabaseObjects::status_item_status);

        // projects list panel delagets
        m_status_items_status_delegate = new ComboBoxDelegate(this, &m_status_items_status);

        setItemDelegateForColumn(2, m_status_items_status_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

