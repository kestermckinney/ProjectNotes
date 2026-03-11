// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include "inlinespellchecker.h"

#include <QTextEdit>

class TextEdit : public QTextEdit
{
    Q_OBJECT

public:
    TextEdit(QWidget *parent = nullptr);
    ~TextEdit();

    InlineSpellChecker* inlineSpellChecker() { return m_inlinespellchecker; }

    void contextMenuEvent(QContextMenuEvent *event) override;

public slots:
    void checkSpelling();
    void slotPasteUnformated();

private:
    InlineSpellChecker* m_inlinespellchecker = nullptr;
    int m_oldchars = 0;
};

#endif // TEXTEDIT_H
