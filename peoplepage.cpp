#include "peoplepage.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

PeoplePage::PeoplePage()
{
    QString page_title = "People";
    setPageTitle(page_title);
}

void PeoplePage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;
    ui->tableViewPeople->setModel(global_DBObjects.peoplemodelproxy());
    ui->tableViewPeople->selectRow(0);

    setCurrentModel(global_DBObjects.peoplemodelproxy());
    setCurrentView( ui->tableViewPeople );
}
