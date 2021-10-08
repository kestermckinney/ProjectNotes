#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pndatabaseobjects.h"
#include "pnsettings.h"
#include "pncomboboxdelegate.h"
#include "pndateeditdelegate.h"
#include "comboboxdelegate.h"
#include "pnsortfilterproxymodel.h"

#include <QMainWindow>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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

private slots:
    //void handleNewProjectClicked();
    //void handleDeleteProjectClicked();

private:
    Ui::MainWindow *ui;
};

static PNSettings global_Settings;

#endif // MAINWINDOW_H
