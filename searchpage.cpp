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
    setHistoryText("Search");
}

void SearchPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    if (!t_ui)
    {
        ui->plainTextEditSearchText->removeEventFilter(this);
        return;  // closing application
    }

    ui->plainTextEditSearchText->installEventFilter(this);
    ui->tableViewSearchResults->setModel(global_DBObjects.searchresultsmodelproxy());
    ui->tableViewSearchResults->selectRow(0);
    setCurrentModel(global_DBObjects.searchresultsmodelproxy());
    setCurrentView( ui->tableViewSearchResults );
}

void SearchPage::setButtonAndMenuStates()
{

}


bool SearchPage::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    if(event->type() == QKeyEvent::KeyPress)
    {
        QKeyEvent * ke = static_cast<QKeyEvent*>(event);
        if(ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
        {
            global_DBObjects.searchresultsmodel()->PerformSearch(ui->plainTextEditSearchText->toPlainText());
            return true; // do not process this event further
        }
    }

    return false;
}
