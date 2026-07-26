// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SPELLCHECK_H
#define SPELLCHECK_H

#include <QObject>
#include <QQuickItem>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

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

// SpellCheck — QML-attachable object. Set `editor` to the TextArea/TextField
// itself (NOT its textDocument — a single-line TextField has no textDocument):
//
//   SpellCheck { editor: someField; enabled: true }
//
// The source text is read from the editor's `textDocument` when it has one
// (TextArea) or its plain `text` otherwise (TextField); edits are applied with
// the editor's own remove()/insert() so both control types work uniformly.
//
// Qt Quick's text node renders neither a wavy (SpellCheckUnderline) nor a
// coloured underline, so the visible red squiggle is drawn by SpellSquiggle.qml
// (a Canvas overlay) from the ranges reported by misspelledRanges(). This class
// supplies word lookup, suggestions, the misspelled-range list, and the editing
// operations the correction menu + SpellCheckDialog need.
class SpellCheck : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem* editor READ editor WRITE setEditor NOTIFY editorChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit SpellCheck(QObject* parent = nullptr);

    QQuickItem* editor() const { return m_editor; }
    void setEditor(QQuickItem* editor);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool e);

    // Word under a text position, and suggestions for it.
    Q_INVOKABLE QString     wordAt(int position) const;
    Q_INVOKABLE QStringList suggestionsFor(const QString& word) const;
    Q_INVOKABLE bool        isMisspelled(const QString& word) const;
    Q_INVOKABLE void        replaceWord(int position, const QString& replacement);
    Q_INVOKABLE void        addToDictionary(const QString& word);

    // Misspelled word ranges for the whole field, as a list of
    // { start, length } — SpellSquiggle draws a red wave under each.
    Q_INVOKABLE QVariantList misspelledRanges() const;

    // Full-field spell-check support (drives SpellCheckDialog.qml).
    // nextMisspelled walks forward from `fromPos` and returns the first
    // unrecognized word as { found: bool, word, start, length }.
    Q_INVOKABLE QVariantMap nextMisspelled(int fromPos) const;
    Q_INVOKABLE void        replaceRange(int start, int length, const QString& replacement);
    Q_INVOKABLE void        replaceAll(const QString& oldWord, const QString& newWord);
    Q_INVOKABLE void        ignoreAll(const QString& word);

signals:
    void editorChanged();
    void enabledChanged();
    // Emitted when the set of misspelled words may have changed for reasons the
    // editor's own textChanged does not cover (dictionary edits, enable toggle).
    void spellingChanged();

private:
    // The editor's current text (from its textDocument if it has one, else its
    // plain `text` property) and a helper to apply an edit via the editor.
    QString sourceText() const;
    void    applyEdit(int start, int length, const QString& replacement);

    QQuickItem* m_editor = nullptr;
    bool        m_enabled = true;
};

#endif // SPELLCHECK_H
