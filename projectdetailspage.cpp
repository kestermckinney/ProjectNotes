#include "projectdetailspage.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

ProjectDetailsPage::ProjectDetailsPage()
{

}

ProjectDetailsPage::~ProjectDetailsPage()
{
    if (m_mapperProjectDetails != nullptr)
        delete m_mapperProjectDetails;

    if (m_project_details_delegate) delete m_project_details_delegate;
}

void ProjectDetailsPage::setupModels( Ui::MainWindow *t_ui )
{

    ui = t_ui;

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
    //m_mapperProjectDetails->addMapping(ui->comboBoxPrimaryContact, 5);
    m_mapperProjectDetails->addMapping(ui->lineEditBudget, 6);
    m_mapperProjectDetails->addMapping(ui->lineEditActual, 7);
    m_mapperProjectDetails->addMapping(ui->lineEditBCWP, 8);
    m_mapperProjectDetails->addMapping(ui->lineEditBCWS, 9);
    m_mapperProjectDetails->addMapping(ui->lineEditBAC, 10);
    m_mapperProjectDetails->addMapping(ui->comboBoxInvoicingPeriod, 11);
    m_mapperProjectDetails->addMapping(ui->comboBoxStatusReportPeriod, 12);
    m_mapperProjectDetails->addMapping(ui->comboBoxClient, 13);
    m_mapperProjectDetails->addMapping(ui->comboBoxProjectStatus, 14);

    // STOPPED HERE trying to setup delegates

    ui->comboBoxInvoicingPeriod->clear();
    ui->comboBoxInvoicingPeriod->addItems(PNDatabaseObjects::invoicing_period);
    ui->comboBoxStatusReportPeriod->clear();
    ui->comboBoxStatusReportPeriod->addItems(PNDatabaseObjects::status_report_period);
    ui->comboBoxProjectStatus->clear();
    ui->comboBoxProjectStatus->addItems(PNDatabaseObjects::project_status);

    //ui->comboBoxPrimaryContact->setModel(global_DBObjects.projectteammembersmodel());
    ui->comboBoxPrimaryContact->setModel(global_DBObjects.unfilteredclientsmodel());
    ui->comboBoxPrimaryContact->setModelColumn(1);
    ui->comboBoxPrimaryContact->setEditable(true);

    //ui->comboBoxClient->setModel(global_DBObjects.unfilteredclientsmodel());
    ui->comboBoxClient->setModel(global_DBObjects.unfilteredclientsmodel());
    ui->comboBoxClient->setModelColumn(1);
    ui->comboBoxClient->setEditable(true);

    setCurrentModel(global_DBObjects.projectinformationmodelproxy());
    // TODO: Fix Type setCurrentView( ui->tableViewStatusReportItems );
}

void ProjectDetailsPage::toFirst()
{
    if (m_mapperProjectDetails != nullptr)
        m_mapperProjectDetails->toFirst();
}
