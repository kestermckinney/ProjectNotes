// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include <QGuiApplication>
#include <QClipboard>

#include "pnsettings.h"
#include "pntextedit.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


PNTextEdit::PNTextEdit(QWidget *parent) : QTextEdit(parent)
{
    m_inlinespellchecker = new PNInlineSpellChecker(this);

    connect(this, SIGNAL(textChanged()), this, SLOT(checkSpelling()));
}

PNTextEdit::~PNTextEdit()
{
    disconnect(this, SIGNAL(textChanged()), this, SLOT(checkSpelling()));

    delete m_inlinespellchecker;
}

// connected to textChanged signal to check the spelling
void PNTextEdit::checkSpelling()
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    int newchars = document()->characterCount();

    if ( abs(m_oldchars - newchars) > 3 ) // if document has changed significantly check it all, could be a cut paste are new doc
    {
        QTextCursor qtc = QTextCursor(document());
        QList<QTextEdit::ExtraSelection> es = extraSelections();
        //qDebug() << "checking all document";

        setExtraSelections( m_inlinespellchecker->spellCheckDocument(qtc, es) );
    }
    else
    {
        QTextCursor qtc = textCursor();
        QList<QTextEdit::ExtraSelection> es = extraSelections();

        setExtraSelections( m_inlinespellchecker->spellCheckCursor(qtc, es) );
    }

    m_oldchars = newchars;
}

void PNTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;

    QTextCursor qtc = cursorForPosition(event->pos());
    qtc.select(QTextCursor::WordUnderCursor);

    m_inlinespellchecker->buildContextMenu(menu, qtc);
    menu.addAction(QIcon(":/icons/editundo.png"), "&Undo", this, SLOT(undo()))->setEnabled(isUndoRedoEnabled());
    menu.addAction(QIcon(":/icons/editredo.png"), "&Redo", this, SLOT(redo()))->setEnabled(isUndoRedoEnabled());
    menu.addSeparator();
    menu.addAction(QIcon(":/icons/editcut.png"), "Cu&t", this, SLOT(cut()))->setEnabled(textCursor().selectedText().length() > 0);
    menu.addAction(QIcon(":/icons/editcopy.png"), "&Copy", this, SLOT(copy()))->setEnabled(textCursor().selectedText().length() > 0);
    menu.addAction(QIcon(":/icons/editpaste.png"), "&Paste", this, SLOT(paste()))->setEnabled(canPaste());
    menu.addAction(QIcon(":/icons/editpaste.png"), "Paste Un&formated", this, SLOT(slotPasteUnformated()))->setEnabled(canPaste());
    menu.addAction(QIcon(":/icons/delete.png"), "&Delete", this, [this](){insertPlainText("");} )->setEnabled(textCursor().selectedText().length() > 0);
    menu.addSeparator();
    menu.addAction(QIcon(":/icons/selectall.png"), "&Select All", this, SLOT(selectAll()));

    menu.exec(event->globalPos());
}

void PNTextEdit::slotPasteUnformated()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    textCursor().insertText(clipboard->text());
}
