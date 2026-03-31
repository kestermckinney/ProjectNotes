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

    qsizetype getCRLFCount(QString& searchstring, qsizetype start, qsizetype end);
    bool doFind(bool quiet = false);
    void showReplaceWindow(QLineEdit* lineEdit);
    void showReplaceWindow(QTextEdit* textEdit);
    void showReplaceWindow(QPlainTextEdit* plainTextEdit);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void on_lineEditFind_returnPressed();

    void on_pushButtonFindNext_clicked();

    void on_pushButtonReplace_clicked();

    void on_pushButtonReplaceAll_clicked();

    void on_pushButtonCancel_clicked();

private:
    Ui::FindReplaceDialog *ui;

    // currently checking work
    qsizetype m_startCurrent = 0;
    qsizetype m_endCurrent = 0;

    QWidget* m_findWidget = nullptr;
};

#endif // FINDREPLACEDIALOG_H
