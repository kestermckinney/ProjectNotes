// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

//#include <QDebug>
#include "projectslistpage.h"
#include "pndatabaseobjects.h"

#include "ui_mainwindow.h"


ProjectsListPage::ProjectsListPage()
{

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

void ProjectsListPage::setButtonAndMenuStates()
{

}

void ProjectsListPage::setPageTitle()
{
    topLevelWidget()->setWindowTitle(QString("Project Notes"));
}
