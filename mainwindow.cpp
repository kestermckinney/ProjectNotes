#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "pntableview.h"
#include "projectslistmodel.h"
#include "pnsqlquerymodel.h"

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

    m_preferences_dialog = new PreferencesDialog(this);

    // view state
    m_page_history.clear();

    if (!global_Settings.getLastDatabase().toString().isEmpty())
        openDatabase(global_Settings.getLastDatabase().toString());

    setButtonAndMenuStates();

    global_Settings.getWindowState("MainWindow", *this);
    ui->actionStatus_Bar->setChecked(statusBar()->isVisibleTo(this));

    connect(ui->tableViewProjects, SIGNAL(signalOpenRecordWindow()), this, SLOT(on_actionOpen_ProjectDetails_triggered()));
}

MainWindow::~MainWindow()
{
    disconnect(ui->tableViewProjects, SIGNAL(ui->tableViewProjects->signalOpenRecordWindow()), this, SLOT(on_actionOpen_ProjectDetails_triggered()));

    // need to save the screen layout befor the model is removed from the view
    // The destructor of PNTableview does not save the state
    ui->tableViewProjects->setModel(nullptr);
    ui->tableViewClients->setModel(nullptr);
    ui->tableViewPeople->setModel(nullptr);
    ui->tableViewStatusReportItems->setModel(nullptr);
    ui->tableViewTeam->setModel(nullptr);
    ui->tableViewTrackerItems->setModel(nullptr);

    if (global_DBObjects.isOpen())
        global_DBObjects.closeDatabase();

    global_Settings.setWindowState("MainWindow", *this);

    delete m_preferences_dialog;

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

    ui->stackedWidget->setVisible(dbopen);

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


    ui->actionNew_Item->setEnabled(dbopen);
    ui->actionCopy_Item->setEnabled(dbopen);
    ui->actionDelete_Item->setEnabled(dbopen);
    ui->actionEdit_Items->setEnabled(dbopen);
    ui->actionBack->setEnabled(!navigateAtStart());
    ui->actionForward->setEnabled(!navigateAtEnd());
    ui->actionClients->setEnabled(dbopen);
    ui->actionPeople->setEnabled(dbopen);
    ui->actionFilter->setEnabled(dbopen);

    if (dbopen)
    {
        ui->actionClosed_Projects->setChecked(global_DBObjects.getShowClosedProjects());
        ui->actionInternal_Items->setChecked(global_DBObjects.getShowInternalItems());

        if (global_DBObjects.getShowInternalItems())
        {
            ui->tableViewTrackerItems->setColumnHidden(15, false);
            ui->tableViewTrackerItems->resizeColumnToContents(15);
        }
        else
            ui->tableViewTrackerItems->setColumnHidden(15, true);

        ui->actionAll_Tracker_Action_Items->setChecked(global_DBObjects.getShowAllTrackerItems());

        if (global_DBObjects.getShowAllTrackerItems())
        {
            ui->tabWidgetProject->setTabText(2, "Tracker [ALL]");

            ui->tableViewTrackerItems->setColumnHidden(0, true);
            ui->tableViewTrackerItems->setColumnHidden(14, false);
            ui->tableViewTrackerItems->setColumnHidden(17, false);
            ui->tableViewTrackerItems->setColumnHidden(18, false);

            ui->tableViewTrackerItems->resizeColumnToContents(14);
            ui->tableViewTrackerItems->resizeColumnToContents(17);
            ui->tableViewTrackerItems->resizeColumnToContents(18);
        }
        else
        {
            ui->tabWidgetProject->setTabText(2, "Tracker");

            ui->tableViewTrackerItems->setColumnHidden(0, true);
            ui->tableViewTrackerItems->setColumnHidden(14, true);
            ui->tableViewTrackerItems->setColumnHidden(17, true);
            ui->tableViewTrackerItems->setColumnHidden(18, true);
        }
    }
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
    ui->pageProjectDetails->setupModels(ui);

    navigateClearHistory();
    navigateToPage(ui->pageProjectsList);
    //navigateToPage(ui->pageClients);

    setButtonAndMenuStates();
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

void MainWindow::on_actionOpen_ProjectDetails_triggered()
{

    ui->pageProjectDetails->toFirst();

    navigateToPage(ui->pageProjectDetails);
}

void MainWindow::on_actionInternal_Items_triggered()
{
    global_DBObjects.setShowInternalItems(ui->actionInternal_Items->isChecked());
    global_DBObjects.setGlobalSearches(true);

    setButtonAndMenuStates();
}

void MainWindow::on_actionAll_Tracker_Action_Items_triggered()
{
    global_DBObjects.setShowAllTrackerItems(ui->actionAll_Tracker_Action_Items->isChecked());

    // filter tracker items by project
    if (ui->actionAll_Tracker_Action_Items->isChecked())
        global_DBObjects.projectactionitemsmodel()->clearFilter(14);
    else
    {
        QVariant projectid = global_DBObjects.projectinformationmodel()->data(global_DBObjects.projectinformationmodel()->index(0,0));
        global_DBObjects.projectactionitemsmodel()->setFilter(14, projectid.toString());
    }

    global_DBObjects.projectactionitemsmodel()->refresh();

    setButtonAndMenuStates();
}

void MainWindow::on_actionResolved_Tracker_Action_Items_triggered()
{
    global_DBObjects.setShowResolvedTrackerItems(ui->actionResolved_Tracker_Action_Items->isChecked());

    // filter tracker items by Resolved
    if (ui->actionResolved_Tracker_Action_Items->isChecked())
        global_DBObjects.projectactionitemsmodel()->clearFilter(9);
    else
    {
        global_DBObjects.projectactionitemsmodel()->setFilter(9, "Resolved", PNSqlQueryModel::NotEqual );
    }

    global_DBObjects.projectactionitemsmodel()->refresh();

    setButtonAndMenuStates();
}

void MainWindow::on_actionPreferences_triggered()
{
    m_preferences_dialog->show();
}

