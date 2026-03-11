// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "findreplacedialog.h"
#include "ui_findreplacedialog.h"
#include "plaintextedit.h"
#include "textedit.h"
#include "appsettings.h"

#include <QRegularExpression>
#include <QMessageBox>

FindReplaceDialog::FindReplaceDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new Ui::FindReplaceDialog)
{
    ui->setupUi(this);

    QString storename = objectName();
    global_Settings.getWindowState(storename, this);
}

FindReplaceDialog::~FindReplaceDialog()
{
    QString storename = objectName();
    global_Settings.setWindowState(storename, this);

    delete ui;
}

qsizetype FindReplaceDialog::getCRLFCount(QString& searchstring, qsizetype start, qsizetype end)
{
    qsizetype count = 0;
    qsizetype currentstart = start;

    while ( currentstart < end )
    {
        if ( searchstring.at(currentstart) == '\r' && searchstring.at(currentstart + 1) == '\n' )
            count++;

        currentstart++;
    }

    return count;
}

bool FindReplaceDialog::doFind(bool quiet)
{
    if (QString(m_findWidget->metaObject()->className()).compare("QLineEdit") == 0)
    {
        QString findstring = ui->lineEditFind->text();

        QString searching;

        searching = dynamic_cast<QLineEdit*>(m_findWidget)->text();

        qsizetype findloc;

        if (ui->checkBoxUseRegEx->isChecked())
        {
            if (ui->radioButtonDown->isChecked())
                findloc = searching.indexOf(QRegularExpression(findstring, (ui->checkBoxMatchCase->isChecked() ? QRegularExpression::NoPatternOption  : QRegularExpression::CaseInsensitiveOption)), m_endCurrent);
            else
                findloc = searching.lastIndexOf(QRegularExpression(findstring, (ui->checkBoxMatchCase->isChecked() ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption)), m_startCurrent - 1);
        }
        else
        {
            if (ui->radioButtonDown->isChecked())
                findloc = searching.indexOf(findstring, m_endCurrent, (ui->checkBoxMatchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive));
            else
                findloc = searching.lastIndexOf(findstring, m_startCurrent - 1, (ui->checkBoxMatchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive));
        }

        if (findloc != -1)
        {
            m_startCurrent = findloc;
            m_endCurrent = m_startCurrent + findstring.length();

            dynamic_cast<QLineEdit*>(m_findWidget)->setSelection(m_startCurrent, findstring.length());
            dynamic_cast<QLineEdit*>(m_findWidget)->setFocus();
        }
        else
        {
            if (!quiet)
            {
                QMessageBox::StandardButton reply;

                reply = QMessageBox::question(this, "End of Text", "Restart from the beginning?",
                                            QMessageBox::Yes|QMessageBox::No);

                if (reply == QMessageBox::Yes)
                {
                    if (ui->radioButtonDown)
                        m_startCurrent = m_endCurrent = 0;
                    else
                        m_startCurrent = m_endCurrent = qMax(0, searching.length() - 1);
                }
            }
        }

        return (findloc != -1);
    }
    else
    {
        QTextCursor current;
        QTextCursor found;
        QTextDocument* doc;

        if (QString(m_findWidget->metaObject()->className()).compare("PlainTextEdit") == 0)
        {
            current = dynamic_cast<PlainTextEdit*>(m_findWidget)->textCursor();
            doc = dynamic_cast<PlainTextEdit*>(m_findWidget)->document();
        }
        else
        {
            current = dynamic_cast<TextEdit*>(m_findWidget)->textCursor();
            doc = dynamic_cast<TextEdit*>(m_findWidget)->document();
        }

        if (ui->checkBoxUseRegEx->isChecked())
        {
            if (ui->radioButtonDown->isChecked())
                if ( ui->checkBoxMatchCase->isChecked() )
                    found = doc->find(
                                QRegularExpression(ui->lineEditFind->text()),
                                current,
                                QTextDocument::FindCaseSensitively);
                else
                    found = doc->find(
                                QRegularExpression(ui->lineEditFind->text()),
                                current);
            else
                found = doc->find(
                            QRegularExpression(ui->lineEditFind->text()),
                            current,
                            (ui->checkBoxMatchCase->isChecked() ? QTextDocument::FindCaseSensitively|QTextDocument::FindBackward : QTextDocument::FindBackward));
        }
        else
        {
            if (ui->radioButtonDown->isChecked())
                if (ui->checkBoxMatchCase->isChecked())
                    found = doc->find(ui->lineEditFind->text(),
                                current,
                                QTextDocument::FindCaseSensitively);
                else
                    found = doc->find(ui->lineEditFind->text(),
                                current);
            else
                found = doc->find(ui->lineEditFind->text(),
                            current,
                            (ui->checkBoxMatchCase->isChecked() ? QTextDocument::FindCaseSensitively|QTextDocument::FindBackward : QTextDocument::FindBackward) );
        }

        if (found.isNull())
        {
            if (!quiet)
            {
                QMessageBox::StandardButton reply;

                reply = QMessageBox::question(this, "End of Text", "Restart from the beginning?",
                                            QMessageBox::Yes|QMessageBox::No);

                if (reply == QMessageBox::Yes)
                {
                    if (QString(m_findWidget->metaObject()->className()).compare("PlainTextEdit") == 0)
                    {
                        if (ui->radioButtonDown->isChecked())
                            dynamic_cast<PlainTextEdit*>(m_findWidget)->moveCursor(QTextCursor::Start);
                        else
                            dynamic_cast<PlainTextEdit*>(m_findWidget)->moveCursor(QTextCursor::End);
                    }
                    else
                    {
                        if (ui->radioButtonDown->isChecked())
                            dynamic_cast<TextEdit*>(m_findWidget)->moveCursor(QTextCursor::Start);
                        else
                            dynamic_cast<TextEdit*>(m_findWidget)->moveCursor(QTextCursor::End);
                    }
                }
            }

            return false;
        }
        else
        {
            if (QString(m_findWidget->metaObject()->className()).compare("PlainTextEdit") == 0)
            {
                dynamic_cast<PlainTextEdit*>(m_findWidget)->setTextCursor(found);
                dynamic_cast<PlainTextEdit*>(m_findWidget)->ensureCursorVisible();
            }
            else
            {
                dynamic_cast<TextEdit*>(m_findWidget)->setTextCursor(found);
                dynamic_cast<TextEdit*>(m_findWidget)->ensureCursorVisible();
            }

            m_findWidget->setFocus();

            QCoreApplication::processEvents();

            return true;
        }
    }
}

void FindReplaceDialog::showReplaceWindow(QLineEdit* lineEdit)
{
    m_findWidget = dynamic_cast<QWidget*>(lineEdit);

    qsizetype s, e;

    s = lineEdit->selectionStart();
    e = lineEdit->selectionEnd();

    // if we have a selection start there
    if (s != -1 && e != -1)
    {
        m_startCurrent = s;
        m_endCurrent = e;
    }
    else
    {
        qsizetype endsearch = lineEdit->cursorPosition();
        m_startCurrent = m_endCurrent = endsearch;
    }

    show();

    lineEdit->setFocus();
}

void FindReplaceDialog::showReplaceWindow(QTextEdit* textEdit)
{
    // setup search window
    m_findWidget = dynamic_cast<QWidget*>(textEdit);

    show();

    m_findWidget->setFocus();
}

void FindReplaceDialog::showReplaceWindow(QPlainTextEdit* plainTextEdit)
{
    // setup search window
    m_findWidget = dynamic_cast<QWidget*>(plainTextEdit);

    show();

    m_findWidget->setFocus();
}

void FindReplaceDialog::on_lineEditFind_returnPressed()
{
    doFind();
}


void FindReplaceDialog::on_pushButtonFindNext_clicked()
{
    doFind();
}


void FindReplaceDialog::on_pushButtonReplace_clicked()
{
    // if current selection isn't changed start search from begining of the selection
    if (ui->checkBoxMatchCase->isChecked())
    {
        if (ui->lineEditReplace->selectedText() != ui->lineEditFind->text())
            m_endCurrent = m_startCurrent;
    }
    else
    {
        if (ui->lineEditReplace->selectedText().toUpper() != ui->lineEditFind->text().toUpper())
            m_endCurrent = m_startCurrent;
    }

    if (doFind())
    {
        if (QString(m_findWidget->metaObject()->className()).compare("QLineEdit") == 0)
        {
            QString replacestring;

            replacestring = dynamic_cast<QLineEdit*>(m_findWidget)->text();
            replacestring.replace(m_startCurrent, m_endCurrent - m_startCurrent, ui->lineEditReplace->text());
            dynamic_cast<QLineEdit*>(m_findWidget)->setText(replacestring);
            dynamic_cast<QLineEdit*>(m_findWidget)->setSelection(m_startCurrent, ui->lineEditReplace->text().length());

            m_findWidget->setFocus();
        }
        else // replace text in text edit
        {
            if (QString(m_findWidget->metaObject()->className()).compare("TextEdit") == 0)
                dynamic_cast<TextEdit*>(m_findWidget)->insertPlainText(ui->lineEditReplace->text());
            else
                dynamic_cast<PlainTextEdit*>(m_findWidget)->insertPlainText(ui->lineEditReplace->text());

            QCoreApplication::processEvents();

            m_findWidget->setFocus();
        }
    }

}

void FindReplaceDialog::on_pushButtonReplaceAll_clicked()
{
    int count = 0;

    if (QString(m_findWidget->metaObject()->className()).compare("QLineEdit") == 0)
    {
        QString replacestring = ui->lineEditReplace->text();

        m_startCurrent = m_endCurrent = 0;

        while (doFind(true))
        {
            replacestring = dynamic_cast<QLineEdit*>(m_findWidget)->text();
            replacestring.replace(m_startCurrent, m_endCurrent - m_startCurrent, ui->lineEditReplace->text());
            dynamic_cast<QLineEdit*>(m_findWidget)->setText(replacestring);
            dynamic_cast<QLineEdit*>(m_findWidget)->setSelection(m_startCurrent, ui->lineEditReplace->text().length());
            m_findWidget->setFocus();

            count++;
        }
    }
    else
    {
        while (doFind(true))
        {   
            if (QString(m_findWidget->metaObject()->className()).compare("TextEdit") == 0)
                dynamic_cast<TextEdit*>(m_findWidget)->insertPlainText(ui->lineEditReplace->text());
            else
                dynamic_cast<PlainTextEdit*>(m_findWidget)->insertPlainText(ui->lineEditReplace->text());

            count++;
        }
    }

    if (count > 0)
    {
        QMessageBox::information(
              this,
              tr("Replace All Complete"),
              QString("%1 occurences found.").arg(count));
    }
}

void FindReplaceDialog::on_pushButtonCancel_clicked()
{
    hide();
}

