#include "peoplepage.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

PeoplePage::PeoplePage()
{

}

void PeoplePage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;
    ui->tableViewProjects->setModel(global_DBObjects.projectslistmodelproxy());
    ui->tableViewProjects->selectRow(0);
    //m_current_model = global_DBObjects.projectslistmodel();
    //m_current_view = ui->tableViewProjects;

}


void PeoplePage::newRecord()
{

}

void PeoplePage::copyItem()
{

}

void PeoplePage::deleteItem()
{

}
