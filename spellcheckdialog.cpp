// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include <QCompleter>

#include "pndatabaseobjects.h"
#include "spellcheckdialog.h"
#include "ui_spellcheckdialog.h"
#include "pnsettings.h"
#include "pntextedit.h"
#include "pnplaintextedit.h"
#include <QMessageBox>
#include <QFile>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

SpellCheckDialog::SpellCheckDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowCloseButtonHint),
    ui(new Ui::SpellCheckDialog)
{
    ui->setupUi(this);

    if (!global_Settings.spellchecker()->hasDictionary())
    {
        this->setWindowTitle("Spelling: No Dictionary Configured");
    }
    else
    {
        m_populating = true;
        ui->comboBoxDictionaryLanguage->addItems(global_Settings.spellchecker()->dictionaryNames());
        ui->comboBoxDictionaryLanguage->setCurrentIndex(global_Settings.spellchecker()->defaultDictionaryIndex());

        this->setWindowTitle("Spelling: " + global_Settings.spellchecker()->defaultDictionaryName());
        m_populating = false;
    }

    QString storename = objectName();
    global_Settings.getWindowState(storename, this);
}

void SpellCheckDialog::spellCheck(QWidget* t_focus_control)
{
    m_check_widget = t_focus_control;

    if (!global_Settings.spellchecker()->hasDictionary())
    {
        QMessageBox::critical(this, QObject::tr("Dictionary Files Not Found"),
            QString(tr("No dictionary files were found.  You may need to re-install Project Notes.")), QMessageBox::Close);
        return;
    }

    SpellCheckDialog::SpellCheckAction spellResult = NothingDone;

    // save the position of the current cursor
    QTextCursor oldCursor;

    if (QString(m_check_widget->metaObject()->className()).compare("PNTextEdit") == 0)
        oldCursor = dynamic_cast<PNTextEdit*>(m_check_widget)->textCursor();
    else
        oldCursor = dynamic_cast<PNPlainTextEdit*>(m_check_widget)->textCursor();

    // create a new cursor to walk through the text
    QTextCursor cursor(oldCursor);

    while (!cursor.atEnd())
    {
        QCoreApplication::processEvents();
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);

        QString word = cursor.selectedText();

        // Workaround for better recognition of words
        // punctuation etc. does not belong to words
        while (!word.isEmpty() &&
             !word.at(0).isLetter() &&
             cursor.anchor() < cursor.position())
        {
            int cursorPos = cursor.position();
            cursor.setPosition(cursor.anchor() + 1, QTextCursor::MoveAnchor);
            cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
            word = cursor.selectedText();
        }

        // fix a bug with selecting a contraction
        if (cursor.selectionEnd() + 1 < cursor.document()->characterCount())
            if (
                    cursor.document()->characterAt(cursor.selectionEnd()) == QChar('\'') &&
                    cursor.document()->characterAt(cursor.selectionEnd() + 1) == QChar('t')
                )
            {
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
                //qDebug() << "reselected to word: " << cursor.selectedText();
                word = cursor.selectedText();
            }

        if (!word.isEmpty() && !global_Settings.spellchecker()->isGoodWord(word))
        {
            QTextCursor tmpCursor(cursor);
            tmpCursor.setPosition(cursor.anchor());
            tmpCursor.select(QTextCursor::WordUnderCursor);

            // fix a bug with selecting a contraction
            if (tmpCursor.selectionEnd() + 1 < tmpCursor.document()->characterCount())
                if (
                        tmpCursor.document()->characterAt(tmpCursor.selectionEnd()) == QChar('\'') &&
                        tmpCursor.document()->characterAt(tmpCursor.selectionEnd() + 1) == QChar('t')
                    )
                {
                    tmpCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
                    //qDebug() << "reselected to word: " << cursor.selectedText();
                }

            if (QString(m_check_widget->metaObject()->className()).compare("PNTextEdit") == 0)
            {
                dynamic_cast<QTextEdit*>(m_check_widget)->setTextCursor(tmpCursor);
                dynamic_cast<QTextEdit*>(m_check_widget)->ensureCursorVisible();
            }
            else
            {
                dynamic_cast<QPlainTextEdit*>(m_check_widget)->setTextCursor(tmpCursor);
                dynamic_cast<QPlainTextEdit*>(m_check_widget)->ensureCursorVisible();
            }

            QCoreApplication::processEvents();

            // ask the user what to do
            spellResult = checkWord(word);

            QCoreApplication::processEvents();

            if (spellResult == SpellCheckDialog::AbortCheck)
                break;

            switch (spellResult)
            {
            case SpellCheckDialog::Change:
                cursor.insertText(ui->lineEditChange->text());
                break;
            case SpellCheckDialog::ChangeAll:
                replaceAll(cursor, m_unknown_word, ui->lineEditChange->text());
                if (QString(m_check_widget->metaObject()->className()).compare("PNTextEdit") == 0)
                {
                    dynamic_cast<PNTextEdit*>(m_check_widget)->inlineSpellChecker()->unmarkWord(m_unknown_word);
                }
                else
                {
                    dynamic_cast<PNPlainTextEdit*>(m_check_widget)->inlineSpellChecker()->unmarkWord(m_unknown_word);
                }
                break;

            default:
                break;
            }

            QCoreApplication::processEvents();
        }
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
    }

    if (QString(m_check_widget->metaObject()->className()).compare("PNTextEdit") == 0)
        dynamic_cast<QTextEdit*>(m_check_widget)->setTextCursor(oldCursor);
    else
        dynamic_cast<QPlainTextEdit*>(m_check_widget)->setTextCursor(oldCursor);

    if (spellResult != SpellCheckDialog::AbortCheck)
        QMessageBox::information(
              this,
              tr("Finished"),
              tr("Spell check has finished."));
}

SpellCheckDialog::~SpellCheckDialog()
{
    QString storename = objectName();
    global_Settings.setWindowState(storename, this);

    delete ui;
}

SpellCheckDialog::SpellCheckAction SpellCheckDialog::checkWord(const QString &t_word)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return AbortCheck;  // no dictionary was setup

    m_unknown_word = t_word;

    ui->lineEditChange->setText(m_unknown_word);

    QStringList suggestions = global_Settings.spellchecker()->suggest(t_word);

    ui->listWidgetSuggestions->clear();
    ui->listWidgetSuggestions->addItems(suggestions);

    if (suggestions.count() > 0)
        ui->listWidgetSuggestions->setCurrentRow(0);

    m_return_code = AbortCheck;
    QDialog::exec();

    return m_return_code;
}

void SpellCheckDialog::on_comboBoxDictionaryLanguage_currentIndexChanged(int index)
{
    if (m_populating) return;

    global_Settings.spellchecker()->setDefaultDictionary(index);
    this->setWindowTitle("Spelling: " + global_Settings.spellchecker()->defaultDictionaryName());
}

void SpellCheckDialog::on_lineEditChange_returnPressed()
{
    m_return_code = Change;
    accept();
}

void SpellCheckDialog::on_pushButtonIgnoreOnce_clicked()
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    if (QString(m_check_widget->metaObject()->className()).compare("PNTextEdit") == 0)
    {
        QTextCursor qtc = dynamic_cast<PNTextEdit*>(m_check_widget)->textCursor();
        dynamic_cast<PNTextEdit*>(m_check_widget)->inlineSpellChecker()->unmarkCursor(qtc);
    }
    else
    {
        QTextCursor qtc = dynamic_cast<PNPlainTextEdit*>(m_check_widget)->textCursor();
        dynamic_cast<PNPlainTextEdit*>(m_check_widget)->inlineSpellChecker()->unmarkCursor(qtc);
    }

    m_return_code = IgnoreOnce;
    accept();
}

void SpellCheckDialog::on_pushButtonIgnoreAll_clicked()
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    global_Settings.spellchecker()->ignoreWord(m_unknown_word);

    if (QString(m_check_widget->metaObject()->className()).compare("PNTextEdit") == 0)
    {
        dynamic_cast<PNTextEdit*>(m_check_widget)->inlineSpellChecker()->unmarkWord(m_unknown_word);
    }
    else
    {
        dynamic_cast<PNPlainTextEdit*>(m_check_widget)->inlineSpellChecker()->unmarkWord(m_unknown_word);
    }

    m_return_code = IgnoreAll;
    accept();
}


void SpellCheckDialog::on_pushButtonAddToDictionary_clicked()
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    global_Settings.spellchecker()->ignoreWord(m_unknown_word);
    global_Settings.spellchecker()->AddToPersonalWordList(m_unknown_word);

    if (QString(m_check_widget->metaObject()->className()).compare("PNTextEdit") == 0)
    {
        dynamic_cast<PNTextEdit*>(m_check_widget)->inlineSpellChecker()->unmarkWord(m_unknown_word);
    }
    else
    {
        dynamic_cast<PNPlainTextEdit*>(m_check_widget)->inlineSpellChecker()->unmarkWord(m_unknown_word);
    }

    m_return_code = AddToDict;
    accept();
}


void SpellCheckDialog::on_pushButtonChange_clicked()
{
    if (m_unknown_word == ui->lineEditChange->text() ) // if a new value wasn't typed in used the one selcted
    {
        if ( ui->listWidgetSuggestions->selectedItems().count() > 0 )
            ui->lineEditChange->setText( ui->listWidgetSuggestions->currentItem()->text() );
    }

    m_return_code = Change;
    accept();
}

void SpellCheckDialog::replaceAll(QTextCursor t_cursor, const QString&t_old_word, const QString &t_new_word)
{

    while (!t_cursor.atEnd())
    {
        QCoreApplication::processEvents();
        t_cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);
        QString word = t_cursor.selectedText();

        // Workaround for better recognition of words
        // punctuation etc. does not belong to words
        while (!word.isEmpty() &&
             !word.at(0).isLetter() &&
             t_cursor.anchor() < t_cursor.position())
        {
            int cursorPos = t_cursor.position();
            t_cursor.setPosition(t_cursor.anchor() + 1, QTextCursor::MoveAnchor);
            t_cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
            word = t_cursor.selectedText();
        }

        if (word == t_old_word)
        {
            t_cursor.insertText(t_new_word);
            QCoreApplication::processEvents();
        }

        t_cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
    }
}

void SpellCheckDialog::on_pushButtonChangeAll_clicked()
{
    if (m_unknown_word == ui->lineEditChange->text() ) // if a new value wasn't typed in used the one selcted
    {
        if ( ui->listWidgetSuggestions->selectedItems().count() > 0 )
            ui->lineEditChange->setText( ui->listWidgetSuggestions->currentItem()->text() );
    }

    m_return_code = ChangeAll;
    accept();
}


void SpellCheckDialog::on_pushButtonCancel_clicked()
{
    reject();
}


void SpellCheckDialog::on_listWidgetSuggestions_itemClicked(QListWidgetItem *item)
{
    if ( ui->listWidgetSuggestions->selectedItems().count() > 0 )
        ui->lineEditChange->setText(item->text());
}


void SpellCheckDialog::on_listWidgetSuggestions_itemDoubleClicked(QListWidgetItem *item)
{
    ui->lineEditChange->setText(item->text());

    if (ui->listWidgetSuggestions->selectedItems().count() > 0)
        on_pushButtonChange_clicked();
}

