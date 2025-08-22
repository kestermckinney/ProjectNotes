// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "peoplepage.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

PeoplePage::PeoplePage()
{
    setTableName("people");
}

void PeoplePage::openRecord(QVariant& t_record_id)
{
    setRecordId(t_record_id);

    if (!t_record_id.isNull())
    {
        global_DBObjects.peoplemodel()->deactivateUserFilter(global_DBObjects.peoplemodel()->objectName());

        global_DBObjects.refreshDirty();

        QModelIndex qmi = global_DBObjects.peoplemodel()->findIndex(t_record_id, 0);
        QModelIndex qi = global_DBObjects.peoplemodelproxy()->index(global_DBObjects.peoplemodelproxy()->mapFromSource(qmi).row(), 1);  // usa a visible column

        ui->tableViewPeople->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewPeople->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
    else
    {
        global_DBObjects.refreshDirty();
        loadState();
    }
}

void PeoplePage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    if (!t_ui)
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
    topLevelWidget()->setWindowTitle(QString("Project Notes 4 Beta People"));

    if (getRecordId().isNull())
    {
        setHistoryText("People");
    }
    else
    {
        QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(ui->tableViewPeople->model());
        PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

        QModelIndexList qil = ui->tableViewPeople->selectionModel()->selectedRows();
        auto qi = qil.begin();
        QModelIndex qq = sortmodel->mapToSource(*qi);
        QModelIndex kqi = currentmodel->index(qq.row(), 1);
        QVariant person = currentmodel->data(kqi);

        setHistoryText(person.toString());
    }
}
