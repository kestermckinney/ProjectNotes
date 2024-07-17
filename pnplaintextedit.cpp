// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

//#include <QDebug>

#include <QMimeData>

#include "pnsettings.h"
#include "pnplaintextedit.h"



PNPlainTextEdit::PNPlainTextEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    m_inlinespellchecker = new PNInlineSpellChecker(this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setWordWrapMode(QTextOption::WordWrap);
    setContentsMargins(0,0,0,0);
    setViewportMargins(0,0,0,0);
    document()->setDocumentMargin(0.0);

    setTabChangesFocus(true);

    connect(this, SIGNAL(textChanged()), this, SLOT(checkSpelling()));

    installEventFilter(this);
}

PNPlainTextEdit::~PNPlainTextEdit()
{
    removeEventFilter(this);

    disconnect(this, SIGNAL(textChanged()), this, SLOT(checkSpelling()));

    delete m_inlinespellchecker;
}

// connected to textChanged signal to check the spelling
void PNPlainTextEdit::checkSpelling()
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    int newchars = document()->characterCount();

    if ( abs(m_oldchars - newchars) > 3 ) // if document has changed significantly check it all, could be a cut paste are new doc
    {
        QTextCursor qtc = QTextCursor(document());
        QList<QTextEdit::ExtraSelection> es = extraSelections();

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

void PNPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
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
    menu.addAction(QIcon(":/icons/delete.png"), "&Delete", this, [this](){insertPlainText("");} )->setEnabled(textCursor().selectedText().length() > 0);
    menu.addSeparator();
    menu.addAction(QIcon(":/icons/selectall.png"), "&Select All", this, SLOT(selectAll()));

    menu.exec(event->globalPos());
}

bool PNPlainTextEdit::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    if (m_allow_enter)
        return false;

    if(event->type() == QKeyEvent::KeyPress)
    {
        QKeyEvent * ke = static_cast<QKeyEvent*>(event);
        if(ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
        {
            focusNextChild();

            return true; // do not process this event further
        }
    }

    return false;
}

void PNPlainTextEdit::insertFromMimeData(const QMimeData* source)
{
    if (source->hasText() && !m_allow_enter)
    {
        QString text = source->text();
        QTextCursor cursor = textCursor();

        for (int x = 0, pos = cursor.positionInBlock(); x < text.size(); x++, pos++)
        {
            if (text[x] == '\n')
            {
                text[x] = ' ';
            }
            else if (text[x] == '\r')
            {
               text[x] = ' ';
            }
        }
        cursor.insertText(text);

        return;
    }

    QPlainTextEdit::insertFromMimeData(source);
}
