// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SPELLCHECKDIALOG_H
#define SPELLCHECKDIALOG_H

#include "hunspell/hunspell.hxx"

#include <QDialog>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>


namespace Ui {
class SpellCheckDialog;
}

class SpellCheckDialog : public QDialog
{
    Q_OBJECT
protected:

public:
    explicit SpellCheckDialog(QWidget *parent = nullptr);
    ~SpellCheckDialog();

    enum SpellCheckAction {AbortCheck, IgnoreOnce, IgnoreAll,
                           Change, ChangeAll, AddToDict, ChangeDict};

    void spellCheck(QWidget* t_focus_control);
    SpellCheckAction checkWord(const QString &t_word);
    void replaceAll(QTextCursor t_cursor, const QString &t_old_word, const QString &sNew);

private slots:
    void on_comboBoxDictionaryLanguage_currentIndexChanged(int index);

    void on_lineEditChange_returnPressed();

    void on_pushButtonIgnoreOnce_clicked();

    void on_pushButtonIgnoreAll_clicked();

    void on_pushButtonAddToDictionary_clicked();

    void on_pushButtonChange_clicked();

    void on_pushButtonChangeAll_clicked();

    void on_pushButtonCancel_clicked();

    void on_listWidgetSuggestions_itemClicked(QListWidgetItem *item);

    void on_listWidgetSuggestions_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::SpellCheckDialog *ui;

    QString m_unknown_word;

    //QTextEdit* m_check_widget = nullptr;
    SpellCheckAction m_return_code;
};

#endif // SPELLCHECKDIALOG_H
