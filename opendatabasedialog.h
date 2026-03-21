// Copyright (C) 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef OPENDATABASEDIALOG_H
#define OPENDATABASEDIALOG_H

#include <QDialog>

namespace Ui {
class OpenDatabaseDialog;
}

class OpenDatabaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenDatabaseDialog(QWidget *parent = nullptr);
    ~OpenDatabaseDialog() override;

    bool syncEnabled() const;
    void setSyncEnabled(bool enabled);

    int syncHostType() const;
    void setSyncHostType(int type);

    QString postgrestUrl() const;
    void setPostgrestUrl(const QString& url);

    QString email() const;
    void setEmail(const QString& email);

    QString password() const;
    void setPassword(const QString& password);

    QString encryptionPhrase() const;
    void setEncryptionPhrase(const QString& phrase);

    QString supabaseKey() const;
    void setSupabaseKey(const QString& key);

private slots:
    void on_checkBoxSyncEnabled_toggled(bool checked);
    void on_comboBoxSyncHostType_currentIndexChanged(int index);

private:
    void updateSupabaseKeyVisibility(int hostTypeIndex);

    Ui::OpenDatabaseDialog *ui;
};

#endif // OPENDATABASEDIALOG_H
