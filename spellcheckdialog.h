#ifndef SPELLCHECKDIALOG_H
#define SPELLCHECKDIALOG_H

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
class SpellCheckDialog;
}

class SpellCheckDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpellCheckDialog(QWidget *parent = nullptr);
    ~SpellCheckDialog();

private slots:
    void on_comboBoxDictionaryLanguage_currentIndexChanged(int index);

    void on_lineEditChange_returnPressed();

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_pushButtonIgnoreOnce_clicked();

    void on_pushButtonIgnoreAll_clicked();

    void on_pushButtonAddToDictionary_clicked();

    void on_pushButtonChange_clicked();

    void on_pushButtonChangeAll_clicked();

    void on_pushButtonCancel_clicked();

private:
    Ui::SpellCheckDialog *ui;
};

#endif // SPELLCHECKDIALOG_H
