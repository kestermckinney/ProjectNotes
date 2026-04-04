// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include <QCompleter>

#include "databaseobjects.h"
#include "spellcheckdialog.h"
#include "ui_spellcheckdialog.h"
#include "appsettings.h"
#include "textedit.h"
#include "plaintextedit.h"
#include <QMessageBox>
#include <QFile>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

SpellCheckDialog::SpellCheckDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
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

        this->setWindowTitle(QString("Spelling: %1").arg(global_Settings.spellchecker()->defaultDictionaryName()));
        m_populating = false;
    }
}

void SpellCheckDialog::spellCheck(QWidget* focusControl)
{
    m_checkWidget = focusControl;

    if (!global_Settings.spellchecker()->hasDictionary())
    {
        QMessageBox::critical(this, QObject::tr("Dictionary Files Not Found"),
            QString(tr("No dictionary files were found.  You may need to re-install Project Notes.")), QMessageBox::Close);
        return;
    }

    SpellCheckDialog::SpellCheckAction spellResult = NothingDone;

    const bool isTextEdit = (QLatin1StringView(m_checkWidget->metaObject()->className()) == "TextEdit");

    // save the position of the current cursor
    QTextCursor oldCursor;

    if (isTextEdit)
        oldCursor = dynamic_cast<TextEdit*>(m_checkWidget)->textCursor();
    else
        oldCursor = dynamic_cast<PlainTextEdit*>(m_checkWidget)->textCursor();

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

            if (isTextEdit)
            {
                dynamic_cast<QTextEdit*>(m_checkWidget)->setTextCursor(tmpCursor);
                dynamic_cast<QTextEdit*>(m_checkWidget)->ensureCursorVisible();
            }
            else
            {
                dynamic_cast<QPlainTextEdit*>(m_checkWidget)->setTextCursor(tmpCursor);
                dynamic_cast<QPlainTextEdit*>(m_checkWidget)->ensureCursorVisible();
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
                replaceAll(cursor, m_unknownWord, ui->lineEditChange->text());
                if (isTextEdit)
                {
                    dynamic_cast<TextEdit*>(m_checkWidget)->inlineSpellChecker()->unmarkWord(m_unknownWord);
                }
                else
                {
                    dynamic_cast<PlainTextEdit*>(m_checkWidget)->inlineSpellChecker()->unmarkWord(m_unknownWord);
                }
                break;

            default:
                break;
            }

            QCoreApplication::processEvents();
        }
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
    }

    if (isTextEdit)
        dynamic_cast<QTextEdit*>(m_checkWidget)->setTextCursor(oldCursor);
    else
        dynamic_cast<QPlainTextEdit*>(m_checkWidget)->setTextCursor(oldCursor);

    if (spellResult != SpellCheckDialog::AbortCheck)
        QMessageBox::information(
              this,
              tr("Finished"),
              tr("Spell check has finished."));
}

SpellCheckDialog::~SpellCheckDialog()
{
    delete ui;
}

SpellCheckDialog::SpellCheckAction SpellCheckDialog::checkWord(const QString &word)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return AbortCheck;  // no dictionary was setup

    m_unknownWord = word;

    ui->lineEditChange->setText(m_unknownWord);

    QStringList suggestions = global_Settings.spellchecker()->suggest(word);

    ui->listWidgetSuggestions->clear();
    ui->listWidgetSuggestions->addItems(suggestions);

    if (suggestions.count() > 0)
        ui->listWidgetSuggestions->setCurrentRow(0);

    m_returnCode = AbortCheck;
    QDialog::exec();

    return m_returnCode;
}

void SpellCheckDialog::on_comboBoxDictionaryLanguage_currentIndexChanged(int index)
{
    if (m_populating) return;

    global_Settings.spellchecker()->setDefaultDictionary(index);
    this->setWindowTitle(QString("Spelling: %1").arg(global_Settings.spellchecker()->defaultDictionaryName()));
}

void SpellCheckDialog::on_lineEditChange_returnPressed()
{
    m_returnCode = Change;
    accept();
}

void SpellCheckDialog::on_pushButtonIgnoreOnce_clicked()
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    if (QLatin1StringView(m_checkWidget->metaObject()->className()) == "TextEdit")
    {
        QTextCursor qtc = dynamic_cast<TextEdit*>(m_checkWidget)->textCursor();
        dynamic_cast<TextEdit*>(m_checkWidget)->inlineSpellChecker()->unmarkCursor(qtc);
    }
    else
    {
        QTextCursor qtc = dynamic_cast<PlainTextEdit*>(m_checkWidget)->textCursor();
        dynamic_cast<PlainTextEdit*>(m_checkWidget)->inlineSpellChecker()->unmarkCursor(qtc);
    }

    m_returnCode = IgnoreOnce;
    accept();
}

void SpellCheckDialog::on_pushButtonIgnoreAll_clicked()
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    global_Settings.spellchecker()->ignoreWord(m_unknownWord);

    if (QLatin1StringView(m_checkWidget->metaObject()->className()) == "TextEdit")
    {
        dynamic_cast<TextEdit*>(m_checkWidget)->inlineSpellChecker()->unmarkWord(m_unknownWord);
    }
    else
    {
        dynamic_cast<PlainTextEdit*>(m_checkWidget)->inlineSpellChecker()->unmarkWord(m_unknownWord);
    }

    m_returnCode = IgnoreAll;
    accept();
}


void SpellCheckDialog::on_pushButtonAddToDictionary_clicked()
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    global_Settings.spellchecker()->ignoreWord(m_unknownWord);
    global_Settings.spellchecker()->AddToPersonalWordList(m_unknownWord);

    if (QLatin1StringView(m_checkWidget->metaObject()->className()) == "TextEdit")
    {
        dynamic_cast<TextEdit*>(m_checkWidget)->inlineSpellChecker()->unmarkWord(m_unknownWord);
    }
    else
    {
        dynamic_cast<PlainTextEdit*>(m_checkWidget)->inlineSpellChecker()->unmarkWord(m_unknownWord);
    }

    m_returnCode = AddToDict;
    accept();
}


void SpellCheckDialog::on_pushButtonChange_clicked()
{
    if (m_unknownWord == ui->lineEditChange->text() ) // if a new value wasn't typed in used the one selcted
    {
        if ( ui->listWidgetSuggestions->selectedItems().count() > 0 )
            ui->lineEditChange->setText( ui->listWidgetSuggestions->currentItem()->text() );
    }

    m_returnCode = Change;
    accept();
}

void SpellCheckDialog::replaceAll(QTextCursor cursor, const QString&oldWord, const QString &newWord)
{

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

        if (word == oldWord)
        {
            cursor.insertText(newWord);
            QCoreApplication::processEvents();
        }

        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
    }
}

void SpellCheckDialog::on_pushButtonChangeAll_clicked()
{
    if (m_unknownWord == ui->lineEditChange->text() ) // if a new value wasn't typed in used the one selcted
    {
        if ( ui->listWidgetSuggestions->selectedItems().count() > 0 )
            ui->lineEditChange->setText( ui->listWidgetSuggestions->currentItem()->text() );
    }

    m_returnCode = ChangeAll;
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

void SpellCheckDialog::hideEvent(QHideEvent *event)
{
    global_Settings.setWindowState(objectName(), this);
    QDialog::hideEvent(event);
}

