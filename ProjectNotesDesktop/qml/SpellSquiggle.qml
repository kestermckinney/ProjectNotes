// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick

// Red wavy underline overlay. Qt Quick's text node renders neither a wavy nor a
// coloured underline via QTextCharFormat, so the squiggle is drawn here on a
// Canvas that overlays the editor. It underlines every misspelled range reported
// by `spell`, mapping document positions to pixels with positionToRectangle().
//
// Purely decorative: it accepts no input, so text selection/caret still work.
Canvas {
    id: canvas
    anchors.fill: parent

    // The SpellCheck object (source of ranges) and the editor being underlined.
    property var  spell:  null
    property Item editor: parent
    property color squiggleColor: "#c0442e"

    readonly property real _amp: 1.3      // wave height (px)
    readonly property real _period: 4     // px per full zig-zag

    // A single-line TextField (TextInput — no `textDocument`) reports
    // positionToRectangle x WITHOUT its leftPadding, whereas a TextArea (TextEdit)
    // includes it. Shift TextField x by leftPadding so the wave sits under the
    // glyphs, not in the left padding.
    readonly property real _xShift:
        (editor && editor.textDocument === undefined) ? (editor.leftPadding || 0) : 0

    // Repaint whenever the text, geometry, scroll, caret, or dictionary changes.
    Connections {
        target: canvas.editor
        ignoreUnknownSignals: true
        function onTextChanged()          { canvas.requestPaint() }
        function onContentHeightChanged() { canvas.requestPaint() }
        function onContentYChanged()      { canvas.requestPaint() }
        function onContentXChanged()      { canvas.requestPaint() }
        function onCursorRectangleChanged() { canvas.requestPaint() }
    }
    Connections {
        target: canvas.spell
        ignoreUnknownSignals: true
        function onSpellingChanged() { canvas.requestPaint() }
    }
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    Component.onCompleted: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        if (!spell || !editor)
            return
        var ranges = spell.misspelledRanges()
        if (!ranges || ranges.length === 0)
            return

        ctx.strokeStyle = squiggleColor
        ctx.lineWidth = 1.3

        for (var i = 0; i < ranges.length; i++) {
            var start = ranges[i].start
            var end   = start + ranges[i].length
            var r1 = editor.positionToRectangle(start)
            var r2 = editor.positionToRectangle(end)

            if (Math.abs(r1.y - r2.y) < 0.5) {
                // Single line (the common case, and always true for TextField).
                _wave(ctx, r1.x, r2.x, _baseline(r1))
            } else {
                // The word wraps: split the squiggle at each line break by walking
                // the character positions and drawing one segment per line.
                var lineY = r1.y
                var lineStartX = r1.x
                for (var p = start + 1; p <= end; p++) {
                    var rp = editor.positionToRectangle(p)
                    if (Math.abs(rp.y - lineY) > 0.5) {
                        var rPrev = editor.positionToRectangle(p - 1)
                        _wave(ctx, lineStartX, rPrev.x, rPrev.y + rPrev.height - 1)
                        lineY = rp.y
                        lineStartX = rp.x
                    }
                }
                _wave(ctx, lineStartX, r2.x, r2.y + r2.height - 1)
            }
        }
    }

    // Underline baseline for a line rect. positionToRectangle reports the line at
    // the top of the content area and IGNORES vertical centering, so for a
    // vertically-centred single-line field (e.g. FormField's 34px TextField) the
    // glyphs actually sit lower — derive the baseline from the field's centre.
    function _baseline(r) {
        if (editor && editor.verticalAlignment === TextEdit.AlignVCenter)
            return editor.height / 2 + r.height / 2 - 1
        return r.y + r.height - 1
    }

    // Draw a zig-zag (the classic spell squiggle) between x1..x2 at baseline y.
    function _wave(ctx, x1, x2, y) {
        x1 += _xShift
        x2 += _xShift
        if (x2 <= x1)
            return
        ctx.beginPath()
        ctx.moveTo(x1, y)
        var up = true
        for (var x = x1; x <= x2; x += _period / 2) {
            ctx.lineTo(x, y + (up ? -_amp : _amp))
            up = !up
        }
        ctx.stroke()
    }
}
