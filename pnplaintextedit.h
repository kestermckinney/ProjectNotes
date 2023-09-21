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

    void contextMenuEvent(QContextMenuEvent *event) override;

public slots:
    void checkSpelling();

private:
    PNInlineSpellChecker* m_inlinespellchecker = nullptr;
};

#endif // PNPLAINTEXTEDIT_H
