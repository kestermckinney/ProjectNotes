#include <QDebug>
#include "projectslistpage.h"
#include "pndatabaseobjects.h"

#include "ui_mainwindow.h"


ProjectsListPage::ProjectsListPage()
{
    QString page_title = "Projects";
    setPageTitle(page_title);
}

void ProjectsListPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    if (!t_ui)
        return;  // closing application

    ui->tableViewProjects->setModel(global_DBObjects.projectslistmodelproxy());
    ui->tableViewProjects->selectRow(0);
    setCurrentModel(global_DBObjects.projectslistmodelproxy());
    setCurrentView( ui->tableViewProjects );
}
