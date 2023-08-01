#include "findreplacedialog.h"
#include "ui_findreplacedialog.h"

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
    if (m_find_line)
    {
        QString findstring = ui->lineEditFind->text();

        QString searching;

        searching = m_find_line->text();

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

            // adjust for CR/LF in string on unix/mac platforms
    //        qsizetype crlfcount_start = getCRLFCount(searching, 0, m_start_current);
    //        qsizetype crlfcount_end = getCRLFCount(searching, 0, m_end_current);

            //m_find_line->setSelection(m_start_current - crlfcount_start, m_end_current - crlfcount_end);
            m_find_line->setSelection(m_start_current, findstring.length());
            m_find_line->setFocus();
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
        QTextCursor current = m_find_text->textCursor();
        QTextCursor found;

        if (ui->checkBoxUseRegEx->isChecked())
        {
            if (ui->radioButtonDown->isChecked())
                if ( ui->checkBoxMatchCase->isChecked() )
                    found = m_find_text->document()->find(
                                QRegularExpression(ui->lineEditFind->text()),
                                current,
                                QTextDocument::FindCaseSensitively);
                else
                    found = m_find_text->document()->find(
                                QRegularExpression(ui->lineEditFind->text()),
                                current);
            else
                found = m_find_text->document()->find(
                            QRegularExpression(ui->lineEditFind->text()),
                            current,
                            (ui->checkBoxMatchCase->isChecked() ? QTextDocument::FindCaseSensitively|QTextDocument::FindBackward : QTextDocument::FindBackward));
        }
        else
        {
            if (ui->radioButtonDown->isChecked())
                if (ui->checkBoxMatchCase->isChecked())
                    found = m_find_text->document()->find(ui->lineEditFind->text(),
                                current,
                                QTextDocument::FindCaseSensitively);
                else
                    found = m_find_text->document()->find(ui->lineEditFind->text(),
                                current);
            else
                found = m_find_text->document()->find(ui->lineEditFind->text(),
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
                    if (ui->radioButtonDown->isChecked())
                        m_find_text->moveCursor(QTextCursor::Start);
                    else
                        m_find_text->moveCursor(QTextCursor::End);
                }
            }

            return false;
        }
        else
        {
            m_find_text->setTextCursor(found);

            QCoreApplication::processEvents();

            return true;
        }
    }

}

void FindReplaceDialog::showReplaceWindow(QLineEdit* t_line_edit)
{
    // setup search window
    m_find_line = t_line_edit;
    m_find_text = nullptr;

    QString searching = m_find_line->text();


    qsizetype s, e;

    s = m_find_line->selectionStart();
    e = m_find_line->selectionEnd();

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
        qsizetype endsearch = m_find_line->cursorPosition();

        // adjust for CR/LF in string on unix/mac platforms
//        int crlfcount_end = GetCRLFCount(searching, 0, endsearch);

        m_start_current = m_end_current = endsearch;// + crlfcount_end;
    }

    show();

    m_find_line->setFocus();
}

void FindReplaceDialog::showReplaceWindow(QTextEdit* t_text_edit)
{
    // setup search window
    m_find_text = t_text_edit;
    m_find_line = nullptr;

//    QString searching = m_find_line->text();

//    qsizetype s, e;

//    s = m_find_text->textCursor().selectionStart();
//    e = m_find_text->textCursor().selectionEnd();

//    // if we have a selection start there
//    if (s != -1 && e != -1)
//    {
//        // adjust for CR/LF in string on unix/mac platforms
//        //int crlfcount_start = GetCRLFCount(searching, 0, s);
//        //int crlfcount_end = GetCRLFCount(searching, 0, e);

//        m_start_current = s;// + crlfcount_start;
//        m_end_current = e;// + crlfcount_end;
//    }
//    else
//    {
//        qsizetype endsearch = m_find_line->cursorPosition();

        // adjust for CR/LF in string on unix/mac platforms
//        int crlfcount_end = GetCRLFCount(searching, 0, endsearch);

//        m_start_current = m_end_current = endsearch;// + crlfcount_end;
//    }

    show();

    m_find_text->setFocus();
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
        if (m_find_line)  // replace text in a line edit
        {
            QString searching;
            QString replacestring;

            searching = m_find_line->text();

            // adjust for CR/LF in string on unix/mac platforms
    //        int crlfcount_start = getCRLFCount(searching, 0, m_start_current);
    //        int crlfcount_end = getCRLFCount(searching, 0, m_end_current);

            replacestring = m_find_line->text();
            replacestring.replace(m_start_current, m_end_current - m_start_current, ui->lineEditReplace->text());
            m_find_line->setText(replacestring);
            m_find_line->setSelection(m_start_current, ui->lineEditReplace->text().length());

            //m_find_line->repl   Replace(m_start_current - crlfcount_start, m_end_current - crlfcount_end, ui->lineEditReplace->text());
            //m_find_line->SetSelection(m_start_current - crlfcount_start, m_end_current - crlfcount_end);
            m_find_line->setFocus();
        }
        else // replace text in text edit
        {
            m_find_text->insertPlainText(ui->lineEditReplace->text());

            QCoreApplication::processEvents();

            m_find_text->setFocus();
        }
    }

}

void FindReplaceDialog::on_pushButtonReplaceAll_clicked()
{
    int count = 0;

    if (m_find_line)
    {
        QString findstring = ui->lineEditFind->text();
        QString replacestring = ui->lineEditReplace->text();

        QString searching = m_find_line->text();

        m_start_current = m_end_current = 0;

        while (doFind(true))
        {
            // adjust for CR/LF in string on unix/mac platforms
    //        int crlfcount_start = getCRLFCount(searching, 0, m_start_current);
    //        int crlfcount_end = getCRLFCount(searching, 0, m_end_current);

            replacestring = m_find_line->text();
            replacestring.replace(m_start_current, m_end_current - m_start_current, ui->lineEditReplace->text());
            m_find_line->setText(replacestring);
            m_find_line->setSelection(m_start_current, ui->lineEditReplace->text().length());
            //m_find_line->repl   Replace(m_start_current - crlfcount_start, m_end_current - crlfcount_end, ui->lineEditReplace->text());
            //m_find_line->SetSelection(m_start_current - crlfcount_start, m_end_current - crlfcount_end);
            m_find_line->setFocus();

            count++;
        }
    }
    else
    {
        while (doFind(true))
        {
            m_find_text->insertPlainText(ui->lineEditReplace->text());
            m_find_text->setFocus();

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

