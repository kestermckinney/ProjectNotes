#include "xmlexportdialog.h"
#include "ui_xmlexportdialog.h"

XMLExportDialog::XMLExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::XMLExportDialog)
{
    ui->setupUi(this);

    ui->comboBoxExportData->addItem("Projects");
    ui->comboBoxExportData->addItem("Clients");
    ui->comboBoxExportData->addItem("Attendees");
    ui->comboBoxExportData->addItem("Team Members");
    ui->comboBoxExportData->addItem("Notes Action Items");
    ui->comboBoxExportData->addItem("People");
    ui->comboBoxExportData->addItem("Project Locations");
    ui->comboBoxExportData->addItem("Project Notes");
    ui->comboBoxExportData->addItem("Project People");
    ui->comboBoxExportData->addItem("Search Results");
    ui->comboBoxExportData->addItem("Status Report Items");
    ui->comboBoxExportData->addItem("Tracker Comments");
    ui->comboBoxExportData->addItem("Project Action Items");
    ui->comboBoxExportData->model()->sort(0);
}

XMLExportDialog::~XMLExportDialog()
{
    delete ui;
}

void XMLExportDialog::setTableDisplayName(const QString& t_displayname)
{
    int i = ui->comboBoxExportData->findText(t_displayname, Qt::MatchExactly);
    ui->comboBoxExportData->setCurrentIndex(i);
}

void XMLExportDialog::setRecordKey(const QString& t_recordkey)
{

}

void XMLExportDialog::on_buttonBox_accepted()
{

}


void XMLExportDialog::on_pushButtonFileSelect_clicked()
{

}

