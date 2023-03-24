#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "mainwindow.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    QString version = QString("Project Notes Version %1\.%2\.%3").arg(PNMajorVersion).arg(PNMinorVersion).arg(PNFixVersion);
    ui->labelVersion->setText(version);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
