// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// TextFormatter — QML-accessible singleton that applies rich-text formatting
// to a TextEdit's internal QTextDocument via QTextCursor.
//
// Usage (QML):
//   TextFormatter.toggleBold(noteEdit.textDocument,
//                            noteEdit.selectionStart,
//                            noteEdit.selectionEnd)
//   noteEdit.forceActiveFocus()   // restore selection highlight

#pragma once

#include <QObject>
#include <QColor>
#include <QQuickTextDocument>

class TextFormatter : public QObject
{
    Q_OBJECT

public:
    explicit TextFormatter(QObject* parent = nullptr);

    static TextFormatter* create(QQmlEngine*, QJSEngine*);

    // Toggle bold / italic / underline on the selected range.
    // If selStart == selEnd (no selection) the format is set for the next
    // character typed (i.e. the cursor's char format is updated).
    Q_INVOKABLE void toggleBold(QQuickTextDocument* doc, int selStart, int selEnd);
    Q_INVOKABLE void toggleItalic(QQuickTextDocument* doc, int selStart, int selEnd);
    Q_INVOKABLE void toggleUnderline(QQuickTextDocument* doc, int selStart, int selEnd);

    // Toggle a bullet list on the paragraph(s) that overlap the selection.
    Q_INVOKABLE void toggleBulletList(QQuickTextDocument* doc, int selStart, int selEnd);

    // Increase / decrease font size by 2pt on the selected range (or current paragraph).
    Q_INVOKABLE void increaseFontSize(QQuickTextDocument* doc, int selStart, int selEnd);
    Q_INVOKABLE void decreaseFontSize(QQuickTextDocument* doc, int selStart, int selEnd);

    // Apply a heading level (1–6) or paragraph (0) style to the paragraph(s) in the selection.
    Q_INVOKABLE void applyHeading(QQuickTextDocument* doc, int selStart, int selEnd, int level);

    // Apply a paragraph/list style by index matching the desktop combo:
    //   0=Standard, 1=Bullet(Disc), 2=Bullet(Circle), 3=Bullet(Square),
    //   4=Ordered(Decimal), 5=Ordered(Alpha lower), 6=Ordered(Alpha upper),
    //   7=Ordered(Roman lower), 8=Ordered(Roman upper),
    //   9..14=Heading 1..6
    Q_INVOKABLE void applyStyle(QQuickTextDocument* doc, int selStart, int selEnd, int styleIndex);

    // Increase / decrease the block indent level of all paragraphs in the selection.
    Q_INVOKABLE void indentText(QQuickTextDocument* doc, int selStart, int selEnd);
    Q_INVOKABLE void unindentText(QQuickTextDocument* doc, int selStart, int selEnd);

    // Set paragraph alignment: 0=left, 1=center, 2=right, 3=justify.
    Q_INVOKABLE void setAlignment(QQuickTextDocument* doc, int selStart, int selEnd, int alignment);

    // Apply font family, exact point size, or foreground color to the selection.
    Q_INVOKABLE void applyFontFamily(QQuickTextDocument* doc, int selStart, int selEnd, const QString& family);
    Q_INVOKABLE void applyFontPointSize(QQuickTextDocument* doc, int selStart, int selEnd, int pointSize);
    Q_INVOKABLE void applyFontColor(QQuickTextDocument* doc, int selStart, int selEnd, const QColor& color);

    // Read the format at a given document position (used to pre-populate the font dialog).
    Q_INVOKABLE QString currentFontFamily(QQuickTextDocument* doc, int pos) const;
    Q_INVOKABLE int     currentFontPointSize(QQuickTextDocument* doc, int pos) const;
    Q_INVOKABLE QColor  currentFontColor(QQuickTextDocument* doc, int pos) const;

    // Serialize the backing QTextDocument so toolbar formatting changes are persisted.
    Q_INVOKABLE QString documentHtml(QQuickTextDocument* doc) const;

    // Returns the list of available font families for use in the font picker.
    Q_INVOKABLE QStringList availableFontFamilies() const;
};
