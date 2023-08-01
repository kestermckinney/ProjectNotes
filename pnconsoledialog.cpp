#include "pnconsoledialog.h"
#include "ui_pnconsoledialog.h"

#include "pnpluginmanager.h"

#include <QScrollBar>

PNConsoleDialog::PNConsoleDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new Ui::PNConsoleDialog)
{
    ui->setupUi(this);

    QPlainTextEdit* pe = ui->ConsolePlainTextEdit;
    stdout_write_type outwrite;

    outwrite = [pe] (std::string s)
    {
        pe->setPlainText( pe->toPlainText() + QString::fromStdString(s));
        pe->verticalScrollBar()->setValue(pe->verticalScrollBar()->maximum());
    };
    set_stdout(outwrite);

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

