#include "pninlinespellchecker.h"
#include "pnsettings.h"

#include <QApplication>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

PNInlineSpellChecker::PNInlineSpellChecker(QObject *parent)
    : QObject{parent}
{

}

PNInlineSpellChecker::~PNInlineSpellChecker()
{

}

void PNInlineSpellChecker::buildContextMenu(QMenu& t_menu, QTextCursor& t_cursor)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    QString word = t_cursor.selectedText();
    bool badword = !global_Settings.spellchecker()->isGoodWord(word);

    int c = 10;
    if (badword)
    {
        foreach (QString s, global_Settings.spellchecker()->suggest(word))
        {
            t_menu.addAction(s, [&t_cursor, s, this](){slotCorrectWord(t_cursor, s);});
            c--;
            if (c == 0) break; // max 10 itemss
        }
    }

    t_menu.addSeparator();
    t_menu.addAction("Spelling...", [&t_cursor, this](){slotCheckSpelling(t_cursor);});
    if (badword)
    {
        t_menu.addAction("Add To Dictionary", [&t_cursor, this](){slotAddToDictionary(t_cursor);});
        t_menu.addAction("Ignore All", [&t_cursor, this](){slottIgnoreAll(t_cursor);});
        t_menu.addAction("Ignore", [&t_cursor, this](){slotCheckSpelling(t_cursor);});
    }
    t_menu.addSeparator();
}

void PNInlineSpellChecker::addSelection(QTextCursor& t_cursor, QList<QTextEdit::ExtraSelection>& t_extraselections)
{
    QTextCharFormat highlightFormat;
    highlightFormat.setUnderlineColor(QColor("red"));
    highlightFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

    QTextEdit::ExtraSelection es;
    es.cursor = t_cursor;
    es.format = highlightFormat;

    t_extraselections << es;
}


bool PNInlineSpellChecker::cursorsOverlap(QTextCursor& t_cur1, QTextCursor& t_cur2)
{
    int s = std::min(t_cur1.anchor(), t_cur1.position());
    int e = std::max(t_cur1.anchor(), t_cur1.position());

    int sc = std::min(t_cur2.anchor(), t_cur2.position());
    int ec = std::max(t_cur2.anchor(), t_cur2.position());

    return ( (s >= sc && s <= ec) || (e >= sc && e <= ec) );
}

bool PNInlineSpellChecker::cursorsOverlap(FixedTextCursor& t_cur1, FixedTextCursor& t_cur2)
{
    int s = std::min(t_cur1.anchor, t_cur1.position);
    int e = std::max(t_cur1.anchor, t_cur1.position);

    int sc = std::min(t_cur2.anchor, t_cur2.position);
    int ec = std::max(t_cur2.anchor, t_cur2.position);

    return ( (s >= sc && s <= ec) || (e >= sc && e <= ec) );
}

void PNInlineSpellChecker::removeIfOverlaps(QTextCursor& t_cursor, QList<QTextEdit::ExtraSelection>& t_extraselections)
{
    // look for a selection that overlaps and remove it
    // there should only be one
    QList<QTextEdit::ExtraSelection>::iterator i;
    for (i = t_extraselections.begin(); i != t_extraselections.end(); ++i)
    {
        // if spellcheck cursors overlap then remove it
        if ( cursorsOverlap((*i).cursor, t_cursor)
            && (*i).format.underlineStyle() == QTextCharFormat::SpellCheckUnderline)
        {
            t_extraselections.erase(i);
            break;
        }
    }
}

QList<QTextEdit::ExtraSelection> PNInlineSpellChecker::spellCheckDocument(QTextCursor& t_cursor, QList<QTextEdit::ExtraSelection>& t_extraselections)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return t_extraselections;  // no dictionary was setup

    t_extraselections.clear();

    t_cursor.movePosition(QTextCursor::Start);

    while(!t_cursor.atEnd())
    {
        t_cursor.select(QTextCursor::WordUnderCursor);
        QString word = t_cursor.selectedText();

        // fix a bug with selecting a contraction
        if (t_cursor.selectionEnd() + 1 < t_cursor.document()->characterCount())
            if (
                    t_cursor.document()->characterAt(t_cursor.selectionEnd()) == QChar('\'') &&
                    t_cursor.document()->characterAt(t_cursor.selectionEnd() + 1) == QChar('t')
                )
            {
                t_cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
                word = t_cursor.selectedText();
            }

        if ( !word.isEmpty() &&
            word.at(0).isLetter() )  // work around for eliminating non-words
        {
            if (!global_Settings.spellchecker()->isGoodWord(word))
            {
                addSelection(t_cursor, t_extraselections);
            }
        }

        t_cursor.movePosition(QTextCursor::NextWord);
        QApplication::processEvents();
    }

    return t_extraselections;
}

QList<QTextEdit::ExtraSelection> PNInlineSpellChecker::spellCheckCursor(QTextCursor& t_cursor, QList<QTextEdit::ExtraSelection>& t_extraselections)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return t_extraselections;  // no dictionary was setup

    t_cursor.select(QTextCursor::WordUnderCursor);

    // fix a bug with selecting a contraction
    if (t_cursor.position() > 0)
    if (
            t_cursor.document()->characterAt(t_cursor.position()-1) == QChar('\'')
        )
    {
        t_cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
        t_cursor.select(QTextCursor::WordUnderCursor);
        t_cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
    }

    // fix a bug with selecting a contraction
    if (t_cursor.position() > 1)
    if (
            t_cursor.document()->characterAt(t_cursor.position()-1) == QChar('t') &&
            t_cursor.document()->characterAt(t_cursor.position()-2) == QChar('\'')
        )
    {
        t_cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 3);
        t_cursor.select(QTextCursor::WordUnderCursor);
        t_cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
    }


    FixedTextCursor ftc;
    ftc.anchor = t_cursor.anchor();
    ftc.position = t_cursor.position();
    ftc.word = t_cursor.selectedText();

    if (!t_cursor.selectedText().isEmpty())
    {
        // if we just modifed a word, remove it from the word cursor que
        QList<FixedTextCursor>::iterator it = m_checkque.begin();
        while (it != m_checkque.end())
        {
            if ( cursorsOverlap(*it, ftc))
            {
                it = m_checkque.erase(it);
            }
            else
                ++it;
        }

        m_checkque.append(ftc);
    }


    // check all other words in the queue except the current one8
    // if someone clicks anywhere it will always check the last word
    QList<FixedTextCursor>::iterator it = m_checkque.begin();
    while (it != m_checkque.end())
    {
        if ( !cursorsOverlap((*it), ftc) )
        {

            if ( !global_Settings.spellchecker()->isGoodWord((*it).word))
            {
                QTextCursor ntc = QTextCursor(t_cursor);
                ntc.setPosition((*it).anchor, QTextCursor::MoveAnchor);
                ntc.setPosition((*it).position, QTextCursor::KeepAnchor);

                removeIfOverlaps(ntc, t_extraselections);
                addSelection(ntc, t_extraselections);
            }
            else
            {
                QTextCursor ntc = QTextCursor(t_cursor);
                ntc.setPosition((*it).anchor, QTextCursor::MoveAnchor);
                ntc.setPosition((*it).position, QTextCursor::KeepAnchor);

                // if we marked a good word remove it
                removeIfOverlaps(ntc, t_extraselections);
            }

            it = m_checkque.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return t_extraselections;
}

void PNInlineSpellChecker::slotCorrectWord(QTextCursor& t_cursor, const QString t_word)
{
    t_cursor.select(QTextCursor::WordUnderCursor);

    // this is to fix a bug in how Qt selects words
    if (
            QString(".,\"';:[]{}()&%$#@!<>/?\\|").contains(t_cursor.selectedText())
        )
    {
        t_cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
        t_cursor.select(QTextCursor::WordUnderCursor);
    }

    t_cursor.insertText(t_word);
}

void PNInlineSpellChecker::slotCheckSpelling(QTextCursor& t_cursor)
{
    Q_UNUSED(t_cursor)

    SpellCheckDialog spellcheck_dialog(dynamic_cast<QWidget*>(parent()));
    spellcheck_dialog.spellCheck(dynamic_cast<QWidget*>(parent()));
}

void PNInlineSpellChecker::slotAddToDictionary(QTextCursor& t_cursor)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    QTextCursor tc(t_cursor);
    tc.select(QTextCursor::WordUnderCursor);

    // fix a bug with selecting a contraction
    if (t_cursor.selectionEnd() + 1 < t_cursor.document()->characterCount())
        if (
                t_cursor.document()->characterAt(t_cursor.selectionEnd()) == QChar('\'') &&
                t_cursor.document()->characterAt(t_cursor.selectionEnd() + 1) == QChar('t')
            )
        {
            t_cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
            //qDebug() << "reselected to word: " << t_cursor.selectedText();
        }

    QString word = tc.selectedText();

    global_Settings.spellchecker()->ignoreWord(word);
    global_Settings.spellchecker()->AddToPersonalWordList(word);

    unmarkWord(word);
}

void PNInlineSpellChecker::slottIgnoreAll(QTextCursor& t_cursor)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    QTextCursor tc(t_cursor);
    tc.select(QTextCursor::WordUnderCursor);

    // fix a bug with selecting a contraction
    if (t_cursor.selectionEnd() + 1 < t_cursor.document()->characterCount())
        if (
                t_cursor.document()->characterAt(t_cursor.selectionEnd()) == QChar('\'') &&
                t_cursor.document()->characterAt(t_cursor.selectionEnd() + 1) == QChar('t')
            )
        {
            t_cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
        }

    QString word = tc.selectedText();

    global_Settings.spellchecker()->ignoreWord(word);

    unmarkWord(word);
}

void PNInlineSpellChecker::slotIgnore(QTextCursor& t_cursor)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    QTextCursor tc(t_cursor);
    tc.select(QTextCursor::WordUnderCursor);

    // fix a bug with selecting a contraction
    if (t_cursor.selectionEnd() + 1 < t_cursor.document()->characterCount())
        if (
                t_cursor.document()->characterAt(t_cursor.selectionEnd()) == QChar('\'') &&
                t_cursor.document()->characterAt(t_cursor.selectionEnd() + 1) == QChar('t')
            )
        {
            t_cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
        }

    QString word = tc.selectedText();

    global_Settings.spellchecker()->ignoreWord(word);

    unmarkCursor(t_cursor);
}

void PNInlineSpellChecker::unmarkCursor(QTextCursor& t_cursor)
{
    // clear the selection
    QList<QTextEdit::ExtraSelection> es;
    if (QString(parent()->metaObject()->className()).compare("PNTextEdit") == 0)
    {
        es = dynamic_cast<QTextEdit*>(parent())->extraSelections();
    }
    else
    {
        es = dynamic_cast<QPlainTextEdit*>(parent())->extraSelections();
    }

    removeIfOverlaps(t_cursor, es);

    if (QString(parent()->metaObject()->className()).compare("PNTextEdit") == 0)
    {
        dynamic_cast<QTextEdit*>(parent())->setExtraSelections(es);
    }
    else
    {
        dynamic_cast<QPlainTextEdit*>(parent())->setExtraSelections(es);
    }
}

void PNInlineSpellChecker::unmarkWord(QString& t_word)
{
    // clear the selection
    QList<QTextEdit::ExtraSelection> es;
    if (QString(parent()->metaObject()->className()).compare("PNTextEdit") == 0)
    {
        es = dynamic_cast<QTextEdit*>(parent())->extraSelections();
    }
    else
    {
        es = dynamic_cast<QPlainTextEdit*>(parent())->extraSelections();
    }

    QList<QTextEdit::ExtraSelection>::iterator i = es.begin();
    while( i != es.end() )
    {
        // if spellcheck cursors overlap then remove it
        if ( (*i).cursor.selectedText().compare(t_word, Qt::CaseInsensitive) == 0
            && (*i).format.underlineStyle() == QTextCharFormat::SpellCheckUnderline)
        {
            i = es.erase(i);
        }
        else
            i++;
    }

    if (QString(parent()->metaObject()->className()).compare("PNTextEdit") == 0)
    {
        dynamic_cast<QTextEdit*>(parent())->setExtraSelections(es);
    }
    else
    {
        dynamic_cast<QPlainTextEdit*>(parent())->setExtraSelections(es);
    }
}


