// Copyright (C) 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "cloudsyncsettingsdialog.h"
#include "ui_cloudsyncsettingsdialog.h"
#include "appsettings.h"

CloudSyncSettingsDialog::CloudSyncSettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CloudSyncSettingsDialog)
{
    ui->setupUi(this);
}

CloudSyncSettingsDialog::~CloudSyncSettingsDialog()
{
    delete ui;
}

bool CloudSyncSettingsDialog::syncEnabled() const
{
    return ui->checkBoxSyncEnabled->isChecked();
}

void CloudSyncSettingsDialog::setSyncEnabled(bool enabled)
{
    ui->checkBoxSyncEnabled->setChecked(enabled);
}

QString CloudSyncSettingsDialog::email() const
{
    return ui->lineEditUsername->text();
}

void CloudSyncSettingsDialog::setEmail(const QString& email)
{
    ui->lineEditUsername->setText(email);
}

QString CloudSyncSettingsDialog::password() const
{
    return ui->lineEditPassword->text();
}

void CloudSyncSettingsDialog::setPassword(const QString& password)
{
    ui->lineEditPassword->setText(password);
}

QString CloudSyncSettingsDialog::encryptionPhrase() const
{
    return ui->lineEditEncryptionPhrase->text();
}

void CloudSyncSettingsDialog::setEncryptionPhrase(const QString& phrase)
{
    ui->lineEditEncryptionPhrase->setText(phrase);
}

void CloudSyncSettingsDialog::setSubscriptionStatus(const QString& text)
{
    ui->labelSubscriptionStatus->setText(text);
}

void CloudSyncSettingsDialog::setConnectionInfo(const QString& text)
{
    ui->labelConnectionInfo->setText(text);
}

void CloudSyncSettingsDialog::on_checkBoxSyncEnabled_toggled(bool checked)
{
    ui->groupBoxSync->setEnabled(checked);
}

void CloudSyncSettingsDialog::showEvent(QShowEvent *event)
{
    global_Settings.getWindowState(objectName(), this);
    QDialog::showEvent(event);
}

void CloudSyncSettingsDialog::hideEvent(QHideEvent *event)
{
    global_Settings.setWindowState(objectName(), this);
    QDialog::hideEvent(event);
}
