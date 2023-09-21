#include "pninlinespellchecker.h"
#include "pnsettings.h"

#include <QApplication>
#include <QDebug>

PNInlineSpellChecker::PNInlineSpellChecker(QObject *parent)
    : QObject{parent}
{

}

PNInlineSpellChecker::~PNInlineSpellChecker()
{

}

void PNInlineSpellChecker::buildContextMenu(QMenu& t_menu, QTextCursor& t_cursor)
{
    QString word = t_cursor.selectedText();
    bool badword = !global_Settings.spellchecker()->isGoodWord(word);

    if (badword)
        foreach (QString s, global_Settings.spellchecker()->suggest(word))
            t_menu.addAction(s, [&t_cursor, s, this](){slotCorrectWord(t_cursor, s);});

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
    t_extraselections.clear();

    t_cursor.movePosition(QTextCursor::Start);

    while(!t_cursor.atEnd())
    {
        t_cursor.select(QTextCursor::WordUnderCursor);
        QString word = t_cursor.selectedText();

        if ( !word.isEmpty() &&
            word.at(0).isLetter() )  // work around for eliminating non-words
        {
            if (!global_Settings.spellchecker()->isGoodWord(word))
            {
                //removeIfOverlaps(t_cursor, t_extraselections);
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
    // if set text was called then we need to check the entire document
    if ( t_cursor.anchor() == 0 && t_cursor.position() == 0)
    {
        return spellCheckDocument(t_cursor, t_extraselections);
    }

    t_cursor.select(QTextCursor::WordUnderCursor);

    FixedTextCursor ftc;
    ftc.anchor = t_cursor.anchor();
    ftc.position = t_cursor.position();
    ftc.word = t_cursor.selectedText();

    QString word = t_cursor.selectedText();
    qDebug() << ">> selcted " << word << " trimmed " << word.trimmed();

    if (!t_cursor.selectedText().isEmpty())
    {
        // if we just modifed a word, remove it from the word cursor que
        QList<FixedTextCursor>::iterator it = m_checkque.begin();
        while (it != m_checkque.end())
        {
            if ( cursorsOverlap(*it, ftc))
            {
                qDebug() << "removing " << ftc.word;
                it = m_checkque.erase(it);
            }
            else
                ++it;
        }

        qDebug() << "adding word " << t_cursor.selectedText() << t_cursor.position() << "  " << t_cursor.anchor();
        m_checkque.append(ftc);
    }

    qDebug() << "list size " << m_checkque.count();

    // check all other words in the queue except the current one8
    // if someone clicks anywhere it will always check the last word
    QList<FixedTextCursor>::iterator it = m_checkque.begin();
    while (it != m_checkque.end())
    {
        qDebug() << "list: " << (*it).word;
        if ( !cursorsOverlap((*it), ftc) )
        {
            qDebug() << "checking word: " << (*it).word;

            if ( !global_Settings.spellchecker()->isGoodWord((*it).word))
            {
                QTextCursor ntc = QTextCursor(t_cursor);
                ntc.setPosition((*it).anchor, QTextCursor::MoveAnchor);
                ntc.setPosition((*it).position, QTextCursor::KeepAnchor);

                qDebug() << "word is bad: " << ntc.selectedText();
                removeIfOverlaps(ntc, t_extraselections);
                addSelection(ntc, t_extraselections);
            }
            else
            {
                QTextCursor ntc = QTextCursor(t_cursor);
                ntc.setPosition((*it).anchor, QTextCursor::MoveAnchor);
                ntc.setPosition((*it).position, QTextCursor::KeepAnchor);

                // if we marked a good word remove it
                qDebug() << "word is good: " << ntc.selectedText();
                removeIfOverlaps(ntc, t_extraselections);
            }

            it = m_checkque.erase(it);
        }
        else
        {
            qDebug() << "not checking yet: " << (*it).word;
            ++it;
        }
    }

    return t_extraselections;
}

void PNInlineSpellChecker::slotCorrectWord(QTextCursor& t_cursor, const QString t_word)
{
    t_cursor.select(QTextCursor::WordUnderCursor);
    t_cursor.insertText(t_word);
}

void PNInlineSpellChecker::slotCheckSpelling(QTextCursor& t_cursor)
{
    SpellCheckDialog spellcheck_dialog(dynamic_cast<QWidget*>(parent()));
    spellcheck_dialog.spellCheck(dynamic_cast<QWidget*>(parent()));
}

void PNInlineSpellChecker::slotAddToDictionary(QTextCursor& t_cursor)
{
    QTextCursor tc(t_cursor);
    tc.select(QTextCursor::WordUnderCursor);

    QString word = tc.selectedText();

    global_Settings.spellchecker()->ignoreWord(word);
    global_Settings.spellchecker()->AddToPersonalWordList(word);
}

void PNInlineSpellChecker::slottIgnoreAll(QTextCursor& t_cursor)
{
    QTextCursor tc(t_cursor);
    tc.select(QTextCursor::WordUnderCursor);

    QString word = tc.selectedText();

    global_Settings.spellchecker()->ignoreWord(word);
}

void PNInlineSpellChecker::slotIgnore(QTextCursor& t_cursor)
{
    QTextCursor tc(t_cursor);
    tc.select(QTextCursor::WordUnderCursor);

    QString word = tc.selectedText();

    global_Settings.spellchecker()->ignoreWord(word);
}
