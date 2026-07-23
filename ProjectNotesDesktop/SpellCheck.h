// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SPELLCHECK_H
#define SPELLCHECK_H

#include <QObject>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QStringList>

class Hunspell;
class QTextDocument;

// DesktopSpellChecker — lean Hunspell wrapper, self-contained (no global_Settings
// or credential store). Loads en_US from <exeDir>/dictionary and keeps a personal
// word list in the app data directory. One shared instance for the whole app.
class DesktopSpellChecker
{
public:
    static DesktopSpellChecker& instance();

    bool        available() const { return m_hunspell != nullptr; }
    bool        isGood(const QString& word);
    QStringList suggest(const QString& word);
    void        addToPersonal(const QString& word);
    void        ignore(const QString& word);

private:
    DesktopSpellChecker();
    ~DesktopSpellChecker();
    Q_DISABLE_COPY(DesktopSpellChecker)

    void loadPersonalWords();

    Hunspell*   m_hunspell = nullptr;
    QByteArray  m_affPath;
    QByteArray  m_dicPath;
    QString     m_personalPath;
};

// SpellCheckHighlighter — underlines words Hunspell does not recognize.
class SpellCheckHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit SpellCheckHighlighter(QTextDocument* doc);
    void setEnabled(bool enabled);
    bool enabled() const { return m_enabled; }

protected:
    void highlightBlock(const QString& text) override;

private:
    bool            m_enabled = true;
    QTextCharFormat m_misspelledFormat;
};

// SpellCheck — QML-attachable object. Set `document` to a TextArea/TextEdit's
// `textDocument` to enable inline spell-check on it.
//
//   SpellCheck { document: noteEdit.textDocument; enabled: true }
//
// Also exposes word lookup + suggestions so QML can build a correction menu.
class SpellCheck : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit SpellCheck(QObject* parent = nullptr);

    QQuickTextDocument* document() const { return m_quickDoc; }
    void setDocument(QQuickTextDocument* doc);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool e);

    // Word under a document character position, and suggestions for it.
    Q_INVOKABLE QString     wordAt(int position) const;
    Q_INVOKABLE QStringList suggestionsFor(const QString& word) const;
    Q_INVOKABLE bool        isMisspelled(const QString& word) const;
    Q_INVOKABLE void        replaceWord(int position, const QString& replacement);
    Q_INVOKABLE void        addToDictionary(const QString& word);
    Q_INVOKABLE void        rehighlight();

signals:
    void documentChanged();
    void enabledChanged();

private:
    QQuickTextDocument*    m_quickDoc = nullptr;
    SpellCheckHighlighter* m_highlighter = nullptr;
    bool                   m_enabled = true;

    void rebuild();
};

#endif // SPELLCHECK_H
