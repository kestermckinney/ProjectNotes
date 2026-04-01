// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "allitemspage.h"
#include "databaseobjects.h"
#include "ui_mainwindow.h"

AllItemsPage::AllItemsPage()
{
}

void AllItemsPage::setupModels(Ui::MainWindow *ui)
{
    this->ui = ui;

    if (!ui)
        return;  // closing application

    ui->tableViewAllItems->setModel(global_DBObjects.allitemsmodelproxy());
    setCurrentModel(global_DBObjects.allitemsmodelproxy());
    setCurrentView(ui->tableViewAllItems);
}

void AllItemsPage::setButtonAndMenuStates()
{
    if (ui)
    {
        ui->actionNew_Item->setEnabled(false);
        ui->actionCopy_Item->setEnabled(false);
        ui->actionDelete_Item->setEnabled(false);
    }
}

void AllItemsPage::setPageTitle()
{
    topLevelWidget()->setWindowTitle(QString("Project Notes - Master Item List"));
    setHistoryText("Master Item List");
}
