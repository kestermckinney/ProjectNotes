#include "statusreportitemsview.h"
#include "databaseobjects.h"
#include "plaintexteditdelegate.h"

StatusReportItemsView::StatusReportItemsView(QWidget* parent) : TableView(parent)
{
    setObjectName("tableViewStatusReportItems");
}


StatusReportItemsView::~StatusReportItemsView()
{
    if (m_statusItemsStatusDelegate) delete m_statusItemsStatusDelegate;
    if (m_statusItemDescription) delete m_statusItemDescription;
}

void StatusReportItemsView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);

        // setup model lists
        m_statusItemsStatus.setStringList(DatabaseObjects::status_item_status);

        // projects list panel delagets
        m_statusItemsStatusDelegate = new ComboBoxDelegate(this, &m_statusItemsStatus);
        m_statusItemDescription = new PlainTextEditDelegate(this);

        setItemDelegateForColumn(2, m_statusItemsStatusDelegate);
        setItemDelegateForColumn(3, m_statusItemDescription);
    }
    else
    {
        TableView::setModel(model);
    }
}

void StatusReportItemsView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<StatusReportItemsModel*>(currentmodel)->getFilter(1); // get the project id

    dynamic_cast<StatusReportItemsModel*>(currentmodel)->newRecord(&fk_value1);
}
