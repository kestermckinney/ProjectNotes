#include "pnconsoledialog.h"
#include "ui_pnconsoledialog.h"

PNConsoleDialog::PNConsoleDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PNConsoleDialog)
{
    ui->setupUi(this);

    QPlainTextEdit* pe = ui->ConsolePlainTextEdit;

    m_write = [pe] (std::string s) { pe->setPlainText( pe->toPlainText() + QString::fromStdString(s)); };
    set_stdout(m_write);

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

