#include "projectdetailspage.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

ProjectDetailsPage::ProjectDetailsPage()
{

}


void ProjectDetailsPage::setupModels( Ui::MainWindow *t_ui )
{
    // STOPPED HERE - how do you bind a form?
    ui = t_ui;

    m_mapperProjectDetails.setModel(global_DBObjects.projectinformationmodelproxy());
    m_mapperProjectDetails.setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

    m_mapperProjectDetails.addMapping(ui->lineEditNumber, 1);
    m_mapperProjectDetails.addMapping(ui->lineEditName, 2);
    m_mapperProjectDetails.addMapping(ui->dateEditLastStatus, 3);
    m_mapperProjectDetails.addMapping(ui->dateEditLastInvoiced, 4);
    m_mapperProjectDetails.addMapping(ui->comboBoxPrimaryContact, 5);
    m_mapperProjectDetails.addMapping(ui->lineEditBudget, 6);
    m_mapperProjectDetails.addMapping(ui->lineEditActual, 7);
    m_mapperProjectDetails.addMapping(ui->lineEditBCWS, 8);
    m_mapperProjectDetails.addMapping(ui->lineEditBCWS, 9);
    m_mapperProjectDetails.addMapping(ui->lineEditBAC, 10);
    m_mapperProjectDetails.addMapping(ui->comboBoxInvoicingPeriod, 11);
    m_mapperProjectDetails.addMapping(ui->comboBoxProjectStatus, 12);
    m_mapperProjectDetails.addMapping(ui->comboBoxClient, 13);
    m_mapperProjectDetails.addMapping(ui->comboBoxStatusReportPeriod, 14);
    m_mapperProjectDetails.toFirst();


    setCurrentModel(global_DBObjects.projectinformationmodelproxy());
    // TODO: Fix Type setCurrentView( ui->tableViewStatusReportItems );
}
