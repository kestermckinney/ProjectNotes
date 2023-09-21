// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNTEXTEDIT_H
#define PNTEXTEDIT_H

#include "pninlinespellchecker.h"

#include <QTextEdit>

class PNTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    PNTextEdit(QWidget *parent = nullptr);
    ~PNTextEdit();

    void contextMenuEvent(QContextMenuEvent *event) override;

public slots:
    void checkSpelling();

private:
    PNInlineSpellChecker* m_inlinespellchecker = nullptr;
};

#endif // PNTEXTEDIT_H
