#include "projectlocationdialog.h"
#include "ui_projectlocationdialog.h"

ProjectLocationDialog::ProjectLocationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProjectLocationDialog)
{
    ui->setupUi(this);
}

ProjectLocationDialog::~ProjectLocationDialog()
{
    delete ui;
}
