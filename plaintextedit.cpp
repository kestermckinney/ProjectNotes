// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

#include <QMimeData>
#include <QResizeEvent>
#include <QShowEvent>
#include <QAbstractTextDocumentLayout>

#include "appsettings.h"
#include "plaintextedit.h"



PlainTextEdit::PlainTextEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    m_inlinespellchecker = new InlineSpellChecker(this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setWordWrapMode(QTextOption::WordWrap);
    setContentsMargins(0,0,0,0);
    setViewportMargins(3, 0, 3, 0);
    document()->setDocumentMargin(0.0);

    setTabChangesFocus(true);

    connect(this, SIGNAL(textChanged()), this, SLOT(checkSpelling()));

    installEventFilter(this);

    setCenterVertically(true);
}

PlainTextEdit::~PlainTextEdit()
{
    removeEventFilter(this);

    disconnect(this, SIGNAL(textChanged()), this, SLOT(checkSpelling()));

    delete m_inlinespellchecker;
}

// connected to textChanged signal to check the spelling
void PlainTextEdit::checkSpelling()
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

void PlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
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

bool PlainTextEdit::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    if (m_allowEnter)
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

void PlainTextEdit::setCenterVertically(bool center)
{
    if (m_centerVertically == center)
        return;

    m_centerVertically = center;

    if (center)
    {
        connect(this, &QPlainTextEdit::textChanged, this, &PlainTextEdit::updateVerticalAlignment);
        connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
                this, &PlainTextEdit::updateVerticalAlignment);
    }
    else
    {
        disconnect(this, &QPlainTextEdit::textChanged, this, &PlainTextEdit::updateVerticalAlignment);
        disconnect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
                   this, &PlainTextEdit::updateVerticalAlignment);
        const QMargins m = viewportMargins();
        setViewportMargins(m.left(), 0, m.right(), 0);
    }

    updateVerticalAlignment();
}

void PlainTextEdit::setTopAligned(int topMargin)
{
    setCenterVertically(false);
    const QMargins m = viewportMargins();
    setViewportMargins(m.left(), topMargin, m.right(), m.bottom());
}

void PlainTextEdit::updateVerticalAlignment()
{
    if (!m_centerVertically)
        return;

    const int textHeight = fontMetrics().height();
    const int topInset = contentsRect().top();
    int topMargin = (height() - textHeight) / 2 - topInset;

    if (topMargin < 0)
        topMargin = 0;

    const QMargins current = viewportMargins();
    if (current.top() != topMargin)
        setViewportMargins(current.left(), topMargin, current.right(), current.bottom());
}

void PlainTextEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateVerticalAlignment();
}

void PlainTextEdit::showEvent(QShowEvent *event)
{
    QPlainTextEdit::showEvent(event);
    updateVerticalAlignment();
}

void PlainTextEdit::insertFromMimeData(const QMimeData* source)
{
    if (source->hasText() && !m_allowEnter)
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
