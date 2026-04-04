// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef INLINESPELLCHECKER_H
#define INLINESPELLCHECKER_H

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

class InlineSpellChecker : public QObject
{
    Q_OBJECT

public:
    explicit InlineSpellChecker(QObject *parent = nullptr);
    ~InlineSpellChecker();

    QList<QTextEdit::ExtraSelection> spellCheckCursor(QTextCursor& cursor, QList<QTextEdit::ExtraSelection>& extraselections);
    QList<QTextEdit::ExtraSelection> spellCheckDocument(QTextCursor& cursor, QList<QTextEdit::ExtraSelection>& extraselections);

    void addSelection(QTextCursor& cursor, QList<QTextEdit::ExtraSelection>& extraselections);
    void removeIfOverlaps(QTextCursor& cursor, QList<QTextEdit::ExtraSelection>& extraselections);
    void buildContextMenu(QMenu& menu, QTextCursor& cursor);
    bool cursorsOverlap(FixedTextCursor& cur1, FixedTextCursor& cur2);
    bool cursorsOverlap(QTextCursor& cur1, QTextCursor& cur2);
    void unmarkWord(QString& word);
    void unmarkCursor(QTextCursor& cursor);

private slots:
    void slotCorrectWord(QTextCursor& cursor, const QString word);
    void slotCheckSpelling(QTextCursor& cursor);
    void slotAddToDictionary(QTextCursor& cursor);
    void slottIgnoreAll(QTextCursor& cursor);
    void slotIgnore(QTextCursor& cursor);

private:

    QList<FixedTextCursor> m_checkque;

};

#endif // INLINESPELLCHECKER_H
