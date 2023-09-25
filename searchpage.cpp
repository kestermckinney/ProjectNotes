// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

//#include <QDebug>
#include "searchpage.h"
#include "pndatabaseobjects.h"

#include "ui_mainwindow.h"


SearchPage::SearchPage()
{

}

void SearchPage::setPageTitle()
{
    topLevelWidget()->setWindowTitle(QString("Project Notes Search"));
}

void SearchPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    if (!t_ui)
        return;  // closing application

    ui->tableViewSearchResults->setModel(global_DBObjects.searchresultsmodelproxy());
    ui->tableViewSearchResults->selectRow(0);
    setCurrentModel(global_DBObjects.searchresultsmodelproxy());
    setCurrentView( ui->tableViewSearchResults );
}

void SearchPage::setButtonAndMenuStates()
{

}

