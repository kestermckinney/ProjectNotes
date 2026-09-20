// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick

// Keeps the caret on screen while typing in an editor that grows with its own
// content inside a scrolling page. Such an editor never scrolls internally, so
// once it is taller than the page viewport the line being typed slides under
// the bottom edge and the text looks cut off. Place this inside the editor,
// the same way SpellCheckField is placed:
//
//   TextArea {
//       id: body
//       CaretFollow {}
//   }
//
// It is inert when the editor has no scrolling ancestor.
Item {
    id: root

    // The editor to watch. Defaults to the parent (the usual inline placement).
    property Item target: parent
    // Breathing room kept above and below the caret.
    property int margin: 12

    width: 0
    height: 0

    property Flickable _flick: null

    function _isFlickable(item) {
        return item !== null && item.contentY !== undefined
               && item.contentHeight !== undefined
               && item.flickableDirection !== undefined
    }

    // ScrollView reparents its children into its internal Flickable, so the
    // scrolling ancestor is found by walking up rather than by name. The result
    // is stored rather than returned from a bound expression, so nothing binds
    // to a function that writes the property it reads.
    function _resolveFlickable() {
        if (root._flick !== null)
            return root._flick
        var item = root.target
        while (item !== null) {
            if (root._isFlickable(item)) {
                root._flick = item
                break
            }
            item = item.parent
        }
        return root._flick
    }

    Component.onCompleted: root._resolveFlickable()

    function ensureVisible() {
        var ed = root.target
        if (!ed || !ed.activeFocus)
            return
        var flick = root._resolveFlickable()
        if (!flick || flick.height <= 0)
            return

        var r = ed.cursorRectangle
        var p = ed.mapToItem(flick.contentItem, r.x, r.y)
        var top = p.y - root.margin
        var bottom = p.y + r.height + root.margin
        var maxY = Math.max(0, flick.contentHeight - flick.height)

        if (bottom > flick.contentY + flick.height)
            flick.contentY = Math.min(maxY, bottom - flick.height)
        else if (top < flick.contentY)
            flick.contentY = Math.max(0, Math.min(maxY, top))
    }

    Connections {
        target: root.target
        function onCursorRectangleChanged() { Qt.callLater(root.ensureVisible) }
        function onActiveFocusChanged() { Qt.callLater(root.ensureVisible) }
    }

    // A new line grows the editor, which grows the page, which only then gives
    // the Flickable room to scroll. Reacting to that growth as well as to the
    // caret move keeps the two orderings from leaving the caret off screen.
    Connections {
        target: root._flick
        function onContentHeightChanged() { Qt.callLater(root.ensureVisible) }
    }
}
