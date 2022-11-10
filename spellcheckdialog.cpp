#include "spellcheckdialog.h"
#include "ui_spellcheckdialog.h"

SpellCheckDialog::SpellCheckDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SpellCheckDialog)
{
    ui->setupUi(this);
}

SpellCheckDialog::~SpellCheckDialog()
{
    delete ui;
}

void SpellCheckDialog::on_comboBoxDictionaryLanguage_currentIndexChanged(int index)
{

}


void SpellCheckDialog::on_lineEditChange_returnPressed()
{

}


void SpellCheckDialog::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{

}


void SpellCheckDialog::on_pushButtonIgnoreOnce_clicked()
{

}


void SpellCheckDialog::on_pushButtonIgnoreAll_clicked()
{

}


void SpellCheckDialog::on_pushButtonAddToDictionary_clicked()
{

}


void SpellCheckDialog::on_pushButtonChange_clicked()
{

}


void SpellCheckDialog::on_pushButtonChangeAll_clicked()
{

}


void SpellCheckDialog::on_pushButtonCancel_clicked()
{

}

