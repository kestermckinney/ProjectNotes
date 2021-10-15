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
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (!global_Settings.getLastDatabase().toString().isEmpty())
        OpenDatabase(global_Settings.getLastDatabase().toString());

    // connect events
    //connect(ui->pushButtonNewProject, &QPushButton::clicked, this, &MainWindow::handleNewProjectClicked);
    //connect(ui->pushButtonDeleteProject, &QPushButton::clicked, this, &MainWindow::handleDeleteProjectClicked);

    setButtonAndMenuStates();

    global_Settings.getWindowState("MainWindow", *this);
    ui->actionStatus_Bar->setChecked(statusBar()->isVisibleTo(this));
}

MainWindow::~MainWindow()
{
    // disconnect events
    //disconnect(ui->pushButtonNewProject, &QPushButton::clicked, this, &MainWindow::handleNewProjectClicked);
    //disconnect(ui->pushButtonDeleteProject, &QPushButton::clicked, this, &MainWindow::handleDeleteProjectClicked);

    ui->tableViewProjects->setModel(nullptr);

    if (global_DBObjects.isOpen())
        global_DBObjects.CloseDatabase();

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

void MainWindow::setButtonAndMenuStates()
{
    bool dbopen = global_DBObjects.isOpen();

    ui->actionSearch->setEnabled(dbopen);
    ui->actionXML_Export->setEnabled(dbopen);
    ui->actionXML_Import->setEnabled(dbopen);
    ui->actionBackup_Database->setEnabled(dbopen);

    /*
    QAction *actionFind;
    QAction *actionSpell_Check;
    QAction *actionUndo;
    QAction *actionRedo;
    QAction *actionCopy;
    QAction *actionCut;
    QAction *actionPaste;
    QAction *actionDelete;
    QAction *actionSelect_All;
    */


    ui->actionInternal_Items->setEnabled(dbopen);
    ui->actionAll_Tracker_Action_Items->setEnabled(dbopen);

    ui->actionProjects->setEnabled(dbopen);
    ui->actionClosed_Projects->setEnabled(dbopen);

    if (dbopen)
        ui->actionClosed_Projects->setChecked(global_DBObjects.GetShowClosedProjects());

    ui->actionNew_Item->setEnabled(dbopen);
    ui->actionCopy_Item->setEnabled(dbopen);
    ui->actionDelete_Item->setEnabled(dbopen);
    ui->actionEdit_Itesm->setEnabled(dbopen);
    ui->actionBack->setEnabled(dbopen);
    ui->actionForward->setEnabled(dbopen);
    ui->actionClients->setEnabled(dbopen);
    ui->actionPeople->setEnabled(dbopen);
    ui->actionFilter->setEnabled(dbopen);
}

// TODO: Save column resizing that is done
// TODO: Save sorting that is done
// TODO: Allow for a reset of columnn sizing and sorting

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionOpen_Database_triggered()
{
    QString dbfile = QFileDialog::getOpenFileName(this, tr("Open Project Notes file"), QString(), tr("Project Notes (*.db)"));

    if (!dbfile.isEmpty())
    {
        OpenDatabase(dbfile);
    }

    setButtonAndMenuStates();
}

void MainWindow::OpenDatabase(QString dbfile)
{
    if (!global_DBObjects.OpenDatabase(dbfile))
        return;

    global_DBObjects.SetGlobalSearches(false);
    global_DBObjects.projectslistmodel()->Refresh();
    global_DBObjects.unfilteredpeoplemodel()->Refresh();
    global_DBObjects.unfilteredclientsmodel()->Refresh();
    global_DBObjects.clientsmodel()->Refresh();

    ui->tableViewProjects->setModel(global_DBObjects.projectslistmodelproxy());

    global_Settings.setLastDatabase(dbfile);
}

void MainWindow::on_actionClose_Database_triggered()
{
    global_Settings.setLastDatabase(QString());
    global_DBObjects.CloseDatabase();
    setButtonAndMenuStates();
}

void MainWindow::on_actionClosed_Projects_triggered()
{
    global_DBObjects.SetShowClosedProjects(ui->actionClosed_Projects->isChecked());
    global_DBObjects.SetGlobalSearches(true);
}

void MainWindow::on_actionStatus_Bar_triggered()
{
    statusBar()->setVisible(!statusBar()->isVisible());
}
