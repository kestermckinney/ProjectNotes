// Copyright (C) 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CLOUDSYNCSETTINGSDIALOG_H
#define CLOUDSYNCSETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class CloudSyncSettingsDialog;
}

class CloudSyncSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CloudSyncSettingsDialog(QWidget *parent = nullptr);
    ~CloudSyncSettingsDialog() override;

    bool syncEnabled() const;
    void setSyncEnabled(bool enabled);

    QString email() const;
    void setEmail(const QString& email);

    QString password() const;
    void setPassword(const QString& password);

    QString encryptionPhrase() const;
    void setEncryptionPhrase(const QString& phrase);

    void setSubscriptionStatus(const QString& text);
    void setConnectionInfo(const QString& text);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void on_checkBoxSyncEnabled_toggled(bool checked);

private:
    Ui::CloudSyncSettingsDialog *ui;
};

#endif // CLOUDSYNCSETTINGSDIALOG_H
