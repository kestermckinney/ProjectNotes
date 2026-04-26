// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "peoplepage.h"
#include "mainwindow.h"
#include "appsettings.h"
#include "ui_mainwindow.h"

PeoplePage::PeoplePage()
{
    setTableName("people");
}

void PeoplePage::openRecord(QVariant& recordId)
{
    setRecordId(recordId);

    if (!recordId.isNull())
    {
        global_DBObjects.peoplemodel()->deactivateUserFilter(global_DBObjects.peoplemodel()->objectName());

        QModelIndex qmi = global_DBObjects.peoplemodel()->findIndex(recordId, 0);
        QModelIndex qi = global_DBObjects.peoplemodelproxy()->index(global_DBObjects.peoplemodelproxy()->mapFromSource(qmi).row(), 1);  // usa a visible column

        ui->tableViewPeople->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewPeople->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
    // else
    // {
    //     loadState();
    // }
}

void PeoplePage::setupModels( Ui::MainWindow *ui )
{
    this->ui = ui;

    if (!ui)
        return;  // closing application

    ui->tableViewPeople->setModel(global_DBObjects.peoplemodelproxy());
    ui->tableViewPeople->selectRow(0);

    setCurrentModel(global_DBObjects.peoplemodelproxy());
    setCurrentView( ui->tableViewPeople );
}

void PeoplePage::setButtonAndMenuStates()
{

}

void PeoplePage::setPageTitle()
{
    topLevelWidget()->setWindowTitle(AppSettings::developerProfilePrefix() + QString("Project Notes People"));

    if (getRecordId().isNull())
    {
        setHistoryText("People");
    }
    else
    {
        QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(ui->tableViewPeople->model());
        SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

        QModelIndexList qil = ui->tableViewPeople->selectionModel()->selectedRows();
        auto qi = qil.begin();
        QModelIndex qq = sortmodel->mapToSource(*qi);
        QModelIndex kqi = currentmodel->index(qq.row(), 1);
        QVariant person = currentmodel->data(kqi);

        setHistoryText(person.toString());
    }
}
