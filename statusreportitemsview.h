#ifndef STATUSREPORTITEMSVIEW_H
#define STATUSREPORTITEMSVIEW_H

#include "comboboxdelegate.h"
#include "pntableview.h"

#include <QObject>

class StatusReportItemsView : public PNTableView
{
public:
    StatusReportItemsView(QWidget* t_parent = nullptr);
    ~StatusReportItemsView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    QStringListModel m_status_items_status; // (PNDatabaseObjects::status_item_status);

    // projects list panel delegates
    ComboBoxDelegate* m_status_items_status_delegate = nullptr;
};

#endif // STATUSREPORTITEMSVIEW_H
