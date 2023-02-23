#ifndef PNCONSOLEDIALOG_H
#define PNCONSOLEDIALOG_H

#include <QDialog>
#include <QAbstractButton>

#include "pnpluginmanager.h"

namespace Ui {
class PNConsoleDialog;
}

class PNConsoleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PNConsoleDialog(QWidget *parent = nullptr);
    ~PNConsoleDialog();

private slots:
    void on_ClosePushButton_clicked();

    void on_ClearPushButton_clicked();

private:
    Ui::PNConsoleDialog *ui;

    stdout_write_type m_write = nullptr;
};

#endif // PNCONSOLEDIALOG_H
