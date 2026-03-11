#include "inlinespellchecker.h"
#include "appsettings.h"

#include <QApplication>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

InlineSpellChecker::InlineSpellChecker(QObject *parent)
    : QObject{parent}
{

}

InlineSpellChecker::~InlineSpellChecker()
{

}

void InlineSpellChecker::buildContextMenu(QMenu& menu, QTextCursor& cursor)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    QString word = cursor.selectedText();
    bool badword = !global_Settings.spellchecker()->isGoodWord(word);

    int c = 10;
    if (badword)
    {
        foreach (QString s, global_Settings.spellchecker()->suggest(word))
        {
            menu.addAction(s, [&cursor, s, this](){slotCorrectWord(cursor, s);});
            c--;
            if (c == 0) break; // max 10 itemss
        }
    }

    menu.addSeparator();
    menu.addAction("Spelling...", [&cursor, this](){slotCheckSpelling(cursor);});
    if (badword)
    {
        menu.addAction("Add To Dictionary", [&cursor, this](){slotAddToDictionary(cursor);});
        menu.addAction("Ignore All", [&cursor, this](){slottIgnoreAll(cursor);});
        menu.addAction("Ignore", [&cursor, this](){slotCheckSpelling(cursor);});
    }
    menu.addSeparator();
}

void InlineSpellChecker::addSelection(QTextCursor& cursor, QList<QTextEdit::ExtraSelection>& extraselections)
{
    QTextCharFormat highlightFormat;
    highlightFormat.setUnderlineColor(QColor("red"));
    highlightFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

    QTextEdit::ExtraSelection es;
    es.cursor = cursor;
    es.format = highlightFormat;

    extraselections << es;
}


bool InlineSpellChecker::cursorsOverlap(QTextCursor& cur1, QTextCursor& cur2)
{
    int s = std::min(cur1.anchor(), cur1.position());
    int e = std::max(cur1.anchor(), cur1.position());

    int sc = std::min(cur2.anchor(), cur2.position());
    int ec = std::max(cur2.anchor(), cur2.position());

    return ( (s >= sc && s <= ec) || (e >= sc && e <= ec) );
}

bool InlineSpellChecker::cursorsOverlap(FixedTextCursor& cur1, FixedTextCursor& cur2)
{
    int s = std::min(cur1.anchor, cur1.position);
    int e = std::max(cur1.anchor, cur1.position);

    int sc = std::min(cur2.anchor, cur2.position);
    int ec = std::max(cur2.anchor, cur2.position);

    return ( (s >= sc && s <= ec) || (e >= sc && e <= ec) );
}

void InlineSpellChecker::removeIfOverlaps(QTextCursor& cursor, QList<QTextEdit::ExtraSelection>& extraselections)
{
    // look for a selection that overlaps and remove it
    // there should only be one
    QList<QTextEdit::ExtraSelection>::iterator i;
    for (i = extraselections.begin(); i != extraselections.end(); ++i)
    {
        // if spellcheck cursors overlap then remove it
        if ( cursorsOverlap((*i).cursor, cursor)
            && (*i).format.underlineStyle() == QTextCharFormat::SpellCheckUnderline)
        {
            extraselections.erase(i);
            break;
        }
    }
}

QList<QTextEdit::ExtraSelection> InlineSpellChecker::spellCheckDocument(QTextCursor& cursor, QList<QTextEdit::ExtraSelection>& extraselections)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return extraselections;  // no dictionary was setup

    extraselections.clear();

    cursor.movePosition(QTextCursor::Start);

    while(!cursor.atEnd())
    {
        cursor.select(QTextCursor::WordUnderCursor);
        QString word = cursor.selectedText();

        // fix a bug with selecting a contraction
        if (cursor.selectionEnd() + 1 < cursor.document()->characterCount())
            if (
                    cursor.document()->characterAt(cursor.selectionEnd()) == QChar('\'') &&
                    cursor.document()->characterAt(cursor.selectionEnd() + 1) == QChar('t')
                )
            {
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
                word = cursor.selectedText();
            }

        if ( !word.isEmpty() &&
            word.at(0).isLetter() )  // work around for eliminating non-words
        {
            if (!global_Settings.spellchecker()->isGoodWord(word))
            {
                addSelection(cursor, extraselections);
            }
        }

        cursor.movePosition(QTextCursor::NextWord);
        QApplication::processEvents();
    }

    return extraselections;
}

QList<QTextEdit::ExtraSelection> InlineSpellChecker::spellCheckCursor(QTextCursor& cursor, QList<QTextEdit::ExtraSelection>& extraselections)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return extraselections;  // no dictionary was setup

    cursor.select(QTextCursor::WordUnderCursor);

    // fix a bug with selecting a contraction
    if (cursor.position() > 0)
    if (
            cursor.document()->characterAt(cursor.position()-1) == QChar('\'')
        )
    {
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
        cursor.select(QTextCursor::WordUnderCursor);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
    }

    // fix a bug with selecting a contraction
    if (cursor.position() > 1)
    if (
            cursor.document()->characterAt(cursor.position()-1) == QChar('t') &&
            cursor.document()->characterAt(cursor.position()-2) == QChar('\'')
        )
    {
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 3);
        cursor.select(QTextCursor::WordUnderCursor);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
    }


    FixedTextCursor ftc;
    ftc.anchor = cursor.anchor();
    ftc.position = cursor.position();
    ftc.word = cursor.selectedText();

    if (!cursor.selectedText().isEmpty())
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
                QTextCursor ntc = QTextCursor(cursor);
                ntc.setPosition((*it).anchor, QTextCursor::MoveAnchor);
                ntc.setPosition((*it).position, QTextCursor::KeepAnchor);

                removeIfOverlaps(ntc, extraselections);
                addSelection(ntc, extraselections);
            }
            else
            {
                QTextCursor ntc = QTextCursor(cursor);
                ntc.setPosition((*it).anchor, QTextCursor::MoveAnchor);
                ntc.setPosition((*it).position, QTextCursor::KeepAnchor);

                // if we marked a good word remove it
                removeIfOverlaps(ntc, extraselections);
            }

            it = m_checkque.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return extraselections;
}

void InlineSpellChecker::slotCorrectWord(QTextCursor& cursor, const QString word)
{
    cursor.select(QTextCursor::WordUnderCursor);

    // this is to fix a bug in how Qt selects words
    if (
            QString(".,\"';:[]{}()&%$#@!<>/?\\|").contains(cursor.selectedText())
        )
    {
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
        cursor.select(QTextCursor::WordUnderCursor);
    }

    cursor.insertText(word);
}

void InlineSpellChecker::slotCheckSpelling(QTextCursor& cursor)
{
    Q_UNUSED(cursor)

    SpellCheckDialog spellcheck_dialog(dynamic_cast<QWidget*>(parent()));
    spellcheck_dialog.spellCheck(dynamic_cast<QWidget*>(parent()));
}

void InlineSpellChecker::slotAddToDictionary(QTextCursor& cursor)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    QTextCursor tc(cursor);
    tc.select(QTextCursor::WordUnderCursor);

    // fix a bug with selecting a contraction
    if (cursor.selectionEnd() + 1 < cursor.document()->characterCount())
        if (
                cursor.document()->characterAt(cursor.selectionEnd()) == QChar('\'') &&
                cursor.document()->characterAt(cursor.selectionEnd() + 1) == QChar('t')
            )
        {
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
            //qDebug() << "reselected to word: " << cursor.selectedText();
        }

    QString word = tc.selectedText();

    global_Settings.spellchecker()->ignoreWord(word);
    global_Settings.spellchecker()->AddToPersonalWordList(word);

    unmarkWord(word);
}

void InlineSpellChecker::slottIgnoreAll(QTextCursor& cursor)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    QTextCursor tc(cursor);
    tc.select(QTextCursor::WordUnderCursor);

    // fix a bug with selecting a contraction
    if (cursor.selectionEnd() + 1 < cursor.document()->characterCount())
        if (
                cursor.document()->characterAt(cursor.selectionEnd()) == QChar('\'') &&
                cursor.document()->characterAt(cursor.selectionEnd() + 1) == QChar('t')
            )
        {
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
        }

    QString word = tc.selectedText();

    global_Settings.spellchecker()->ignoreWord(word);

    unmarkWord(word);
}

void InlineSpellChecker::slotIgnore(QTextCursor& cursor)
{
    if (!global_Settings.spellchecker()->hasDictionary()) return;  // no dictionary was setup

    QTextCursor tc(cursor);
    tc.select(QTextCursor::WordUnderCursor);

    // fix a bug with selecting a contraction
    if (cursor.selectionEnd() + 1 < cursor.document()->characterCount())
        if (
                cursor.document()->characterAt(cursor.selectionEnd()) == QChar('\'') &&
                cursor.document()->characterAt(cursor.selectionEnd() + 1) == QChar('t')
            )
        {
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
        }

    QString word = tc.selectedText();

    global_Settings.spellchecker()->ignoreWord(word);

    unmarkCursor(cursor);
}

void InlineSpellChecker::unmarkCursor(QTextCursor& cursor)
{
    // clear the selection
    QList<QTextEdit::ExtraSelection> es;
    if (QString(parent()->metaObject()->className()).compare("TextEdit") == 0)
    {
        es = dynamic_cast<QTextEdit*>(parent())->extraSelections();
    }
    else
    {
        es = dynamic_cast<QPlainTextEdit*>(parent())->extraSelections();
    }

    removeIfOverlaps(cursor, es);

    if (QString(parent()->metaObject()->className()).compare("TextEdit") == 0)
    {
        dynamic_cast<QTextEdit*>(parent())->setExtraSelections(es);
    }
    else
    {
        dynamic_cast<QPlainTextEdit*>(parent())->setExtraSelections(es);
    }
}

void InlineSpellChecker::unmarkWord(QString& word)
{
    // clear the selection
    QList<QTextEdit::ExtraSelection> es;
    if (QString(parent()->metaObject()->className()).compare("TextEdit") == 0)
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
        if ( (*i).cursor.selectedText().compare(word, Qt::CaseInsensitive) == 0
            && (*i).format.underlineStyle() == QTextCharFormat::SpellCheckUnderline)
        {
            i = es.erase(i);
        }
        else
            i++;
    }

    if (QString(parent()->metaObject()->className()).compare("TextEdit") == 0)
    {
        dynamic_cast<QTextEdit*>(parent())->setExtraSelections(es);
    }
    else
    {
        dynamic_cast<QPlainTextEdit*>(parent())->setExtraSelections(es);
    }
}


