// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "clientspage.h"
#include "mainwindow.h"
#include "pnplaintextedit.h"
#include "ui_mainwindow.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

ClientsPage::ClientsPage()
{

}

void ClientsPage::openRecord(QVariant& t_record_id)
{
    setRecordId(t_record_id);

    if (!t_record_id.isNull())
    {
        global_DBObjects.clientsmodel()->deactivateUserFilter(global_DBObjects.peoplemodel()->objectName());

        global_DBObjects.refreshDirty();

        QModelIndex qmi = global_DBObjects.clientsmodel()->findIndex(t_record_id, 0);
        QModelIndex qi = global_DBObjects.clientsmodelproxy()->index(global_DBObjects.clientsmodelproxy()->mapFromSource(qmi).row(), 1);  // usa a visible column

        ui->tableViewClients->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewClients->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
    else
    {
        global_DBObjects.refreshDirty();
        loadState();
    }


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

    if (getRecordId().isNull())
    {
        setHistoryText("Clients");
    }
    else
    {
        QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(ui->tableViewClients->model());
        PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

        QModelIndexList qil = ui->tableViewClients->selectionModel()->selectedRows();
        auto qi = qil.begin();
        QModelIndex qq = sortmodel->mapToSource(*qi);
        QModelIndex kqi = currentmodel->index(qq.row(), 1);
        QVariant client = currentmodel->data(kqi);

        setHistoryText(client.toString());
    }
}


