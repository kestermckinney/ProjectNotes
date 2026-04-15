// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "TextFormatter.h"

#include <QQuickTextDocument>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QTextList>
#include <QTextListFormat>
#include <QBrush>
#include <QColor>
#include <QFontDatabase>

TextFormatter::TextFormatter(QObject* parent)
    : QObject(parent)
{}

TextFormatter* TextFormatter::create(QQmlEngine*, QJSEngine*)
{
    return new TextFormatter();
}

// ── Internal helpers ──────────────────────────────────────────────────────────

static QTextCursor cursorForRange(QQuickTextDocument* doc, int selStart, int selEnd)
{
    QTextCursor cursor(doc->textDocument());
    cursor.setPosition(selStart);
    if (selEnd > selStart)
        cursor.setPosition(selEnd, QTextCursor::KeepAnchor);
    return cursor;
}

// Returns true if every character in the selection already has the property set.
// When there is no selection (selStart == selEnd), checks the cursor char format.
static bool isUniformBold(QQuickTextDocument* doc, int selStart, int selEnd)
{
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    if (selStart == selEnd)
        return cursor.charFormat().fontWeight() >= QFont::Bold;

    // Walk every block that overlaps the selection
    QTextCursor probe(doc->textDocument());
    for (int pos = selStart; pos < selEnd; ++pos) {
        probe.setPosition(pos);
        if (probe.charFormat().fontWeight() < QFont::Bold)
            return false;
    }
    return true;
}

static bool isUniformItalic(QQuickTextDocument* doc, int selStart, int selEnd)
{
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    if (selStart == selEnd)
        return cursor.charFormat().fontItalic();

    QTextCursor probe(doc->textDocument());
    for (int pos = selStart; pos < selEnd; ++pos) {
        probe.setPosition(pos);
        if (!probe.charFormat().fontItalic())
            return false;
    }
    return true;
}

static bool isUniformUnderline(QQuickTextDocument* doc, int selStart, int selEnd)
{
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    if (selStart == selEnd)
        return cursor.charFormat().fontUnderline();

    QTextCursor probe(doc->textDocument());
    for (int pos = selStart; pos < selEnd; ++pos) {
        probe.setPosition(pos);
        if (!probe.charFormat().fontUnderline())
            return false;
    }
    return true;
}

// ── Public slots ──────────────────────────────────────────────────────────────

void TextFormatter::toggleBold(QQuickTextDocument* doc, int selStart, int selEnd)
{
    if (!doc) return;
    bool allBold = isUniformBold(doc, selStart, selEnd);
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    fmt.setFontWeight(allBold ? QFont::Normal : QFont::Bold);
    cursor.mergeCharFormat(fmt);
}

void TextFormatter::toggleItalic(QQuickTextDocument* doc, int selStart, int selEnd)
{
    if (!doc) return;
    bool allItalic = isUniformItalic(doc, selStart, selEnd);
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    fmt.setFontItalic(!allItalic);
    cursor.mergeCharFormat(fmt);
}

void TextFormatter::toggleUnderline(QQuickTextDocument* doc, int selStart, int selEnd)
{
    if (!doc) return;
    bool allUnderline = isUniformUnderline(doc, selStart, selEnd);
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    fmt.setFontUnderline(!allUnderline);
    cursor.mergeCharFormat(fmt);
}

void TextFormatter::toggleBulletList(QQuickTextDocument* doc, int selStart, int selEnd)
{
    if (!doc) return;

    QTextDocument* tdoc = doc->textDocument();
    QTextCursor cursor(tdoc);
    cursor.setPosition(selStart);
    if (selEnd > selStart)
        cursor.setPosition(selEnd, QTextCursor::KeepAnchor);

    // Check if the first paragraph in the selection is already a bullet list
    QTextBlock firstBlock = tdoc->findBlock(selStart);
    bool isBullet = firstBlock.textList() != nullptr;

    cursor.beginEditBlock();

    if (isBullet) {
        // Remove list formatting — restore Normal block style
        QTextCursor bc(tdoc);
        QTextBlock block = firstBlock;
        while (block.isValid() && block.position() <= selEnd) {
            bc.setPosition(block.position());
            QTextBlockFormat fmt;
            fmt.setIndent(0);
            bc.setBlockFormat(fmt);
            if (block.textList())
                block.textList()->remove(block);
            block = block.next();
        }
    } else {
        // Apply disc bullet list
        QTextListFormat listFmt;
        listFmt.setStyle(QTextListFormat::ListDisc);
        cursor.createList(listFmt);
    }

    cursor.endEditBlock();
}

// ── Font size helpers ─────────────────────────────────────────────────────────

// Returns the point size of the first character in the selection (or cursor position).
static int currentPointSize(QQuickTextDocument* doc, int selStart)
{
    QTextCursor probe(doc->textDocument());
    probe.setPosition(selStart);
    qreal ps = probe.charFormat().fontPointSize();
    // 0 means "inherit from block/document default"; treat as 12pt (desktop default)
    return (ps > 0) ? qRound(ps) : 12;
}

// Expand a zero-length selection to the full current paragraph so that
// increaseFontSize / decreaseFontSize actually modify existing text rather
// than only updating the cursor's insert format.
static void expandToParagraph(QTextDocument* tdoc, int& selStart, int& selEnd)
{
    if (selStart != selEnd) return;
    QTextBlock block = tdoc->findBlock(selStart);
    selStart = block.position();
    int blockEnd = block.position() + block.length() - 1;
    selEnd = (blockEnd > selStart) ? blockEnd : selStart;
}

void TextFormatter::increaseFontSize(QQuickTextDocument* doc, int selStart, int selEnd)
{
    if (!doc) return;
    expandToParagraph(doc->textDocument(), selStart, selEnd);
    int newSize = currentPointSize(doc, selStart) + 2;
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    fmt.setFontPointSize(newSize);
    cursor.mergeCharFormat(fmt);
}

void TextFormatter::decreaseFontSize(QQuickTextDocument* doc, int selStart, int selEnd)
{
    if (!doc) return;
    expandToParagraph(doc->textDocument(), selStart, selEnd);
    int newSize = qMax(6, currentPointSize(doc, selStart) - 2);
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    fmt.setFontPointSize(newSize);
    cursor.mergeCharFormat(fmt);
}

// ── Heading / paragraph style ─────────────────────────────────────────────────

void TextFormatter::applyHeading(QQuickTextDocument* doc, int selStart, int selEnd, int level)
{
    if (!doc) return;

    // Point sizes for H1–H6 and body (level 0)
    static const int sizes[] = { 12, 24, 20, 16, 14, 13, 12 };  // [0]=body, [1]=H1 … [6]=H6
    int clampedLevel = qBound(0, level, 6);
    int pointSize    = sizes[clampedLevel];
    bool bold        = (clampedLevel > 0);

    QTextDocument* tdoc = doc->textDocument();
    QTextCursor cursor(tdoc);
    cursor.setPosition(selStart);
    if (selEnd > selStart)
        cursor.setPosition(selEnd, QTextCursor::KeepAnchor);

    cursor.beginEditBlock();

    // Walk every block that overlaps the selection and apply format to the whole block
    QTextBlock block = tdoc->findBlock(selStart);
    int endBlock = tdoc->findBlock(selEnd > selStart ? selEnd - 1 : selStart).blockNumber();

    while (block.isValid() && block.blockNumber() <= endBlock) {
        QTextCursor bc(tdoc);
        bc.setPosition(block.position());
        bc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

        QTextCharFormat charFmt;
        charFmt.setFontPointSize(pointSize);
        charFmt.setFontWeight(bold ? QFont::Bold : QFont::Normal);
        bc.mergeCharFormat(charFmt);

        block = block.next();
    }

    cursor.endEditBlock();
}

// ── Unified paragraph/list style (matches desktop combo) ─────────────────────
// Index mapping:
//  0=Standard  1=Disc  2=Circle  3=Square
//  4=Decimal   5=AlphaLower  6=AlphaUpper  7=RomanLower  8=RomanUpper
//  9..14=Heading 1..6

void TextFormatter::applyStyle(QQuickTextDocument* doc, int selStart, int selEnd, int styleIndex)
{
    if (!doc) return;

    // Heading styles delegate to applyHeading
    if (styleIndex >= 9) {
        applyHeading(doc, selStart, selEnd, styleIndex - 8);  // 9→H1, 10→H2, …, 14→H6
        return;
    }

    QTextDocument* tdoc = doc->textDocument();
    QTextCursor cursor(tdoc);
    cursor.setPosition(selStart);
    if (selEnd > selStart)
        cursor.setPosition(selEnd, QTextCursor::KeepAnchor);

    cursor.beginEditBlock();

    QTextBlock block = tdoc->findBlock(selStart);
    int endBlockNum  = tdoc->findBlock(selEnd > selStart ? selEnd - 1 : selStart).blockNumber();

    if (styleIndex == 0) {
        // Standard: remove list formatting and reset to body character style
        while (block.isValid() && block.blockNumber() <= endBlockNum) {
            QTextCursor bc(tdoc);
            bc.setPosition(block.position());

            // Remove from any list
            if (block.textList())
                block.textList()->remove(block);

            // Reset block format (clears indent)
            QTextBlockFormat bfmt;
            bc.setBlockFormat(bfmt);

            // Reset char format to body (12pt, normal)
            bc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            QTextCharFormat cfmt;
            cfmt.setFontPointSize(12);
            cfmt.setFontWeight(QFont::Normal);
            bc.mergeCharFormat(cfmt);

            block = block.next();
        }
    } else {
        // List styles
        static const QTextListFormat::Style listStyles[] = {
            QTextListFormat::ListDisc,        // 1
            QTextListFormat::ListCircle,      // 2
            QTextListFormat::ListSquare,      // 3
            QTextListFormat::ListDecimal,     // 4
            QTextListFormat::ListLowerAlpha,  // 5
            QTextListFormat::ListUpperAlpha,  // 6
            QTextListFormat::ListLowerRoman,  // 7
            QTextListFormat::ListUpperRoman,  // 8
        };
        QTextListFormat listFmt;
        listFmt.setStyle(listStyles[styleIndex - 1]);
        cursor.createList(listFmt);
    }

    cursor.endEditBlock();
}

// ── Indent / unindent ─────────────────────────────────────────────────────────

static void adjustBlockIndent(QQuickTextDocument* doc, int selStart, int selEnd, int delta)
{
    QTextDocument* tdoc = doc->textDocument();
    QTextCursor cursor(tdoc);
    cursor.beginEditBlock();

    QTextBlock block = tdoc->findBlock(selStart);
    int endBlockNum  = tdoc->findBlock(selEnd > selStart ? selEnd - 1 : selStart).blockNumber();

    while (block.isValid() && block.blockNumber() <= endBlockNum) {
        QTextCursor bc(tdoc);
        bc.setPosition(block.position());
        QTextBlockFormat fmt = bc.blockFormat();
        fmt.setIndent(qMax(0, fmt.indent() + delta));
        bc.setBlockFormat(fmt);
        block = block.next();
    }

    cursor.endEditBlock();
}

void TextFormatter::indentText(QQuickTextDocument* doc, int selStart, int selEnd)
{
    if (!doc) return;
    adjustBlockIndent(doc, selStart, selEnd, +1);
}

void TextFormatter::unindentText(QQuickTextDocument* doc, int selStart, int selEnd)
{
    if (!doc) return;
    adjustBlockIndent(doc, selStart, selEnd, -1);
}

// ── Alignment ─────────────────────────────────────────────────────────────────

void TextFormatter::setAlignment(QQuickTextDocument* doc, int selStart, int selEnd, int alignment)
{
    if (!doc) return;

    static const Qt::Alignment alignments[] = {
        Qt::AlignLeft, Qt::AlignHCenter, Qt::AlignRight, Qt::AlignJustify
    };
    Qt::Alignment align = alignments[qBound(0, alignment, 3)];

    QTextDocument* tdoc = doc->textDocument();
    QTextCursor cursor(tdoc);
    cursor.beginEditBlock();

    QTextBlock block = tdoc->findBlock(selStart);
    int endBlockNum  = tdoc->findBlock(selEnd > selStart ? selEnd - 1 : selStart).blockNumber();

    while (block.isValid() && block.blockNumber() <= endBlockNum) {
        QTextCursor bc(tdoc);
        bc.setPosition(block.position());
        QTextBlockFormat fmt = bc.blockFormat();
        fmt.setAlignment(align);
        bc.setBlockFormat(fmt);
        block = block.next();
    }

    cursor.endEditBlock();
}

// ── Font family / size / color setters ────────────────────────────────────────

void TextFormatter::applyFontFamily(QQuickTextDocument* doc, int selStart, int selEnd,
                                    const QString& family)
{
    if (!doc || family.isEmpty()) return;
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    fmt.setFontFamilies({family});
    cursor.mergeCharFormat(fmt);
}

void TextFormatter::applyFontPointSize(QQuickTextDocument* doc, int selStart, int selEnd,
                                       int pointSize)
{
    if (!doc || pointSize <= 0) return;
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    fmt.setFontPointSize(pointSize);
    cursor.mergeCharFormat(fmt);
}

void TextFormatter::applyFontColor(QQuickTextDocument* doc, int selStart, int selEnd,
                                   const QColor& color)
{
    if (!doc || !color.isValid()) return;
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    fmt.setForeground(color);
    cursor.mergeCharFormat(fmt);
}

// ── Font format getters (pre-populate the dialog) ─────────────────────────────

QString TextFormatter::currentFontFamily(QQuickTextDocument* doc, int pos) const
{
    if (!doc) return QStringLiteral("Arial");
    QTextDocument* tdoc = doc->textDocument();
    QTextCursor probe(tdoc);
    probe.setPosition(qBound(0, pos, tdoc->characterCount() - 1));
    QStringList families = probe.charFormat().fontFamilies().toStringList();
    if (!families.isEmpty()) return families.first();
    return QStringLiteral("Arial");
}

int TextFormatter::currentFontPointSize(QQuickTextDocument* doc, int pos) const
{
    if (!doc) return 12;
    QTextDocument* tdoc = doc->textDocument();
    QTextCursor probe(tdoc);
    probe.setPosition(qBound(0, pos, tdoc->characterCount() - 1));
    qreal ps = probe.charFormat().fontPointSize();
    return (ps > 0) ? qRound(ps) : 12;
}

QColor TextFormatter::currentFontColor(QQuickTextDocument* doc, int pos) const
{
    if (!doc) return Qt::black;
    QTextDocument* tdoc = doc->textDocument();
    QTextCursor probe(tdoc);
    probe.setPosition(qBound(0, pos, tdoc->characterCount() - 1));
    QBrush brush = probe.charFormat().foreground();
    return (brush.style() == Qt::NoBrush) ? Qt::black : brush.color();
}

QStringList TextFormatter::availableFontFamilies() const
{
    return QFontDatabase::families();
}
