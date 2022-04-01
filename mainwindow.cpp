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

MainWindow::MainWindow(QWidget *t_parent)
    : QMainWindow(t_parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // view state
    m_current_page = 0;
    m_page_history.clear();
    m_current_model = nullptr;

    if (!global_Settings.getLastDatabase().toString().isEmpty())
        openDatabase(global_Settings.getLastDatabase().toString());

    // connect events
    //connect(ui->pushButtonNewProject, &QPushButton::clicked, this, &MainWindow::handleNewProjectClicked);
    //connect(ui->pushButtonDeleteProject, &QPushButton::clicked, this, &MainWindow::handleDeleteProjectClicked);

    setButtonAndMenuStates();

    global_Settings.getWindowState("MainWindow", *this);
    ui->actionStatus_Bar->setChecked(statusBar()->isVisibleTo(this));

    m_filterdialog = new FilterDataDialog(this);
}

MainWindow::~MainWindow()
{
    // disconnect events
    //disconnect(ui->pushButtonNewProject, &QPushButton::clicked, this, &MainWindow::handleNewProjectClicked);
    //disconnect(ui->pushButtonDeleteProject, &QPushButton::clicked, this, &MainWindow::handleDeleteProjectClicked);

    ui->tableViewProjects->setModel(nullptr);

    if (global_DBObjects.isOpen())
        global_DBObjects.closeDatabase();

    global_Settings.setWindowState("MainWindow", *this);

    delete m_filterdialog;
    delete ui;
}
/*
void MainWindow::handleDeleteProjectClicked()
{
    QModelIndexList qi = ui->t_tableViewProjects->selectionModel()->selectedRows();

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
        ui->actionClosed_Projects->setChecked(global_DBObjects.getShowClosedProjects());

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
        openDatabase(dbfile);
    }

    setButtonAndMenuStates();
}

void MainWindow::openDatabase(QString t_dbfile)
{
    if (!global_DBObjects.openDatabase(t_dbfile))
        return;

    global_DBObjects.unfilteredpeoplemodel()->refresh();
    global_DBObjects.unfilteredclientsmodel()->refresh();

    global_DBObjects.setGlobalSearches(false);

    global_DBObjects.clientsmodel()->refresh();

    global_DBObjects.projectslistmodel()->loadUserFilter(global_DBObjects.projectslistmodel()->objectName());
    global_DBObjects.projectslistmodel()->activateUserFilter(global_DBObjects.projectslistmodel()->objectName());
    //global_DBObjects.projectslistmodel()->refresh();


    ui->tableViewProjects->setModel(global_DBObjects.projectslistmodelproxy());
    ui->tableViewProjects->selectRow(0);
    m_current_model = global_DBObjects.projectslistmodel();
    m_current_view = ui->tableViewProjects;

    global_Settings.setLastDatabase(t_dbfile);
}

void MainWindow::on_actionClose_Database_triggered()
{
    global_Settings.setLastDatabase(QString());
    global_DBObjects.closeDatabase();
    setButtonAndMenuStates();
}

void MainWindow::on_actionClosed_Projects_triggered()
{
    global_DBObjects.setShowClosedProjects(ui->actionClosed_Projects->isChecked());
    global_DBObjects.setGlobalSearches(true);
}

void MainWindow::on_actionStatus_Bar_triggered()
{
    statusBar()->setVisible(!statusBar()->isVisible());
}

void MainWindow::on_actionFilter_triggered()
{
    m_filterdialog->setSourceModelView(m_current_model, m_current_view);
    m_filterdialog->show();
}
