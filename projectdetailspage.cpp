// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectdetailspage.h"
#include "pndatabaseobjects.h"
#include <QCompleter>

#include "ui_mainwindow.h"

ProjectDetailsPage::ProjectDetailsPage()
{
    setTableName("projects");
}

ProjectDetailsPage::~ProjectDetailsPage()
{
    if (m_mapperProjectDetails)
        delete m_mapperProjectDetails;

    if (m_project_details_delegate)
        delete m_project_details_delegate;
}

void ProjectDetailsPage::openRecord(QVariant& t_record_id)
{
    setRecordId(t_record_id);

    // filter team members by project
    global_DBObjects.projectteammembersmodel()->setFilter(1, t_record_id.toString());
    global_DBObjects.projectteammembersmodel()->refresh();

    // filter project status items
    global_DBObjects.statusreportitemsmodel()->setFilter(1, t_record_id.toString());
    global_DBObjects.statusreportitemsmodel()->refresh();

    // filter team members by project for members list
    global_DBObjects.teamsmodel()->setFilter(2, t_record_id.toString());
    global_DBObjects.teamsmodel()->refresh();

    // filter tracker items by project
    global_DBObjects.trackeritemsmodel()->setFilter(14, t_record_id.toString());
    global_DBObjects.trackeritemsmodel()->refresh();

    // filter tracker items by project
    global_DBObjects.trackeritemsmodel()->setFilter(14, t_record_id.toString());
    global_DBObjects.trackeritemsmodel()->refresh();

    global_DBObjects.trackeritemsmeetingsmodel()->setFilter(1, t_record_id.toString());
    global_DBObjects.trackeritemsmeetingsmodel()->refresh();

    global_DBObjects.projectlocationsmodel()->setFilter(1, t_record_id.toString());
    global_DBObjects.projectlocationsmodel()->refresh();

    global_DBObjects.projectnotesmodel()->setFilter(1, t_record_id.toString());
    global_DBObjects.projectnotesmodel()->refresh();

    // only select the records another event will be fired to open the window to show them
    // order is important this needs to be last
    global_DBObjects.projectinformationmodel()->setFilter(0, t_record_id.toString());
    global_DBObjects.projectinformationmodel()->refresh();

    if (m_mapperProjectDetails != nullptr)
        m_mapperProjectDetails->toFirst();

    loadState();
}

void ProjectDetailsPage::setPageTitle()
{
    QString  page_title = QString("%1 %2").arg(ui->lineEditNumber->text(), ui->plainTextEditProjectName->toPlainText().left(25));
    topLevelWidget()->setWindowTitle(QString("Project Notes Project [%1]").arg(page_title));
    setHistoryText(page_title);
}

void ProjectDetailsPage::newRecord()
{
    QVariant project_id = global_DBObjects.projectinformationmodelproxy()->data(global_DBObjects.projectinformationmodelproxy()->index(0,0));

    QModelIndex index = dynamic_cast<PNSqlQueryModel*>(getCurrentModel()->sourceModel())->newRecord(&project_id);


    //  check if column is visible
    int col = 1;
    while (getCurrentView()->isColumnHidden(col))
        col++;


    QModelIndex sort_index = dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model())->index(
        dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model())->mapFromSource(index).row(), col);

    getCurrentView()->selectionModel()->select(sort_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    getCurrentView()->scrollTo(sort_index, QAbstractItemView::PositionAtCenter);
}

void ProjectDetailsPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    if (t_ui)
    {
        connect(ui->tabWidgetProject, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidgetProject_currentChanged(int)));
    }
    else
    {
        disconnect(ui->tabWidgetProject, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidgetProject_currentChanged(int)));
        return; // closing the appliction
    }

    ui->dateEditLastInvoiced->setNullable(true);
    ui->dateEditLastStatus->setNullable(true);

    if (m_mapperProjectDetails == nullptr)
        m_mapperProjectDetails = new QDataWidgetMapper(this);

    if (m_project_details_delegate == nullptr)
        m_project_details_delegate = new ProjectDetailsDelegate(this);

    m_mapperProjectDetails->setItemDelegate(m_project_details_delegate);

    m_mapperProjectDetails->setModel(global_DBObjects.projectinformationmodelproxy());
    setPageModel(global_DBObjects.projectinformationmodel());
    m_mapperProjectDetails->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

    m_mapperProjectDetails->addMapping(ui->lineEditNumber, 1);
    m_mapperProjectDetails->addMapping(ui->plainTextEditProjectName, 2);
    m_mapperProjectDetails->addMapping(ui->dateEditLastStatus, 3);
    m_mapperProjectDetails->addMapping(ui->dateEditLastInvoiced, 4);
    m_mapperProjectDetails->addMapping(ui->comboBoxPrimaryContact, 5);
    m_mapperProjectDetails->addMapping(ui->lineEditBudget, 6);
    m_mapperProjectDetails->addMapping(ui->lineEditActual, 7);
    m_mapperProjectDetails->addMapping(ui->lineEditBCWP, 8);
    m_mapperProjectDetails->addMapping(ui->lineEditBCWS, 9);
    m_mapperProjectDetails->addMapping(ui->lineEditBAC, 10);
    m_mapperProjectDetails->addMapping(ui->comboBoxInvoicingPeriod, 11);
    m_mapperProjectDetails->addMapping(ui->comboBoxStatusReportPeriod, 12);
    m_mapperProjectDetails->addMapping(ui->comboBoxClient, 13);
    m_mapperProjectDetails->addMapping(ui->comboBoxProjectStatus, 14);

    ui->comboBoxInvoicingPeriod->clear();
    ui->comboBoxInvoicingPeriod->addItems(PNDatabaseObjects::invoicing_period);

    ui->comboBoxStatusReportPeriod->clear();
    ui->comboBoxStatusReportPeriod->addItems(PNDatabaseObjects::status_report_period);

    ui->comboBoxProjectStatus->clear();
    ui->comboBoxProjectStatus->addItems(PNDatabaseObjects::project_status);

    ui->comboBoxPrimaryContact->setModel(global_DBObjects.teamsmodel());
    ui->comboBoxPrimaryContact->setModelColumn(1);
    ui->comboBoxPrimaryContact->setEditable(true);
    ui->comboBoxPrimaryContact->completer()->setCaseSensitivity(Qt::CaseInsensitive);

    ui->comboBoxClient->setModel(global_DBObjects.unfilteredclientsmodel());
    ui->comboBoxClient->setModelColumn(1);
    ui->comboBoxClient->setEditable(true);
    ui->comboBoxClient->completer()->setCaseSensitivity(Qt::CaseInsensitive);


    ui->tableViewStatusReportItems->setModel(global_DBObjects.statusreportitemsmodelproxy());

    setCurrentModel(global_DBObjects.statusreportitemsmodelproxy());
    setCurrentView( ui->tableViewStatusReportItems );

    ui->tableViewTeam->setModel(global_DBObjects.projectteammembersmodelproxy());
    ui->tableViewTeam->setKeyToOpenField(2);

    ui->tableViewTrackerItems->setModel(global_DBObjects.trackeritemsmodelproxy());   
    ui->tableViewLocations->setModel(global_DBObjects.projectlocationsmodelproxy());
    ui->tableViewProjectNotes->setModel(global_DBObjects.projectnotesmodelproxy());
}

void ProjectDetailsPage::on_tabWidgetProject_currentChanged(int index)
{
    PNSqlQueryModel::refreshDirty();

    emit setFocus(); // tell the main window to update to call the setButtonsAndMenus function

    switch ( index )
    {
    case 0:
        setCurrentModel(global_DBObjects.statusreportitemsmodelproxy());
        setCurrentView(ui->tableViewStatusReportItems);
        ui->tableViewStatusReportItems->setFocus();
        break;
    case 1:
        setCurrentModel(global_DBObjects.projectteammembersmodelproxy());
        setCurrentView(ui->tableViewTeam);
        ui->tableViewTeam->setFocus();
        break;
    case 2:
        setCurrentModel(global_DBObjects.trackeritemsmodelproxy());
        setCurrentView(ui->tableViewTrackerItems);
        ui->tableViewTrackerItems->setFocus();
        break;
    case 3:
        setCurrentModel(global_DBObjects.projectlocationsmodelproxy());
        setCurrentView(ui->tableViewLocations);
        ui->tableViewLocations->setFocus();
        break;
    case 4:
        setCurrentModel(global_DBObjects.projectnotesmodelproxy());
        setCurrentView(ui->tableViewProjectNotes);
        ui->tableViewProjectNotes->setFocus();
        break;
    }
}

void ProjectDetailsPage::setButtonAndMenuStates()
{
    bool show_internal = global_DBObjects.getShowInternalItems();

    ui->lineEditBAC->setVisible(show_internal);
    ui->lineEditBCWP->setVisible(show_internal);
    ui->lineEditBCWS->setVisible(show_internal);
    ui->lineEditBudget->setVisible(show_internal);
    ui->lineEditActual->setVisible(show_internal);

    ui->labelBAC->setVisible(show_internal);
    ui->labelBCWP->setVisible(show_internal);
    ui->labelBCWS->setVisible(show_internal);
    ui->labelBudget->setVisible(show_internal);
    ui->labelActual->setVisible(show_internal);
}
