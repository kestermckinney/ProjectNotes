#include "projectlistview.h"

ProjectListView::ProjectListView(QWidget* parent) : PNTableView(parent)
{

}

void ProjectListView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        PNTableView::setModel(model);

        setColumnHidden(0, true);

        // setup model lists
        m_ItemType.setStringList(PNDatabaseObjects::item_type);
        m_ItemStatus.setStringList(PNDatabaseObjects::item_status);
        m_ItemPriority.setStringList(PNDatabaseObjects::item_priority);
        m_ProjectStatus.setStringList(PNDatabaseObjects::project_status);
        m_StatusItemStatus.setStringList(PNDatabaseObjects::status_item_status);
        m_InvoicingPeriod.setStringList(PNDatabaseObjects::invoicing_period);
        m_Locations.setStringList(PNDatabaseObjects::locations);
        m_StatusReportPeriod.setStringList(PNDatabaseObjects::status_report_period);

        // projects list panel delagets
        m_UnfilteredPeopleDelegate = new PNComboBoxDelegate(this, global_DBObjects.unfilteredpeoplemodel());
        m_ProjectClientsDelegate = new PNComboBoxDelegate(this, global_DBObjects.clientsmodel());
        m_ProjectDateDelegate = new PNDateEditDelegate(this);
        m_ProjectsReportPeriodDelegate = new ComboBoxDelegate(this, &m_StatusReportPeriod);
        m_ProjectInvoicegPeriodDelegate = new ComboBoxDelegate(this, &m_InvoicingPeriod);
        m_ProjectStatusDelegate = new ComboBoxDelegate(this, &m_ProjectStatus);

        setItemDelegateForColumn(5, m_UnfilteredPeopleDelegate);
        setItemDelegateForColumn(3, m_ProjectDateDelegate);
        setItemDelegateForColumn(4, m_ProjectDateDelegate);
        setItemDelegateForColumn(11, m_ProjectInvoicegPeriodDelegate);
        setItemDelegateForColumn(12, m_ProjectsReportPeriodDelegate);
        setItemDelegateForColumn(13, m_ProjectClientsDelegate);
        setItemDelegateForColumn(14, m_ProjectStatusDelegate);

        setSortingEnabled(true);

    }
    else
    {
        PNTableView::setModel(model);
    }
}

