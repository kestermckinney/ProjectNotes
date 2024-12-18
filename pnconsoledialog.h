// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNCONSOLEDIALOG_H
#define PNCONSOLEDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class PNConsoleDialog;
}

class PNConsoleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PNConsoleDialog(QWidget *parent = nullptr);
    ~PNConsoleDialog();

signals:
    void addTextSignal(const QString& t_text);

private slots:
    void on_ClosePushButton_clicked();

    void on_ClearPushButton_clicked();

    void addTextSlot(const QString& t_text);

private:
    Ui::PNConsoleDialog *ui;
};

#endif // PNCONSOLEDIALOG_H
