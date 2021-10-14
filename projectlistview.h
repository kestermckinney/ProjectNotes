#ifndef PROJECTLISTVIEW_H
#define PROJECTLISTVIEW_H

#include "pncomboboxdelegate.h"
#include "pndateeditdelegate.h"
#include "comboboxdelegate.h"
#include "pnsortfilterproxymodel.h"
#include "pntableview.h"

#include <QObject>

class ProjectListView : public PNTableView
{
public:

    ProjectListView(QWidget* parent = nullptr);
    void setModel(QAbstractItemModel *model) override;

private:
    QStringListModel m_ItemType;//(PNDatabaseObjects::item_type);
    QStringListModel m_ItemStatus;//PNDatabaseObjects::item_status;
    QStringListModel m_ItemPriority;//PNDatabaseObjects::item_priority;
    QStringListModel m_ProjectStatus;//PNDatabaseObjects::project_status;
    QStringListModel m_StatusItemStatus;//PNDatabaseObjects::status_item_status;
    QStringListModel m_InvoicingPeriod;//PNDatabaseObjects::invoicing_period;
    QStringListModel m_StatusReportPeriod; //PNDatabaseObjects::status_report_period;
    QStringListModel m_Locations;//PNDatabaseObjects::locations;

    // projects list panel delegates
    PNComboBoxDelegate* m_UnfilteredPeopleDelegate;
    PNComboBoxDelegate* m_ProjectClientsDelegate;
    PNDateEditDelegate* m_ProjectDateDelegate;
    ComboBoxDelegate* m_ProjectInvoicegPeriodDelegate;
    ComboBoxDelegate* m_ProjectStatusDelegate;
    ComboBoxDelegate* m_ProjectsReportPeriodDelegate;

};

#endif // PROJECTLISTVIEW_H
