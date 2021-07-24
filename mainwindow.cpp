#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pncomboboxdelegate.h"
#include "pndateeditdelegate.h"
#include "comboboxdelegate.h"

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

    global_DBObjects.projectinformationmodel()->Refresh();
    global_DBObjects.peoplemodel()->Refresh();

    ui->tableViewProjects->setModel(global_DBObjects.projectinformationmodel());
    ui->tableViewProjects->setColumnHidden(0, true);


    PNComboBoxDelegate* peopledelegate = new PNComboBoxDelegate(this, global_DBObjects.peoplemodel());
    PNDateEditDelegate* datedelegate = new PNDateEditDelegate(this);

    QStringListModel* invoice_periods = new QStringListModel(PNDatabaseObjects::invoicing_period);
    ComboBoxDelegate* periodsdelegate = new ComboBoxDelegate(this, invoice_periods);

    //QStringListModel report_periods = QStringListModel(PNDatabaseObjects::status_report_period);


    ui->tableViewProjects->setItemDelegateForColumn(5, peopledelegate);
    ui->tableViewProjects->setItemDelegateForColumn(3, datedelegate);
    ui->tableViewProjects->setItemDelegateForColumn(4, datedelegate);
    ui->tableViewProjects->setItemDelegateForColumn(11, periodsdelegate);

    // connect events
    connect(ui->pushButtonNewProject, &QPushButton::clicked, this, &MainWindow::handleNewProjectClicked);
    connect(ui->pushButtonDeleteProject, &QPushButton::clicked, this, &MainWindow::handleDeleteProjectClicked);
}

MainWindow::~MainWindow()
{
    // disconnect events
    disconnect(ui->pushButtonNewProject, &QPushButton::clicked, this, &MainWindow::handleNewProjectClicked);
    disconnect(ui->pushButtonDeleteProject, &QPushButton::clicked, this, &MainWindow::handleDeleteProjectClicked);

    ui->tableViewProjects->setModel(nullptr);

    global_DBObjects.CloseDatabase();

    //delete m_DBObjects;

    delete ui;
}

void MainWindow::handleNewProjectClicked()
{
    global_DBObjects.projectinformationmodel()->AddProject();
}

void MainWindow::handleDeleteProjectClicked()
{
    QModelIndexList qi = ui->tableViewProjects->selectionModel()->selectedRows();

    for (int i = qi.count() - 1; i >= 0; i--)
    {
        global_DBObjects.projectinformationmodel()->DeleteRecord(qi[i]);
    }
}
