#ifndef PROJECTLOCATIONDIALOG_H
#define PROJECTLOCATIONDIALOG_H

#include <QDialog>

namespace Ui {
class ProjectLocationDialog;
}

class ProjectLocationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectLocationDialog(QWidget *parent = nullptr);
    ~ProjectLocationDialog();

private:
    Ui::ProjectLocationDialog *ui;
};

#endif // PROJECTLOCATIONDIALOG_H
