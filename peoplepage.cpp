#include "peoplepage.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

PeoplePage::PeoplePage()
{
    setTableName("people");
}

void PeoplePage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    if (!t_ui)
        return;  // closing application

    ui->tableViewPeople->setModel(global_DBObjects.peoplemodelproxy());
    ui->tableViewPeople->selectRow(0);

    setCurrentModel(global_DBObjects.peoplemodelproxy());
    setCurrentView( ui->tableViewPeople );
}

void PeoplePage::setButtonAndMenuStates()
{

}
