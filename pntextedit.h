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

    PNInlineSpellChecker* inlineSpellChecker() { return m_inlinespellchecker; }

    void contextMenuEvent(QContextMenuEvent *event) override;

public slots:
    void checkSpelling();
    void slotPasteUnformated();

private:
    PNInlineSpellChecker* m_inlinespellchecker = nullptr;
    int m_oldchars = 0;
};

#endif // PNTEXTEDIT_H
