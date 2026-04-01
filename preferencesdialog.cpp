// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include <QCompleter>
#include "databaseobjects.h"
#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "appsettings.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint ),
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
    global_Settings.getWindowState(objectName(), this);
    if (global_DBObjects.isOpen())
    {
        ui->comboBoxManagerCompany->setModel(global_DBObjects.unfilteredclientsmodel());
        ui->comboBoxManagerCompany->setModelColumn(1);
        ui->comboBoxManagerCompany->completer()->setCaseSensitivity(Qt::CaseInsensitive);
        ui->comboBoxProjectManager->setModel(global_DBObjects.unfilteredpeoplemodel());
        ui->comboBoxProjectManager->setModelColumn(1);
        ui->comboBoxProjectManager->completer()->setCaseSensitivity(Qt::CaseInsensitive);

        SqlQueryModel *client_model = static_cast<SqlQueryModel*>(ui->comboBoxManagerCompany->model());
        client_model->refresh();

        QVariant client_id = global_DBObjects.getManagingCompany();
        QString client_name = client_model->findValue(client_id, 0, 1).toString();
        int client_idx = ui->comboBoxManagerCompany->findText(client_name);
        ui->comboBoxManagerCompany->setCurrentIndex(client_idx >= 0 ? client_idx : 0);

        SqlQueryModel *people_model = static_cast<SqlQueryModel*>(ui->comboBoxProjectManager->model());
        people_model->refresh();

        QVariant people_id = global_DBObjects.getProjectManager();
        QString people_name = people_model->findValue(people_id, 0, 1).toString();
        int people_idx = ui->comboBoxProjectManager->findText(people_name);
        ui->comboBoxProjectManager->setCurrentIndex(people_idx >= 0 ? people_idx : 0);
    }

    QDialog::showEvent(ev);
}

void PreferencesDialog::hideEvent(QHideEvent *event)
{
    global_Settings.setWindowState(objectName(), this);
    QDialog::hideEvent(event);
}
