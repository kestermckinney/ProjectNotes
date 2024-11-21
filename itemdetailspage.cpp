// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include <QCompleter>

#include "pnplaintextedit.h"
#include "itemdetailspage.h"
#include "pndatabaseobjects.h"
#include "ui_mainwindow.h"

ItemDetailsPage::ItemDetailsPage()
{
    setTableName("item_tracker");
}

ItemDetailsPage::~ItemDetailsPage()
{
    if (m_mapperItemDetails != nullptr)
        delete m_mapperItemDetails;

    if (m_item_details_delegate)
        delete m_item_details_delegate;
}

void ItemDetailsPage::openRecord(QVariant& t_record_id)
{
    setRecordId(t_record_id);

    global_DBObjects.refreshDirty();

    // only select the records another event will be fired to open the window to show them
    global_DBObjects.actionitemsdetailsmodel()->setFilter(0, t_record_id.toString());
    global_DBObjects.actionitemsdetailsmodel()->refresh();

    global_DBObjects.trackeritemscommentsmodel()->setFilter(1, t_record_id.toString());
    global_DBObjects.trackeritemscommentsmodel()->refresh();

    QVariant project_id = global_DBObjects.actionitemsdetailsmodel()->data(
        global_DBObjects.actionitemsdetailsmodel()->index(0, 14));

    global_DBObjects.actionitemsdetailsmeetingsmodel()->setFilter(1, project_id.toString());
    global_DBObjects.actionitemsdetailsmeetingsmodel()->refresh();

    if (m_mapperItemDetails != nullptr)
        m_mapperItemDetails->toFirst();

    loadState();
}

void ItemDetailsPage::setPageTitle()
{
    QString page_title = QString("%1 %2 %3").arg(ui->lineEditNumber->text(), ui->lineEditItemNumber->text(), ui->plainTextEditName->toPlainText().left(25));
    topLevelWidget()->setWindowTitle(QString("Project Notes Item [%1]").arg(page_title));
    setHistoryText(page_title);
}

void ItemDetailsPage::newRecord()
{
    QVariant tracker_id = global_DBObjects.actionitemsdetailsmodelproxy()->data(global_DBObjects.actionitemsdetailsmodelproxy()->index(0,0));

    int lastrow = dynamic_cast<PNSqlQueryModel*>(getCurrentModel()->sourceModel())->rowCount(QModelIndex());

    dynamic_cast<PNSqlQueryModel*>(getCurrentModel()->sourceModel())->newRecord(&tracker_id);

    getCurrentView()->selectRow(lastrow);
    QModelIndex index = getCurrentView()->model()->index(lastrow, 0);
    getCurrentView()->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void ItemDetailsPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    ui->dateEditDueDate->setNullable(true);
    ui->dateEditDateResolved->setNullable(true);
    ui->dateEditLastUpdated->setNullable(true);

    if (m_mapperItemDetails == nullptr)
        m_mapperItemDetails = new QDataWidgetMapper(this);

    if (m_item_details_delegate == nullptr)
        m_item_details_delegate = new ItemDetailsDelegate(this);

    m_mapperItemDetails->setItemDelegate(m_item_details_delegate);

    m_mapperItemDetails->setModel(global_DBObjects.actionitemsdetailsmodelproxy());
    setPageModel(global_DBObjects.actionitemsdetailsmodel());
    m_mapperItemDetails->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

    m_mapperItemDetails->addMapping(ui->lineEditItemNumber, 1);
    m_mapperItemDetails->addMapping(ui->comboBoxType, 2);
    m_mapperItemDetails->addMapping(ui->plainTextEditName, 3);
    m_mapperItemDetails->addMapping(ui->comboBoxProject, 14);
    m_mapperItemDetails->addMapping(ui->comboBoxMeeting, 13);
    m_mapperItemDetails->addMapping(ui->plainTextEditDescription, 6);
    m_mapperItemDetails->addMapping(ui->comboBoxAssignedTo, 7);
    m_mapperItemDetails->addMapping(ui->comboBoxIdentifiedBy, 4);
    m_mapperItemDetails->addMapping(ui->dateEditIdentifiedDate, 5);
    m_mapperItemDetails->addMapping(ui->comboBoxPriority, 8);
    m_mapperItemDetails->addMapping(ui->comboBoxStatus, 9);
    m_mapperItemDetails->addMapping(ui->dateEditDueDate, 10);
    m_mapperItemDetails->addMapping(ui->dateEditDateResolved, 12);
    m_mapperItemDetails->addMapping(ui->dateEditLastUpdated, 11);
    m_mapperItemDetails->addMapping(ui->checkBoxInternal, 15);

    ui->plainTextEditDescription->setAllowEnter(true);


    ui->comboBoxType->clear();
    ui->comboBoxType->addItems(PNDatabaseObjects::item_type);

    ui->comboBoxStatus->clear();
    ui->comboBoxStatus->addItems(PNDatabaseObjects::item_status);

    ui->comboBoxPriority->clear();
    ui->comboBoxPriority->addItems(PNDatabaseObjects::item_priority);

    ui->comboBoxProject->setModel(global_DBObjects.projectslistmodel());
    ui->comboBoxProject->setModelColumn(1);
    ui->comboBoxProject->setEditable(true);
    ui->comboBoxProject->completer()->setCaseSensitivity(Qt::CaseInsensitive);

    ui->comboBoxMeeting->setModel(global_DBObjects.actionitemsdetailsmeetingsmodel());
    ui->comboBoxMeeting->setModelColumn(2);
    ui->comboBoxMeeting->setEditable(true);
    ui->comboBoxMeeting->completer()->setCaseSensitivity(Qt::CaseInsensitive);

    ui->comboBoxAssignedTo->setModel(global_DBObjects.teamsmodel());
    ui->comboBoxAssignedTo->setModelColumn(1);
    ui->comboBoxAssignedTo->setEditable(true);
    ui->comboBoxAssignedTo->completer()->setCaseSensitivity(Qt::CaseInsensitive);

    ui->comboBoxIdentifiedBy->setModel(global_DBObjects.teamsmodel());
    ui->comboBoxIdentifiedBy->setModelColumn(1);
    ui->comboBoxIdentifiedBy->setEditable(true);
    ui->comboBoxIdentifiedBy->completer()->setCaseSensitivity(Qt::CaseInsensitive);


    ui->tableViewComments->setModel(global_DBObjects.trackeritemscommentsmodelproxy());
    ui->tableViewComments->setWordWrap(true);
    ui->tableViewComments->resizeRowsToContents();

    setCurrentModel(global_DBObjects.trackeritemscommentsmodelproxy());
    setCurrentView( ui->tableViewComments );

    ui->tableViewComments->verticalHeader()->setDefaultSectionSize( 15 * 4 );
}

void ItemDetailsPage::setButtonAndMenuStates()
{

}
