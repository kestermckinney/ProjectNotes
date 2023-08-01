#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "mainwindow.h"

#include <QString>
const QString BUILDV =  QStringLiteral(__DATE__ " " __TIME__);

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint| Qt::WindowCloseButtonHint),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    QString version = QString("Project Notes Version %1\.%2\.%3").arg(PNMajorVersion).arg(PNMinorVersion).arg(PNFixVersion);
    ui->labelVersion->setText(version);
    ui->labelBuild->setText(QString("Build: %1").arg(BUILDV));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
