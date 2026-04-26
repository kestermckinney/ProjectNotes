// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectslistpage.h"
#include "databaseobjects.h"
#include "appsettings.h"

#include "ui_mainwindow.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


ProjectsListPage::ProjectsListPage()
{

}

void ProjectsListPage::setupModels( Ui::MainWindow *ui )
{
    this->ui = ui;

    if (!ui)
        return;  // closing application

    ui->tableViewProjects->setModel(global_DBObjects.projectslistmodelproxy());
    setCurrentModel(global_DBObjects.projectslistmodelproxy());
    setCurrentView( ui->tableViewProjects );
}

void ProjectsListPage::setButtonAndMenuStates()
{

}

void ProjectsListPage::setPageTitle()
{
    topLevelWidget()->setWindowTitle(AppSettings::developerProfilePrefix() + QString("Project Notes"));
    setHistoryText("Projects List");
}
