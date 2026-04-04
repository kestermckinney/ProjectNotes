// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SPELLCHECKDIALOG_H
#define SPELLCHECKDIALOG_H

#include <hunspell/hunspell.hxx>

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
    void hideEvent(QHideEvent *event) override;

public:
    explicit SpellCheckDialog(QWidget *parent = nullptr);
    ~SpellCheckDialog();

    enum SpellCheckAction {AbortCheck, IgnoreOnce, IgnoreAll,
                           Change, ChangeAll, AddToDict, ChangeDict, NothingDone};

    void spellCheck(QWidget* focusControl);
    SpellCheckAction checkWord(const QString &word);
    void replaceAll(QTextCursor cursor, const QString &oldWord, const QString &sNew);

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

    QString m_unknownWord;
    QWidget* m_checkWidget = nullptr;

    //QTextEdit* m_checkWidget = nullptr;
    SpellCheckAction m_returnCode;
    bool m_populating = true;
};

#endif // SPELLCHECKDIALOG_H
