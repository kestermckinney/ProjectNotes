#include "projectdetailspage.h"
#include "pndatabaseobjects.h"

#include "ui_mainwindow.h"

ProjectDetailsPage::ProjectDetailsPage()
{
    QString page_title = "Project Details";
    setPageTitle(page_title);
}

ProjectDetailsPage::~ProjectDetailsPage()
{
    if (ui)
    {
        disconnect(ui->tabWidgetProject, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidgetProject_currentChanged(int)));
        disconnect(global_DBObjects.projectinformationmodel(), SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(toFirst()));
    }

    if (m_mapperProjectDetails != nullptr)
        delete m_mapperProjectDetails;

    if (m_project_details_delegate)
        delete m_project_details_delegate;
}

void ProjectDetailsPage::newRecord()
{
    QVariant project_id = global_DBObjects.projectinformationmodelproxy()->data(global_DBObjects.projectinformationmodelproxy()->index(0,0));

    int lastrow = ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->rowCount(QModelIndex());

    ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->newRecord(&project_id);

    getCurrentView()->selectRow(lastrow);
    QModelIndex index = getCurrentView()->model()->index(lastrow, 0);
    getCurrentView()->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void ProjectDetailsPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    connect(ui->tabWidgetProject, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidgetProject_currentChanged(int)));
    connect(global_DBObjects.projectinformationmodel(), SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(toFirst()));

    ui->dateEditLastInvoiced->setNullable(true);
    ui->dateEditLastStatus->setNullable(true);

    if (m_mapperProjectDetails == nullptr)
        m_mapperProjectDetails = new QDataWidgetMapper(this);

    if (m_project_details_delegate == nullptr)
        m_project_details_delegate = new ProjectDetailsDelegate(this);

    m_mapperProjectDetails->setItemDelegate(m_project_details_delegate);

    m_mapperProjectDetails->setModel(global_DBObjects.projectinformationmodelproxy());
    m_mapperProjectDetails->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

    m_mapperProjectDetails->addMapping(ui->lineEditNumber, 1);
    m_mapperProjectDetails->addMapping(ui->lineEditProjectName, 2);
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

    ui->comboBoxClient->setModel(global_DBObjects.unfilteredclientsmodel());
    ui->comboBoxClient->setModelColumn(1);
    ui->comboBoxClient->setEditable(true);


    ui->tableViewStatusReportItems->setModel(global_DBObjects.statusreportitemsmodelproxy());

    setCurrentModel(global_DBObjects.statusreportitemsmodelproxy());
    setCurrentView( ui->tableViewStatusReportItems );

    ui->tableViewTeam->setModel(global_DBObjects.projectteammembersmodelproxy());
    ui->tableViewTrackerItems->setModel(global_DBObjects.trackeritemsmodelproxy());   
    ui->tableViewLocations->setModel(global_DBObjects.projectlocationsmodelproxy());
    ui->tableViewProjectNotes->setModel(global_DBObjects.projectnotesmodelproxy());
}

void ProjectDetailsPage::toFirst()
{
    if (m_mapperProjectDetails != nullptr)
        m_mapperProjectDetails->toFirst();

    ui->tabWidgetProject->setCurrentIndex(0);  // always set to the first tab on open
}

void ProjectDetailsPage::on_tabWidgetProject_currentChanged(int index)
{

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

