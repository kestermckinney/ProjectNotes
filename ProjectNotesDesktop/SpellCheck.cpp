// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "SpellCheck.h"

#include "hunspell/hunspell.hxx"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextCursor>
#include <QTextDocument>

// ── DesktopSpellChecker ───────────────────────────────────────────────────────

DesktopSpellChecker& DesktopSpellChecker::instance()
{
    static DesktopSpellChecker s;
    return s;
}

DesktopSpellChecker::DesktopSpellChecker()
{
    // Dictionary lives next to the executable (deployed by CMake), en_US default.
    const QString dictDir = QCoreApplication::applicationDirPath() + "/dictionary";
    const QString dic = dictDir + "/en_US.dic";
    const QString aff = dictDir + "/en_US.aff";

    if (QFile::exists(dic) && QFile::exists(aff)) {
        m_affPath = aff.toLocal8Bit();
        m_dicPath = dic.toLocal8Bit();
        m_hunspell = new Hunspell(m_affPath.constData(), m_dicPath.constData());
    }

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    m_personalPath = dataDir + "/personal_words.txt";
    loadPersonalWords();
}

DesktopSpellChecker::~DesktopSpellChecker()
{
    delete m_hunspell;
    m_hunspell = nullptr;
}

void DesktopSpellChecker::loadPersonalWords()
{
    if (!m_hunspell)
        return;
    QFile f(m_personalPath);
    if (!f.open(QFile::ReadOnly | QFile::Text))
        return;
    while (!f.atEnd()) {
        const QString word = QString::fromUtf8(f.readLine()).trimmed();
        if (!word.isEmpty())
            m_hunspell->add(word.toStdString());
    }
    f.close();
}

bool DesktopSpellChecker::isGood(const QString& word)
{
    if (!m_hunspell || word.trimmed().isEmpty())
        return true;
    return m_hunspell->spell(word.toStdString()) != 0;
}

QStringList DesktopSpellChecker::suggest(const QString& word)
{
    QStringList out;
    if (!m_hunspell)
        return out;
    const auto suggestions = m_hunspell->suggest(word.toStdString());
    for (const auto& s : suggestions)
        out.append(QString::fromStdString(s));
    return out;
}

void DesktopSpellChecker::addToPersonal(const QString& word)
{
    if (!m_hunspell || word.trimmed().isEmpty())
        return;
    m_hunspell->add(word.toStdString());
    QFile f(m_personalPath);
    if (f.open(QFile::Append | QFile::Text)) {
        f.write(word.trimmed().toUtf8());
        f.write("\n");
        f.close();
    }
}

void DesktopSpellChecker::ignore(const QString& word)
{
    if (m_hunspell && !word.trimmed().isEmpty())
        m_hunspell->add(word.toStdString());   // session-only (not persisted)
}

// ── SpellCheckHighlighter ─────────────────────────────────────────────────────

SpellCheckHighlighter::SpellCheckHighlighter(QTextDocument* doc)
    : QSyntaxHighlighter(doc)
{
    m_misspelledFormat.setUnderlineColor(QColor(0xc0, 0x44, 0x2e));
    m_misspelledFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
}

void SpellCheckHighlighter::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    rehighlight();
}

void SpellCheckHighlighter::highlightBlock(const QString& text)
{
    if (!m_enabled || !DesktopSpellChecker::instance().available())
        return;

    // Words: letters (incl. apostrophes) — skip 1-char tokens and numbers.
    static const QRegularExpression re(QStringLiteral("[\\p{L}][\\p{L}']*"));
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString word = m.captured(0);
        if (word.length() < 2)
            continue;
        if (!DesktopSpellChecker::instance().isGood(word))
            setFormat(m.capturedStart(0), m.capturedLength(0), m_misspelledFormat);
    }
}

// ── SpellCheck (QML) ──────────────────────────────────────────────────────────

SpellCheck::SpellCheck(QObject* parent)
    : QObject(parent)
{}

void SpellCheck::setDocument(QQuickTextDocument* doc)
{
    if (m_quickDoc == doc)
        return;
    m_quickDoc = doc;
    rebuild();
    emit documentChanged();
}

void SpellCheck::setEnabled(bool e)
{
    if (m_enabled == e)
        return;
    m_enabled = e;
    if (m_highlighter)
        m_highlighter->setEnabled(e);
    emit enabledChanged();
}

void SpellCheck::rebuild()
{
    delete m_highlighter;
    m_highlighter = nullptr;
    if (m_quickDoc && m_quickDoc->textDocument()) {
        m_highlighter = new SpellCheckHighlighter(m_quickDoc->textDocument());
        m_highlighter->setEnabled(m_enabled);
    }
}

QString SpellCheck::wordAt(int position) const
{
    if (!m_quickDoc || !m_quickDoc->textDocument())
        return {};
    QTextCursor cursor(m_quickDoc->textDocument());
    cursor.setPosition(position);
    cursor.select(QTextCursor::WordUnderCursor);
    return cursor.selectedText();
}

QStringList SpellCheck::suggestionsFor(const QString& word) const
{
    return DesktopSpellChecker::instance().suggest(word);
}

bool SpellCheck::isMisspelled(const QString& word) const
{
    return DesktopSpellChecker::instance().available()
           && !DesktopSpellChecker::instance().isGood(word);
}

void SpellCheck::replaceWord(int position, const QString& replacement)
{
    if (!m_quickDoc || !m_quickDoc->textDocument())
        return;
    QTextCursor cursor(m_quickDoc->textDocument());
    cursor.setPosition(position);
    cursor.select(QTextCursor::WordUnderCursor);
    if (cursor.hasSelection())
        cursor.insertText(replacement);
}

void SpellCheck::addToDictionary(const QString& word)
{
    DesktopSpellChecker::instance().addToPersonal(word);
    if (m_highlighter)
        m_highlighter->rehighlight();
}

void SpellCheck::rehighlight()
{
    if (m_highlighter)
        m_highlighter->rehighlight();
}
