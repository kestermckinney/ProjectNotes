// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include "inlinespellchecker.h"

#include <QPlainTextEdit>

class PlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    PlainTextEdit(QWidget *parent = nullptr);
    ~PlainTextEdit();

    InlineSpellChecker* inlineSpellChecker() { return m_inlinespellchecker; }

    void contextMenuEvent(QContextMenuEvent *event) override;
    void setAllowEnter(bool allow) { m_allowEnter = allow; }
    void insertFromMimeData(const QMimeData * source) override;
    void setCenterVertically(bool center);
    void setTopAligned(int topMargin);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

public slots:
    void checkSpelling();

private slots:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void updateVerticalAlignment();

private:
    InlineSpellChecker* m_inlinespellchecker = nullptr;
    int m_oldchars = 0;
    bool m_allowEnter = false;
    bool m_centerVertically = false;
};

#endif // PLAINTEXTEDIT_H
