// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pnconsoledialog.h"
#include "ui_pnconsoledialog.h"

#include "pnpluginmanager.h"

#include <QScrollBar>

PNConsoleDialog::PNConsoleDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new Ui::PNConsoleDialog)
{
    ui->setupUi(this);

    stdout_write_type outwrite;

    outwrite = [this] (std::string s)
    {
        emit addTextSignal(QString::fromStdString(s));
    };

    set_stdout(outwrite);

    // use signals and slot to support multi-threaded access to the console
    connect(this, &PNConsoleDialog::addTextSignal, this, &PNConsoleDialog::addTextSlot);
}

void PNConsoleDialog::addTextSlot(const QString& t_text)
{
    QPlainTextEdit* pe = ui->ConsolePlainTextEdit;
    pe->setPlainText( pe->toPlainText() + t_text);
    pe->verticalScrollBar()->setValue(pe->verticalScrollBar()->maximum());

    qDebug() << t_text;
}

PNConsoleDialog::~PNConsoleDialog()
{
    reset_stdout();
    delete ui;
}

void PNConsoleDialog::on_ClosePushButton_clicked()
{
    close();
}


void PNConsoleDialog::on_ClearPushButton_clicked()
{
    ui->ConsolePlainTextEdit->clear();
}

