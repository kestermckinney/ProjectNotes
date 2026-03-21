// Copyright (C) 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "opendatabasedialog.h"
#include "ui_opendatabasedialog.h"

OpenDatabaseDialog::OpenDatabaseDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OpenDatabaseDialog)
{
    ui->setupUi(this);
    // Apply initial visibility based on default host type (index 0 = Self-Hosted)
    updateSupabaseKeyVisibility(ui->comboBoxSyncHostType->currentIndex());
}

OpenDatabaseDialog::~OpenDatabaseDialog()
{
    delete ui;
}

bool OpenDatabaseDialog::syncEnabled() const
{
    return ui->checkBoxSyncEnabled->isChecked();
}

void OpenDatabaseDialog::setSyncEnabled(bool enabled)
{
    ui->checkBoxSyncEnabled->setChecked(enabled);
}

int OpenDatabaseDialog::syncHostType() const
{
    return ui->comboBoxSyncHostType->currentIndex();
}

void OpenDatabaseDialog::setSyncHostType(int type)
{
    ui->comboBoxSyncHostType->setCurrentIndex(type);
}

QString OpenDatabaseDialog::postgrestUrl() const
{
    return ui->lineEditPostgrestURL->text();
}

void OpenDatabaseDialog::setPostgrestUrl(const QString& url)
{
    ui->lineEditPostgrestURL->setText(url);
}

QString OpenDatabaseDialog::email() const
{
    return ui->lineEditUsername->text();
}

void OpenDatabaseDialog::setEmail(const QString& email)
{
    ui->lineEditUsername->setText(email);
}

QString OpenDatabaseDialog::password() const
{
    return ui->lineEditPassword->text();
}

void OpenDatabaseDialog::setPassword(const QString& password)
{
    ui->lineEditPassword->setText(password);
}

QString OpenDatabaseDialog::encryptionPhrase() const
{
    return ui->lineEditEncryptionPhrase->text();
}

void OpenDatabaseDialog::setEncryptionPhrase(const QString& phrase)
{
    ui->lineEditEncryptionPhrase->setText(phrase);
}

QString OpenDatabaseDialog::supabaseKey() const
{
    return ui->lineEditSupabaseKey->text();
}

void OpenDatabaseDialog::setSupabaseKey(const QString& key)
{
    ui->lineEditSupabaseKey->setText(key);
}

void OpenDatabaseDialog::on_checkBoxSyncEnabled_toggled(bool checked)
{
    ui->groupBoxSync->setEnabled(checked);
}

void OpenDatabaseDialog::on_comboBoxSyncHostType_currentIndexChanged(int index)
{
    updateSupabaseKeyVisibility(index);
}

void OpenDatabaseDialog::updateSupabaseKeyVisibility(int hostTypeIndex)
{
    // index 1 = Supabase; all others (0 = Self-Hosted) don't need the anon key
    const bool isSupabase = (hostTypeIndex == 1);
    ui->labelSupabaseKey->setVisible(isSupabase);
    ui->lineEditSupabaseKey->setVisible(isSupabase);
}
