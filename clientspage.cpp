#include <QDebug>
#include "clientspage.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

ClientsPage::ClientsPage()
{

}

void ClientsPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;
    ui->tableViewClients->setModel(global_DBObjects.clientsmodelproxy());
    ui->tableViewClients->selectRow(0);

    setCurrentModel(global_DBObjects.clientsmodelproxy());
    setCurrentView( ui->tableViewClients );
}
