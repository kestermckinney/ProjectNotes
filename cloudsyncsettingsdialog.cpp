// Copyright (C) 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "cloudsyncsettingsdialog.h"
#include "ui_cloudsyncsettingsdialog.h"
#include "appsettings.h"

CloudSyncSettingsDialog::CloudSyncSettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CloudSyncSettingsDialog)
{
    ui->setupUi(this);
    // Apply initial visibility based on default host type (index 0 = Self-Hosted)
    updateSupabaseKeyVisibility(ui->comboBoxSyncHostType->currentIndex());
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

int CloudSyncSettingsDialog::syncHostType() const
{
    return ui->comboBoxSyncHostType->currentIndex();
}

void CloudSyncSettingsDialog::setSyncHostType(int type)
{
    ui->comboBoxSyncHostType->setCurrentIndex(type);
}

QString CloudSyncSettingsDialog::postgrestUrl() const
{
    return ui->lineEditPostgrestURL->text();
}

void CloudSyncSettingsDialog::setPostgrestUrl(const QString& url)
{
    ui->lineEditPostgrestURL->setText(url);
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

QString CloudSyncSettingsDialog::supabaseKey() const
{
    return ui->lineEditSupabaseKey->text();
}

void CloudSyncSettingsDialog::setSupabaseKey(const QString& key)
{
    ui->lineEditSupabaseKey->setText(key);
}

void CloudSyncSettingsDialog::on_checkBoxSyncEnabled_toggled(bool checked)
{
    ui->groupBoxSync->setEnabled(checked);
}

void CloudSyncSettingsDialog::on_comboBoxSyncHostType_currentIndexChanged(int index)
{
    updateSupabaseKeyVisibility(index);
}

void CloudSyncSettingsDialog::updateSupabaseKeyVisibility(int hostTypeIndex)
{
    // index 1 = Supabase; all others (0 = Self-Hosted) don't need the anon key
    const bool isSupabase = (hostTypeIndex == 1);
    ui->labelSupabaseKey->setVisible(isSupabase);
    ui->lineEditSupabaseKey->setVisible(isSupabase);
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
