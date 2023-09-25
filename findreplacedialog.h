// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef FINDREPLACEDIALOG_H
#define FINDREPLACEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>

namespace Ui {
class FindReplaceDialog;
}

class FindReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindReplaceDialog(QWidget *parent = nullptr);
    ~FindReplaceDialog();

    qsizetype getCRLFCount(QString& t_searchstring, qsizetype t_start, qsizetype t_end);
    bool doFind(bool t_quiet = false);
    void showReplaceWindow(QLineEdit* t_line_edit);
    void showReplaceWindow(QTextEdit* t_text_edit);
    void showReplaceWindow(QPlainTextEdit* t_plain_text_edit);

private slots:
    void on_lineEditFind_returnPressed();

    void on_pushButtonFindNext_clicked();

    void on_pushButtonReplace_clicked();

    void on_pushButtonReplaceAll_clicked();

    void on_pushButtonCancel_clicked();

private:
    Ui::FindReplaceDialog *ui;

    // currently checking work
    qsizetype m_start_current = 0;
    qsizetype m_end_current = 0;

    QWidget* m_find_widget = nullptr;
};

#endif // FINDREPLACEDIALOG_H
