#include <QDebug>
#include "projectslistpage.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"


ProjectsListPage::ProjectsListPage()
{

}

void ProjectsListPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;
    ui->tableViewProjects->setModel(global_DBObjects.projectslistmodelproxy());
    ui->tableViewProjects->selectRow(0);
    setCurrentModel(global_DBObjects.projectslistmodelproxy());
    setCurrentView( ui->tableViewProjects );
}
