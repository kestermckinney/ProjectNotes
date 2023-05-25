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

    if (!t_ui)
        return; // closing application

    ui->tableViewClients->setModel(global_DBObjects.clientsmodelproxy());
    ui->tableViewClients->selectRow(0);

    setCurrentModel(global_DBObjects.clientsmodelproxy());
    setCurrentView( ui->tableViewClients );
}

void ClientsPage::setButtonAndMenuStates()
{

}

void ClientsPage::setPageTitle()
{
    topLevelWidget()->setWindowTitle(QString("Project Notes Clients"));
}
