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
#include <QTextTable>
#include <QTextTableFormat>
#include <QTextLength>
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
//
// Note on the loop bounds: QTextCursor::charFormat() returns the format of the
// character IMMEDIATELY BEFORE the cursor position (or after, when the cursor
// is at the start of a block). So to inspect the character at index i, the
// cursor must sit at position i + 1. To cover the selected characters
// [selStart, selEnd), we walk positions [selStart + 1, selEnd].
static bool isUniformBold(QQuickTextDocument* doc, int selStart, int selEnd)
{
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    if (selStart == selEnd)
        return cursor.charFormat().fontWeight() >= QFont::Bold;

    QTextCursor probe(doc->textDocument());
    for (int pos = selStart + 1; pos <= selEnd; ++pos) {
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
    for (int pos = selStart + 1; pos <= selEnd; ++pos) {
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
    for (int pos = selStart + 1; pos <= selEnd; ++pos) {
        probe.setPosition(pos);
        if (!probe.charFormat().fontUnderline())
            return false;
    }
    return true;
}

static bool isUniformStrikethrough(QQuickTextDocument* doc, int selStart, int selEnd)
{
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    if (selStart == selEnd)
        return cursor.charFormat().fontStrikeOut();

    QTextCursor probe(doc->textDocument());
    for (int pos = selStart + 1; pos <= selEnd; ++pos) {
        probe.setPosition(pos);
        if (!probe.charFormat().fontStrikeOut())
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
    // Drive TextUnderlineStyle (the enum the renderer honors) rather than only the
    // FontUnderline bool, so this still wins when a char already carries an explicit
    // underline-style property. Note: the visible "underline does nothing" bug was
    // NOT here — it was the spell-check highlighter painting SpellCheckUnderline
    // (invisible in Qt Quick) over flagged words, which masked the user's underline.
    fmt.setUnderlineStyle(allUnderline ? QTextCharFormat::NoUnderline
                                       : QTextCharFormat::SingleUnderline);
    cursor.mergeCharFormat(fmt);
}

void TextFormatter::toggleStrikethrough(QQuickTextDocument* doc, int selStart, int selEnd)
{
    if (!doc) return;
    bool allStrike = isUniformStrikethrough(doc, selStart, selEnd);
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(!allStrike);
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
    // 0 means "inherit from block/document default"
    if (ps <= 0) ps = doc->textDocument()->defaultFont().pointSizeF();
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
    cursor.setCharFormat(fmt);
    cursor.clearSelection();
    cursor.setPosition(selStart);
    cursor.setPosition(selEnd, QTextCursor::KeepAnchor);
}

void TextFormatter::decreaseFontSize(QQuickTextDocument* doc, int selStart, int selEnd)
{
    if (!doc) return;
    expandToParagraph(doc->textDocument(), selStart, selEnd);
    int newSize = qMax(6, currentPointSize(doc, selStart) - 2);
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    fmt.setFontPointSize(newSize);
    cursor.setCharFormat(fmt);
    cursor.clearSelection();
    cursor.setPosition(selStart);
    cursor.setPosition(selEnd, QTextCursor::KeepAnchor);
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

void TextFormatter::insertTable(QQuickTextDocument* doc, int pos, int rows, int cols)
{
    if (!doc || rows < 1 || cols < 1) return;

    QTextDocument* tdoc = doc->textDocument();
    QTextCursor cursor(tdoc);
    cursor.setPosition(qBound(0, pos, tdoc->characterCount() - 1));

    QTextTableFormat fmt;
    fmt.setCellPadding(4);
    fmt.setCellSpacing(0);
    fmt.setBorder(1);
    fmt.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    fmt.setWidth(QTextLength(QTextLength::PercentageLength, 100));
    // Equal-width columns so the table fills the editor evenly.
    QVector<QTextLength> constraints(cols, QTextLength(QTextLength::PercentageLength, 100.0 / cols));
    fmt.setColumnWidthConstraints(constraints);

    cursor.insertTable(rows, cols, fmt);
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
    if (!doc) return;
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    if (color.isValid() && color.alpha() > 0)
        fmt.setForeground(color);
    else
        fmt.setForeground(QBrush(Qt::NoBrush));
    cursor.mergeCharFormat(fmt);
}

void TextFormatter::applyFontHighlight(QQuickTextDocument* doc, int selStart, int selEnd,
                                       const QColor& color)
{
    if (!doc) return;
    QTextCursor cursor = cursorForRange(doc, selStart, selEnd);
    QTextCharFormat fmt;
    // An invalid or fully transparent color means "remove highlight": merge a
    // NoBrush background so any existing highlight is cleared.
    if (color.isValid() && color.alpha() > 0)
        fmt.setBackground(color);
    else
        fmt.setBackground(QBrush(Qt::NoBrush));
    cursor.mergeCharFormat(fmt);
}

// ── Font format getters (pre-populate the dialog) ─────────────────────────────

QString TextFormatter::currentFontFamily(QQuickTextDocument* doc, int pos) const
{
    // Apple platforms expose hidden system fonts whose names begin with a dot
    // (e.g. ".AppleSystemUIFont"). Qt's TextEdit.toHtml() bakes the resolved
    // system font into saved HTML, so reloaded text carries those names as the
    // "explicit" family even though the user never picked them. Treat any
    // dotted family as unset and fall through to the document's defaultFont.
    auto isHiddenSystemFont = [](const QString& f) {
        return f.startsWith(QLatin1Char('.'));
    };

    if (!doc) return QStringLiteral("Arial");
    QTextDocument* tdoc = doc->textDocument();
    QTextCursor probe(tdoc);
    probe.setPosition(qBound(0, pos, tdoc->characterCount() - 1));
    QTextCharFormat fmt = probe.charFormat();

    const QStringList families = fmt.fontFamilies().toStringList();
    for (const QString& f : families) {
        if (!f.isEmpty() && !isHiddenSystemFont(f)) return f;
    }
    // Deliberately NOT falling back to the deprecated QTextFormat::fontFamily()
    // (singular) accessor here. It reads from the same underlying "families"
    // property as fontFamilies() above, so it can't tell us anything new — but
    // on at least one real machine (DirectWrite failing to initialize at all:
    // DWriteCreateFactory itself throwing), Qt's own implementation of that
    // deprecated getter called QList::first() on the same, empty families list
    // with no emptiness check, hitting Q_ASSERT(!isEmpty()) in qlist.h and
    // aborting the process. Skip straight to the document's default font.
    QString def = tdoc->defaultFont().family();
    if (!def.isEmpty() && !isHiddenSystemFont(def)) return def;
    return QStringLiteral("Arial");
}

int TextFormatter::currentFontPointSize(QQuickTextDocument* doc, int pos) const
{
    if (!doc) return 12;
    QTextDocument* tdoc = doc->textDocument();
    QTextCursor probe(tdoc);
    probe.setPosition(qBound(0, pos, tdoc->characterCount() - 1));
    qreal ps = probe.charFormat().fontPointSize();
    if (ps <= 0) ps = tdoc->defaultFont().pointSizeF();
    return (ps > 0) ? qRound(ps) : 12;
}

QColor TextFormatter::currentFontColor(QQuickTextDocument* doc, int pos) const
{
    // HTML loaded without explicit "color:" styles has no foreground brush in
    // the char format (style() == Qt::NoBrush). Qt's TextEdit then renders that
    // text using its own "color" property (e.g. palette.text). Mirror that here
    // so the picker reflects what the user actually sees, not a hard-coded black.
    if (!doc) return Qt::black;
    QTextDocument* tdoc = doc->textDocument();
    QTextCursor probe(tdoc);
    probe.setPosition(qBound(0, pos, tdoc->characterCount() - 1));
    QBrush brush = probe.charFormat().foreground();
    if (brush.style() != Qt::NoBrush) return brush.color();

    if (QObject* p = doc->parent()) {
        QVariant v = p->property("color");
        if (v.isValid()) {
            QColor c = v.value<QColor>();
            if (c.isValid()) return c;
        }
    }
    return Qt::black;
}

QColor TextFormatter::currentFontHighlight(QQuickTextDocument* doc, int pos) const
{
    // No highlight applied → return an invalid color so the picker shows "None".
    if (!doc) return QColor();
    QTextDocument* tdoc = doc->textDocument();
    QTextCursor probe(tdoc);
    probe.setPosition(qBound(0, pos, tdoc->characterCount() - 1));
    QBrush brush = probe.charFormat().background();
    if (brush.style() != Qt::NoBrush) return brush.color();
    return QColor();
}

bool TextFormatter::hasFontHighlightAt(QQuickTextDocument* doc, int pos) const
{
    if (!doc) return false;
    QTextDocument* tdoc = doc->textDocument();
    QTextCursor probe(tdoc);
    probe.setPosition(qBound(0, pos, tdoc->characterCount() - 1));
    const QBrush brush = probe.charFormat().background();
    return brush.style() != Qt::NoBrush && brush.color().alpha() > 0;
}

// ── Format-state queries (used by the format sheet to show active options) ──

bool TextFormatter::isBoldAt(QQuickTextDocument* doc, int selStart, int selEnd) const
{
    if (!doc) return false;
    return isUniformBold(doc, selStart, selEnd);
}

bool TextFormatter::isItalicAt(QQuickTextDocument* doc, int selStart, int selEnd) const
{
    if (!doc) return false;
    return isUniformItalic(doc, selStart, selEnd);
}

bool TextFormatter::isUnderlineAt(QQuickTextDocument* doc, int selStart, int selEnd) const
{
    if (!doc) return false;
    return isUniformUnderline(doc, selStart, selEnd);
}

bool TextFormatter::isStrikethroughAt(QQuickTextDocument* doc, int selStart, int selEnd) const
{
    if (!doc) return false;
    return isUniformStrikethrough(doc, selStart, selEnd);
}

int TextFormatter::currentAlignment(QQuickTextDocument* doc, int pos) const
{
    if (!doc) return 0;
    QTextDocument* tdoc = doc->textDocument();
    QTextBlock block = tdoc->findBlock(qBound(0, pos, tdoc->characterCount() - 1));
    Qt::Alignment a = block.blockFormat().alignment();
    if (a & Qt::AlignHCenter) return 1;
    if (a & Qt::AlignRight)   return 2;
    if (a & Qt::AlignJustify) return 3;
    return 0;
}

int TextFormatter::currentListStyle(QQuickTextDocument* doc, int pos) const
{
    if (!doc) return -1;
    QTextDocument* tdoc = doc->textDocument();
    QTextBlock block = tdoc->findBlock(qBound(0, pos, tdoc->characterCount() - 1));
    QTextList* list = block.textList();
    if (!list) return -1;

    switch (list->format().style()) {
        case QTextListFormat::ListDisc:       return 1;
        case QTextListFormat::ListCircle:     return 2;
        case QTextListFormat::ListSquare:     return 3;
        case QTextListFormat::ListDecimal:    return 4;
        case QTextListFormat::ListLowerAlpha: return 5;
        case QTextListFormat::ListUpperAlpha: return 6;
        case QTextListFormat::ListLowerRoman: return 7;
        case QTextListFormat::ListUpperRoman: return 8;
        default:                              return -1;
    }
}

int TextFormatter::currentParagraphStyle(QQuickTextDocument* doc, int pos) const
{
    // Style indices and sizes match applyStyle/applyHeading exactly:
    // 0=Body (12pt normal), 9..14=Heading 1..6 (24/20/16/14/13/12pt bold).
    // Detection uses the first character so an inline emphasis run elsewhere
    // in the paragraph does not change the paragraph-style indicator.
    if (!doc) return -1;
    QTextDocument* tdoc = doc->textDocument();
    QTextBlock block = tdoc->findBlock(qBound(0, pos, tdoc->characterCount() - 1));
    if (!block.isValid()) return -1;

    QTextCursor probe(tdoc);
    probe.setPosition(block.position());
    QTextCharFormat fmt = probe.charFormat();

    qreal ps = fmt.fontPointSize();
    if (ps <= 0) ps = tdoc->defaultFont().pointSizeF();
    int size = qRound(ps);
    bool bold = fmt.fontWeight() >= QFont::Bold;

    if (bold && size == 24) return 9;
    if (bold && size == 20) return 10;
    if (bold && size == 16) return 11;
    if (bold && size == 14) return 12;
    if (bold && size == 13) return 13;
    if (bold && size == 12) return 14;
    if (!bold && size == 12) return 0;
    return -1;
}

QString TextFormatter::documentHtml(QQuickTextDocument* doc) const
{
    return doc ? doc->textDocument()->toHtml() : QString();
}

QStringList TextFormatter::availableFontFamilies() const
{
    // Filter out Apple's hidden system fonts (".AppleSystemUIFont", etc.)
    // — they aren't user-pickable typefaces and only confuse the picker.
    QStringList out;
    const QStringList all = QFontDatabase::families();
    out.reserve(all.size());
    for (const QString& f : all) {
        if (!f.startsWith(QLatin1Char('.')))
            out.append(f);
    }
    return out;
}
