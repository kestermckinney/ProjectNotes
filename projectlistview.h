#ifndef PROJECTLISTVIEW_H
#define PROJECTLISTVIEW_H

#include "pncomboboxdelegate.h"
#include "pndateeditdelegate.h"
#include "comboboxdelegate.h"
#include "pnsortfilterproxymodel.h"
#include "pntableview.h"
#include "pnbasepage.h"

#include <QObject>

class ProjectListView : public PNTableView
{
public:

    ProjectListView(QWidget* t_parent = nullptr);

    void setModel(QAbstractItemModel *t_model) override;

private:
    QStringListModel m_item_type;//(PNDatabaseObjects::item_type);
    QStringListModel m_item_status;//PNDatabaseObjects::item_status;
    QStringListModel m_item_priority;//PNDatabaseObjects::item_priority;
    QStringListModel m_project_status;//PNDatabaseObjects::project_status;
    QStringListModel m_status_item_status;//PNDatabaseObjects::status_item_status;
    QStringListModel m_invoicing_period;//PNDatabaseObjects::invoicing_period;
    QStringListModel m_status_report_period; //PNDatabaseObjects::status_report_period;
    QStringListModel m_locations;//PNDatabaseObjects::locations;

    // projects list panel delegates
    PNComboBoxDelegate* m_unfiltered_people_delegate;
    PNComboBoxDelegate* m_project_clients_delegate;
    PNDateEditDelegate* m_project_date_delegate;
    ComboBoxDelegate* m_project_invoicing_period_delegate;
    ComboBoxDelegate* m_project_status_delegate;
    ComboBoxDelegate* m_projects_report_period_delegate;

};

#endif // PROJECTLISTVIEW_H
