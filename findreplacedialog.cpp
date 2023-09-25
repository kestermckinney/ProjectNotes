// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "findreplacedialog.h"
#include "ui_findreplacedialog.h"
#include "pnplaintextedit.h"
#include "pntextedit.h"

#include <QRegularExpression>
#include <QMessageBox>

FindReplaceDialog::FindReplaceDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new Ui::FindReplaceDialog)
{
    ui->setupUi(this);
}

FindReplaceDialog::~FindReplaceDialog()
{
    delete ui;
}

qsizetype FindReplaceDialog::getCRLFCount(QString& t_searchstring, qsizetype t_start, qsizetype t_end)
{
    qsizetype count = 0;
    qsizetype currentstart = t_start;

    while ( currentstart < t_end )
    {
        if ( t_searchstring.at(currentstart) == '\r' && t_searchstring.at(currentstart + 1) == '\n' )
            count++;

        currentstart++;
    }

    return count;
}

bool FindReplaceDialog::doFind(bool t_quiet)
{
    if (QString(m_find_widget->metaObject()->className()).compare("QLineEdit") == 0)
    {
        QString findstring = ui->lineEditFind->text();

        QString searching;

        searching = dynamic_cast<QLineEdit*>(m_find_widget)->text();

        qsizetype findloc;

        if (ui->checkBoxUseRegEx->isChecked())
        {
            if (ui->radioButtonDown->isChecked())
                findloc = searching.indexOf(QRegularExpression(findstring, (ui->checkBoxMatchCase->isChecked() ? QRegularExpression::NoPatternOption  : QRegularExpression::CaseInsensitiveOption)), m_end_current);
            else
                findloc = searching.lastIndexOf(QRegularExpression(findstring, (ui->checkBoxMatchCase->isChecked() ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption)), m_start_current - 1);
        }
        else
        {
            if (ui->radioButtonDown->isChecked())
                findloc = searching.indexOf(findstring, m_end_current, (ui->checkBoxMatchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive));
            else
                findloc = searching.lastIndexOf(findstring, m_start_current - 1, (ui->checkBoxMatchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive));
        }

        if (findloc != -1)
        {
            m_start_current = findloc;
            m_end_current = m_start_current + findstring.length();

            dynamic_cast<QLineEdit*>(m_find_widget)->setSelection(m_start_current, findstring.length());
            dynamic_cast<QLineEdit*>(m_find_widget)->setFocus();
        }
        else
        {
            if (!t_quiet)
            {
                QMessageBox::StandardButton reply;

                reply = QMessageBox::question(this, "End of Text", "Restart from the beginning?",
                                            QMessageBox::Yes|QMessageBox::No);

                if (reply == QMessageBox::Yes)
                {
                    if (ui->radioButtonDown)
                        m_start_current = m_end_current = 0;
                    else
                        m_start_current = m_end_current = qMax(0, searching.length() - 1);
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

        if (QString(m_find_widget->metaObject()->className()).compare("PNPlainTextEdit") == 0)
        {
            current = dynamic_cast<PNPlainTextEdit*>(m_find_widget)->textCursor();
            doc = dynamic_cast<PNPlainTextEdit*>(m_find_widget)->document();
        }
        else
        {
            current = dynamic_cast<PNTextEdit*>(m_find_widget)->textCursor();
            doc = dynamic_cast<PNTextEdit*>(m_find_widget)->document();
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
            if (!t_quiet)
            {
                QMessageBox::StandardButton reply;

                reply = QMessageBox::question(this, "End of Text", "Restart from the beginning?",
                                            QMessageBox::Yes|QMessageBox::No);

                if (reply == QMessageBox::Yes)
                {
                    if (QString(m_find_widget->metaObject()->className()).compare("PNPlainTextEdit") == 0)
                    {
                        if (ui->radioButtonDown->isChecked())
                            dynamic_cast<PNPlainTextEdit*>(m_find_widget)->moveCursor(QTextCursor::Start);
                        else
                            dynamic_cast<PNPlainTextEdit*>(m_find_widget)->moveCursor(QTextCursor::End);
                    }
                    else
                    {
                        if (ui->radioButtonDown->isChecked())
                            dynamic_cast<PNTextEdit*>(m_find_widget)->moveCursor(QTextCursor::Start);
                        else
                            dynamic_cast<PNTextEdit*>(m_find_widget)->moveCursor(QTextCursor::End);
                    }
                }
            }

            return false;
        }
        else
        {
            if (QString(m_find_widget->metaObject()->className()).compare("PNPlainTextEdit") == 0)
            {
                dynamic_cast<PNPlainTextEdit*>(m_find_widget)->setTextCursor(found);
            }
            else
            {
                dynamic_cast<PNTextEdit*>(m_find_widget)->setTextCursor(found);
            }

            QCoreApplication::processEvents();

            return true;
        }
    }
}

void FindReplaceDialog::showReplaceWindow(QLineEdit* t_line_edit)
{
    m_find_widget = dynamic_cast<QWidget*>(t_line_edit);

    qsizetype s, e;

    s = t_line_edit->selectionStart();
    e = t_line_edit->selectionEnd();

    // if we have a selection start there
    if (s != -1 && e != -1)
    {
        // adjust for CR/LF in string on unix/mac platforms
//        int crlfcount_start = GetCRLFCount(searching, 0, s);
//        int crlfcount_end = GetCRLFCount(searching, 0, e);

        m_start_current = s;// + crlfcount_start;
        m_end_current = e;// + crlfcount_end;
    }
    else
    {
        qsizetype endsearch = t_line_edit->cursorPosition();

        // adjust for CR/LF in string on unix/mac platforms
//        int crlfcount_end = GetCRLFCount(searching, 0, endsearch);

        m_start_current = m_end_current = endsearch;// + crlfcount_end;
    }

    show();

    t_line_edit->setFocus();
}

void FindReplaceDialog::showReplaceWindow(QTextEdit* t_text_edit)
{
    // setup search window
    m_find_widget = dynamic_cast<QWidget*>(t_text_edit);

    show();

    m_find_widget->setFocus();
}

void FindReplaceDialog::showReplaceWindow(QPlainTextEdit* t_plain_text_edit)
{
    // setup search window
    m_find_widget = dynamic_cast<QWidget*>(t_plain_text_edit);

    show();

    m_find_widget->setFocus();
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
            m_end_current = m_start_current;
    }
    else
    {
        if (ui->lineEditReplace->selectedText().toUpper() != ui->lineEditFind->text().toUpper())
            m_end_current = m_start_current;
    }

    if (doFind())
    {
        if (QString(m_find_widget->metaObject()->className()).compare("QLineEdit") == 0)
        {
            QString replacestring;

            replacestring = dynamic_cast<QLineEdit*>(m_find_widget)->text();
            replacestring.replace(m_start_current, m_end_current - m_start_current, ui->lineEditReplace->text());
            dynamic_cast<QLineEdit*>(m_find_widget)->setText(replacestring);
            dynamic_cast<QLineEdit*>(m_find_widget)->setSelection(m_start_current, ui->lineEditReplace->text().length());

            m_find_widget->setFocus();
        }
        else // replace text in text edit
        {
            if (QString(m_find_widget->metaObject()->className()).compare("PNTextEdit") == 0)
                dynamic_cast<PNTextEdit*>(m_find_widget)->insertPlainText(ui->lineEditReplace->text());
            else
                dynamic_cast<PNPlainTextEdit*>(m_find_widget)->insertPlainText(ui->lineEditReplace->text());

            QCoreApplication::processEvents();

            m_find_widget->setFocus();
        }
    }

}

void FindReplaceDialog::on_pushButtonReplaceAll_clicked()
{
    int count = 0;

    if (QString(m_find_widget->metaObject()->className()).compare("QLineEdit") == 0)
    {
        QString replacestring = ui->lineEditReplace->text();

        m_start_current = m_end_current = 0;

        while (doFind(true))
        {
            replacestring = dynamic_cast<QLineEdit*>(m_find_widget)->text();
            replacestring.replace(m_start_current, m_end_current - m_start_current, ui->lineEditReplace->text());
            dynamic_cast<QLineEdit*>(m_find_widget)->setText(replacestring);
            dynamic_cast<QLineEdit*>(m_find_widget)->setSelection(m_start_current, ui->lineEditReplace->text().length());
            m_find_widget->setFocus();

            count++;
        }
    }
    else
    {
        while (doFind(true))
        {   
            if (QString(m_find_widget->metaObject()->className()).compare("PNTextEdit") == 0)
                dynamic_cast<PNTextEdit*>(m_find_widget)->insertPlainText(ui->lineEditReplace->text());
            else
                dynamic_cast<PNPlainTextEdit*>(m_find_widget)->insertPlainText(ui->lineEditReplace->text());

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

