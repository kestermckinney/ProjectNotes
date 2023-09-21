// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNINLINESPELLCHECKER_H
#define PNINLINESPELLCHECKER_H

#include "spellcheckdialog.h"

#include <QObject>
#include <QTextEdit>
#include <QTextCursor>
#include <QMenu>

class FixedTextCursor
{
public:
    QString word;
    int position;
    int anchor;
};

class PNInlineSpellChecker : public QObject
{
    Q_OBJECT

public:
    explicit PNInlineSpellChecker(QObject *parent = nullptr);
    ~PNInlineSpellChecker();

    QList<QTextEdit::ExtraSelection> spellCheckCursor(QTextCursor& t_cursor, QList<QTextEdit::ExtraSelection>& t_extraselections);
    QList<QTextEdit::ExtraSelection> spellCheckDocument(QTextCursor& t_cursor, QList<QTextEdit::ExtraSelection>& t_extraselections);

    void addSelection(QTextCursor& t_cursor, QList<QTextEdit::ExtraSelection>& t_extraselections);
    void removeIfOverlaps(QTextCursor& t_cursor, QList<QTextEdit::ExtraSelection>& t_extraselections);
    void buildContextMenu(QMenu& t_menu, QTextCursor& t_cursor);
    bool cursorsOverlap(FixedTextCursor& t_cur1, FixedTextCursor& t_cur2);
    bool cursorsOverlap(QTextCursor& t_cur1, QTextCursor& t_cur2);

private slots:
    void slotCorrectWord(QTextCursor& t_cursor, const QString t_word);
    void slotCheckSpelling(QTextCursor& t_cursor);
    void slotAddToDictionary(QTextCursor& t_cursor);
    void slottIgnoreAll(QTextCursor& t_cursor);
    void slotIgnore(QTextCursor& t_cursor);

private:

    QList<FixedTextCursor> m_checkque;

};

#endif // PNINLINESPELLCHECKER_H
