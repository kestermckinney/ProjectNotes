#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "pntableview.h"

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

    //m_filterdialog = new FilterDataDialog(this);

    // view state
    m_page_history.clear();

    if (!global_Settings.getLastDatabase().toString().isEmpty())
        openDatabase(global_Settings.getLastDatabase().toString());

    setButtonAndMenuStates();

    global_Settings.getWindowState("MainWindow", *this);
    ui->actionStatus_Bar->setChecked(statusBar()->isVisibleTo(this));
}

MainWindow::~MainWindow()
{
    // need to save the screen layout befor the model is removed from the view
    // The destructor of PNTableview does not save the state
    ui->tableViewProjects->setModel(nullptr);
    ui->tableViewClients->setModel(nullptr);
    ui->tableViewPeople->setModel(nullptr);

    if (global_DBObjects.isOpen())
        global_DBObjects.closeDatabase();

    global_Settings.setWindowState("MainWindow", *this);

    //delete m_filterdialog;

    delete ui;
}

/*
void MainWindow::handleDeleteProjectClicked()
{
    navigateCurrentPage()->deleteItem();


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
    ui->actionEdit_Items->setEnabled(dbopen);
    ui->actionBack->setEnabled(!navigateAtStart());
    ui->actionForward->setEnabled(!navigateAtEnd());
    ui->actionClients->setEnabled(dbopen);
    ui->actionPeople->setEnabled(dbopen);
    ui->actionFilter->setEnabled(dbopen);
}

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

    // load and refresh all of the models in order of their dependancy relationships
    global_DBObjects.unfilteredpeoplemodel()->refresh();
    global_DBObjects.unfilteredclientsmodel()->refresh();

    global_DBObjects.setGlobalSearches(false);

    global_DBObjects.clientsmodel()->loadUserFilter(global_DBObjects.clientsmodel()->objectName());
    global_DBObjects.clientsmodel()->activateUserFilter(global_DBObjects.clientsmodel()->objectName());

    global_DBObjects.peoplemodel()->loadUserFilter(global_DBObjects.peoplemodel()->objectName());
    global_DBObjects.peoplemodel()->activateUserFilter(global_DBObjects.peoplemodel()->objectName());

    global_DBObjects.projectslistmodel()->loadUserFilter(global_DBObjects.projectslistmodel()->objectName());
    global_DBObjects.projectslistmodel()->activateUserFilter(global_DBObjects.projectslistmodel()->objectName());

    global_Settings.setLastDatabase(t_dbfile);

    // assign all of the newly open models
    ui->pageProjectsList->setupModels(ui);
    ui->pageClients->setupModels(ui);
    ui->pagePeople->setupModels(ui);

    navigateClearHistory();
    //navigateToPage(ui->pageProjectsList);
    navigateToPage(ui->pageClients);
}

void MainWindow::navigateToPage(PNBasePage* t_widget)
{
    if ( t_widget == navigateCurrentPage() )
        return;

    m_navigation_location = m_navigation_history.count();
    m_navigation_history.push(t_widget);

    ui->stackedWidget->setCurrentWidget(t_widget);

    setButtonAndMenuStates();
}

void MainWindow::navigateBackward()
{
    if (m_navigation_location > 0)
    {
        m_navigation_location--;

        QWidget* current = m_navigation_history.at(m_navigation_location);
        ui->stackedWidget->setCurrentWidget(current);
    }

    setButtonAndMenuStates();
}

void MainWindow::navigateForward()
{
    if (m_navigation_location < (m_navigation_history.count() - 1) )
    {
        m_navigation_location++;

        QWidget* current = m_navigation_history.at(m_navigation_location);
        ui->stackedWidget->setCurrentWidget(current);
    }

    setButtonAndMenuStates();
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
    PNTableView* curview = navigateCurrentPage()->getCurrentView();

    curview->filterDialog();
}

void MainWindow::on_actionClients_triggered()
{
    navigateToPage(ui->pageClients);
}

void MainWindow::on_actionPeople_triggered()
{
    navigateToPage(ui->pagePeople);
}

void MainWindow::on_actionProjects_triggered()
{
    navigateToPage(ui->pageProjectsList);
}

void MainWindow::on_actionBack_triggered()
{
    navigateBackward();
}

void MainWindow::on_actionForward_triggered()
{
    navigateForward();
}

void MainWindow::on_actionNew_Item_triggered()
{
    if ( navigateCurrentPage() )
        navigateCurrentPage()->newRecord();
}

void MainWindow::on_actionCopy_Item_triggered()
{
    if ( navigateCurrentPage() )
        navigateCurrentPage()->copyItem();
}

void MainWindow::on_actionDelete_Item_triggered()
{
    if ( navigateCurrentPage() )
        navigateCurrentPage()->deleteItem();
}
