// Copyright (C) 2022, 2023 Paul McKinney
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

public slots:
    void checkSpelling();

private slots:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    InlineSpellChecker* m_inlinespellchecker = nullptr;
    int m_oldchars = 0;
    bool m_allowEnter = false;
};

#endif // PLAINTEXTEDIT_H
