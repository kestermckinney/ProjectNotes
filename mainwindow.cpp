#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStringListModel>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QDebug>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //m_DBObjects = nullptr;

    //QString path = "/Users/paulmckinney/ProjectNotes.db";
    QString path = "/home/paulmckinney/ProjectNotes3/database/ProjectNotes.db";

    //m_DBObjects = new PNDatabaseObjects(path, this);

    if (!global_DBObjects.OpenDatabase(path))
        return;

    global_DBObjects.projectslistmodel()->Refresh();
    global_DBObjects.unfilteredpeoplemodel()->Refresh();
    global_DBObjects.unfilteredclientsmodel()->Refresh();
    global_DBObjects.clientsmodel()->Refresh();

    //ui->tableViewProjects->setModel(global_DBObjects.projectslistmodel());
    ui->tableViewProjects->setModel(global_DBObjects.projectslistmodelproxy());

    ui->tableViewProjects->setColumnHidden(0, true);

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

    //QStringListModel report_periods = QStringListModel(PNDatabaseObjects::status_report_period);

    ui->tableViewProjects->setItemDelegateForColumn(5, m_UnfilteredPeopleDelegate);
    ui->tableViewProjects->setItemDelegateForColumn(3, m_ProjectDateDelegate);
    ui->tableViewProjects->setItemDelegateForColumn(4, m_ProjectDateDelegate);
    ui->tableViewProjects->setItemDelegateForColumn(11, m_ProjectInvoicegPeriodDelegate);
    ui->tableViewProjects->setItemDelegateForColumn(12, m_ProjectsReportPeriodDelegate);
    ui->tableViewProjects->setItemDelegateForColumn(13, m_ProjectClientsDelegate);
    ui->tableViewProjects->setItemDelegateForColumn(14, m_ProjectStatusDelegate);

    ui->tableViewProjects->setSortingEnabled(true);
    //ui->tableViewProjects->sortByColumn(3, Qt::AscendingOrder);

    //ui->actionBack->setVisible(false);

    // connect events
    //connect(ui->pushButtonNewProject, &QPushButton::clicked, this, &MainWindow::handleNewProjectClicked);
    //connect(ui->pushButtonDeleteProject, &QPushButton::clicked, this, &MainWindow::handleDeleteProjectClicked);

    global_Settings.getWindowState("MainWindow", *this);
    global_Settings.getTableViewState("ProjectTableColumns", *(ui->tableViewProjects));
}

MainWindow::~MainWindow()
{
    // disconnect events
    //disconnect(ui->pushButtonNewProject, &QPushButton::clicked, this, &MainWindow::handleNewProjectClicked);
    //disconnect(ui->pushButtonDeleteProject, &QPushButton::clicked, this, &MainWindow::handleDeleteProjectClicked);
    global_Settings.setTableViewState("ProjectTableColumns", *(ui->tableViewProjects));

    ui->tableViewProjects->setModel(nullptr);

    global_DBObjects.CloseDatabase();

    //delete m_DBObjects;

    global_Settings.setWindowState("MainWindow", *this);

    delete ui;
}
/*
void MainWindow::handleDeleteProjectClicked()
{
    QModelIndexList qi = ui->tableViewProjects->selectionModel()->selectedRows();

    for (int i = qi.count() - 1; i >= 0; i--)
    {
        global_DBObjects.projectinformationmodel()->DeleteRecord(qi[i]);
    }
}
*/

// TODO: Save column resizing that is done
// TODO: Save sorting that is done
// TODO: Allow for a reset of columnn sizing and sorting
