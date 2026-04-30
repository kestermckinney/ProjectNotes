// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
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

    void setZoomFactor(qreal factor) { m_zoomFactor = factor; }
    qreal zoomFactor() const { return m_zoomFactor; }

    void contextMenuEvent(QContextMenuEvent *event) override;

public slots:
    void checkSpelling();
    void slotPasteUnformated();

private:
    InlineSpellChecker* m_inlinespellchecker = nullptr;
    int m_oldchars = 0;
    qreal m_zoomFactor = 1.0;
};

#endif // TEXTEDIT_H
