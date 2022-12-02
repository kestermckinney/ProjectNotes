#include <QDebug>
#include "searchpage.h"
#include "pndatabaseobjects.h"

#include "ui_mainwindow.h"


SearchPage::SearchPage()
{
    QString page_title = "Database Search";
    setPageTitle(page_title);
}

void SearchPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;
    ui->tableViewSearchResults->setModel(global_DBObjects.searchresultsmodelproxy());
    ui->tableViewSearchResults->selectRow(0);
    setCurrentModel(global_DBObjects.searchresultsmodelproxy());
    setCurrentView( ui->tableViewSearchResults );
}
