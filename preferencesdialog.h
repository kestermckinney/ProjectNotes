#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    ~PreferencesDialog();

protected:
      void showEvent(QShowEvent *ev);

private slots:
    void on_buttonBox_accepted();

private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
