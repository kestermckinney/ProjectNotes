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

// ── SpellCheck (QML) ──────────────────────────────────────────────────────────

// Word tokens: a letter followed by letters/apostrophes. The same regex is used
// for the squiggle ranges, the dialog walk, and Change-All, so all three agree
// on what a "word" is. Text positions align 1:1 with the source string indices
// used by the editor's positionToRectangle()/remove()/insert() (for a rich-text
// TextArea, block separators count as one character in both the document and its
// toPlainText()).
static const QRegularExpression& wordRegex()
{
    static const QRegularExpression re(QStringLiteral("[\\p{L}][\\p{L}']*"));
    return re;
}

SpellCheck::SpellCheck(QObject* parent)
    : QObject(parent)
{}

void SpellCheck::setEditor(QQuickItem* editor)
{
    if (m_editor == editor)
        return;
    m_editor = editor;
    emit editorChanged();
    emit spellingChanged();
}

void SpellCheck::setEnabled(bool e)
{
    if (m_enabled == e)
        return;
    m_enabled = e;
    emit enabledChanged();
    emit spellingChanged();
}

// The field's text: a TextArea (TextEdit) exposes a `textDocument`, so read its
// plain text (positions match remove/insert); a TextField (TextInput) has none,
// so fall back to its plain `text` property.
QString SpellCheck::sourceText() const
{
    if (!m_editor)
        return {};
    QQuickTextDocument* qd = m_editor->property("textDocument").value<QQuickTextDocument*>();
    if (qd && qd->textDocument())
        return qd->textDocument()->toPlainText();
    return m_editor->property("text").toString();
}

// Apply a replacement over [start, start+length). For a rich-text editor
// (TextArea, which exposes a textDocument), we go through QTextCursor directly
// and re-apply the replaced run's own QTextCharFormat — the editor's own QML
// insert() otherwise inserts using the document's default char format, which
// silently drops any bold/italic/font override the replaced word had. A plain
// TextField has no textDocument and no rich formatting to lose, so it keeps
// using the editor's own remove()/insert() methods.
void SpellCheck::applyEdit(int start, int length, const QString& replacement)
{
    if (!m_editor)
        return;

    QQuickTextDocument* qd = m_editor->property("textDocument").value<QQuickTextDocument*>();
    if (qd && qd->textDocument()) {
        QTextCursor cursor(qd->textDocument());
        cursor.setPosition(start);
        cursor.setPosition(start + length, QTextCursor::KeepAnchor);
        const QTextCharFormat format = cursor.charFormat();
        cursor.removeSelectedText();
        if (!replacement.isEmpty())
            cursor.insertText(replacement, format);
        return;
    }

    QMetaObject::invokeMethod(m_editor, "remove", Q_ARG(int, start), Q_ARG(int, start + length));
    if (!replacement.isEmpty())
        QMetaObject::invokeMethod(m_editor, "insert", Q_ARG(int, start), Q_ARG(QString, replacement));
}

QVariantList SpellCheck::misspelledRanges() const
{
    QVariantList out;
    if (!m_enabled || !m_editor || !DesktopSpellChecker::instance().available())
        return out;

    const QString text = sourceText();
    auto it = wordRegex().globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString word = m.captured(0);
        if (word.length() < 2)
            continue;
        if (!DesktopSpellChecker::instance().isGood(word)) {
            QVariantMap r;
            r["start"]  = m.capturedStart(0);
            r["length"] = m.capturedLength(0);
            out.append(r);
        }
    }
    return out;
}

QString SpellCheck::wordAt(int position) const
{
    const QString text = sourceText();
    auto it = wordRegex().globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        if (position >= m.capturedStart(0) && position <= m.capturedEnd(0))
            return m.captured(0);
    }
    return {};
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
    const QString text = sourceText();
    auto it = wordRegex().globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        if (position >= m.capturedStart(0) && position <= m.capturedEnd(0)) {
            applyEdit(m.capturedStart(0), m.capturedLength(0), replacement);
            return;
        }
    }
}

void SpellCheck::addToDictionary(const QString& word)
{
    DesktopSpellChecker::instance().addToPersonal(word);
    emit spellingChanged();
}

// ── Full-field spell-check (drives SpellCheckDialog.qml) ──────────────────────

QVariantMap SpellCheck::nextMisspelled(int fromPos) const
{
    QVariantMap result;
    result["found"] = false;
    if (!m_editor || !DesktopSpellChecker::instance().available())
        return result;

    const QString text = sourceText();
    auto it = wordRegex().globalMatch(text, qMax(0, fromPos));
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString word = m.captured(0);
        if (word.length() < 2)
            continue;
        if (!DesktopSpellChecker::instance().isGood(word)) {
            result["found"]  = true;
            result["word"]   = word;
            result["start"]  = m.capturedStart(0);
            result["length"] = m.capturedLength(0);
            return result;
        }
    }
    return result;
}

void SpellCheck::replaceRange(int start, int length, const QString& replacement)
{
    applyEdit(start, length, replacement);
}

void SpellCheck::replaceAll(const QString& oldWord, const QString& newWord)
{
    if (!m_editor || oldWord.isEmpty())
        return;
    const QString text = sourceText();

    // Collect match starts first; applying edits back-to-front keeps the earlier
    // offsets valid. Every match equals oldWord, so the length is constant.
    QList<int> starts;
    auto it = wordRegex().globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        if (m.captured(0) == oldWord)
            starts.append(m.capturedStart(0));
    }

    const int len = oldWord.length();
    for (int i = starts.size() - 1; i >= 0; --i)
        applyEdit(starts[i], len, newWord);
}

void SpellCheck::ignoreAll(const QString& word)
{
    DesktopSpellChecker::instance().ignore(word);   // session-only
    emit spellingChanged();
}
