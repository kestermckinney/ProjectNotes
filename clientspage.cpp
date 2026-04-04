// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "clientspage.h"
#include "mainwindow.h"
#include "plaintextedit.h"
#include "ui_mainwindow.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

ClientsPage::ClientsPage()
{

}

void ClientsPage::openRecord(QVariant& recordId)
{
    setRecordId(recordId);

    if (!recordId.isNull())
    {
        global_DBObjects.clientsmodel()->deactivateUserFilter(global_DBObjects.peoplemodel()->objectName());

        QModelIndex qmi = global_DBObjects.clientsmodel()->findIndex(recordId, 0);
        QModelIndex qi = global_DBObjects.clientsmodelproxy()->index(global_DBObjects.clientsmodelproxy()->mapFromSource(qmi).row(), 1);  // usa a visible column

        ui->tableViewClients->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewClients->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
    // else
    // {
    //     loadState();
    // }
}

void ClientsPage::setupModels( Ui::MainWindow *ui )
{
    this->ui = ui;

    if (!ui)
        return; // closing application

    ui->tableViewClients->setModel(global_DBObjects.clientsmodelproxy());

    setCurrentModel(global_DBObjects.clientsmodelproxy());
    setCurrentView( ui->tableViewClients );
}

void ClientsPage::setButtonAndMenuStates()
{

}

void ClientsPage::setPageTitle()
{
    topLevelWidget()->setWindowTitle(QString("Project Notes Clients"));

    if (getRecordId().isNull())
    {
        setHistoryText("Clients");
    }
    else
    {
        QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(ui->tableViewClients->model());
        SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

        QModelIndexList qil = ui->tableViewClients->selectionModel()->selectedRows();
        auto qi = qil.begin();
        QModelIndex qq = sortmodel->mapToSource(*qi);
        QModelIndex kqi = currentmodel->index(qq.row(), 1);
        QVariant client = currentmodel->data(kqi);

        setHistoryText(client.toString());
    }
}


