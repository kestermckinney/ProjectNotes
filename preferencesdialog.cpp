#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "pndatabaseobjects.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::on_buttonBox_accepted()
{
    int i = ui->comboBoxManagerCompany->currentIndex();
    QVariant key_val = ui->comboBoxManagerCompany->model()->data(ui->comboBoxManagerCompany->model()->index(i, 0));

    global_DBObjects.setManagingCompany(key_val.toString());

    i = ui->comboBoxProjectManager->currentIndex();
    key_val = ui->comboBoxProjectManager->model()->data(ui->comboBoxProjectManager->model()->index(i, 0));

    global_DBObjects.setProjectManager(key_val.toString());
}


void PreferencesDialog::showEvent(QShowEvent *ev)
{
    if (global_DBObjects.isOpen())
    {
        ui->comboBoxManagerCompany->setModel(global_DBObjects.unfilteredclientsmodel());
        ui->comboBoxManagerCompany->setModelColumn(1);
        ui->comboBoxProjectManager->setModel(global_DBObjects.unfilteredpeoplemodel());
        ui->comboBoxProjectManager->setModelColumn(1);

        PNSqlQueryModel *client_model = static_cast<PNSqlQueryModel*>(ui->comboBoxManagerCompany->model());
        client_model->refresh();

        QVariant client_id = global_DBObjects.getManagingCompany();
        QString client_name = client_model->findValue(client_id, 0, 1).toString();
        ui->comboBoxManagerCompany->setCurrentText(client_name);

        PNSqlQueryModel *people_model = static_cast<PNSqlQueryModel*>(ui->comboBoxProjectManager->model());
        people_model->refresh();

        QVariant people_id = global_DBObjects.getProjectManager();
        QString people_name = people_model->findValue(people_id, 0, 1).toString();
        ui->comboBoxProjectManager->setCurrentText(people_name);
    }

    QDialog::showEvent(ev);
}
