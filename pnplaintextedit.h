// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNPLAINTEXTEDIT_H
#define PNPLAINTEXTEDIT_H

#include "pninlinespellchecker.h"

#include <QPlainTextEdit>

class PNPlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    PNPlainTextEdit(QWidget *parent = nullptr);
    ~PNPlainTextEdit();

    PNInlineSpellChecker* inlineSpellChecker() { return m_inlinespellchecker; }

    void contextMenuEvent(QContextMenuEvent *event) override;
    void setAllowEnter(bool t_allow) { m_allow_enter = t_allow; }
    void insertFromMimeData(const QMimeData * source) override;

public slots:
    void checkSpelling();

private slots:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    PNInlineSpellChecker* m_inlinespellchecker = nullptr;
    int m_oldchars = 0;
    bool m_allow_enter = false;
};

#endif // PNPLAINTEXTEDIT_H
