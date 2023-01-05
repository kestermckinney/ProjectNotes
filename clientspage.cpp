#include <QDebug>
#include "clientspage.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

ClientsPage::ClientsPage()
{
    QString page_title = "Clients";
    setPageTitle(page_title);
}

void ClientsPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    if (!t_ui)
        return; // closing application

    ui->tableViewClients->setModel(global_DBObjects.clientsmodelproxy());
    ui->tableViewClients->selectRow(0);

    setCurrentModel(global_DBObjects.clientsmodelproxy());
    setCurrentView( ui->tableViewClients );
}
