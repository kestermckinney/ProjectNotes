// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import ProjectNotesDesktop

ListView {
    id: view
    activeFocusOnTab: true
    focus: true
    keyNavigationEnabled: false
    keyNavigationWraps: false
    highlightMoveDuration: 0
    // Consumers with guarded selection (settings, filters) can accept or reject
    // a destination without breaking their currentIndex binding.
    property var selectIndex: null
    property bool focusOnTap: true

    highlight: Rectangle {
        z: 2
        color: "transparent"
        border.color: Theme.accent
        border.width: 2
        radius: Theme.radiusSm
        visible: view.activeFocus
    }

    function selectRow(row) {
        if (count === 0) return
        row = Math.max(0, Math.min(count - 1, row))
        if (selectIndex) {
            if (selectIndex(row) === false) return
        } else {
            currentIndex = row
        }
        positionViewAtIndex(row, ListView.Contain)
    }

    function moveRows(direction) {
        var row = currentIndex < 0 ? (direction > 0 ? 0 : count - 1)
                                   : currentIndex + direction
        // Some search lists collapse nonmatching delegates to zero height.
        while (row >= 0 && row < count) {
            var item = itemAtIndex(row)
            if (!item || (item.visible && item.height > 0)) break
            row += direction
        }
        if (row >= 0 && row < count) selectRow(row)
    }

    function movePage(direction) {
        if (count === 0) return
        if (currentIndex < 0 || !currentItem) {
            moveRows(direction)
            return
        }
        var start = currentItem.y + currentItem.height / 2
        var distance = Math.max(currentItem.height + spacing, height - currentItem.height)
        // Bring the destination into the instantiated range before hit testing;
        // a large page can otherwise extend beyond ListView's cache buffer.
        contentY = Math.max(originY, Math.min(Math.max(originY, originY + contentHeight - height),
                                             contentY + direction * distance))
        forceLayout()
        start = currentItem.y + currentItem.height / 2
        var target = Math.max(originY, Math.min(originY + contentHeight - 1,
                                                start + direction * distance))
        var row = -1
        // Probe backwards through section headers and gaps to the nearest row.
        for (var y = target; direction * (y - start) > 0; y -= direction) {
            row = indexAt(width / 2, y)
            if (row >= 0) break
        }
        if (row < 0 || row === currentIndex) moveRows(direction)
        else selectRow(row)
    }

    function activateCurrent() {
        if (count === 0) return false
        if (currentIndex < 0) selectRow(0)
        forceLayout()
        if (!currentItem || !currentItem.visible
                || typeof currentItem.openRecord !== "function") return false
        currentItem.openRecord()
        return true
    }

    Keys.priority: Keys.AfterItem
    Keys.onPressed: (event) => {
        if (event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)) return
        switch (event.key) {
        case Qt.Key_Up: moveRows(-1); break
        case Qt.Key_Down: moveRows(1); break
        case Qt.Key_PageUp: movePage(-1); break
        case Qt.Key_PageDown: movePage(1); break
        case Qt.Key_Return:
        case Qt.Key_Enter:
            if (event.isAutoRepeat || !activateCurrent()) return
            break
        default: return
        }
        event.accepted = true
    }

    TapHandler {
        onTapped: (event) => {
            var position = view.contentItem.mapFromItem(null,
                                                        event.scenePosition.x, event.scenePosition.y)
            var row = view.indexAt(position.x, position.y)
            if (row >= 0) view.selectRow(row)
            // Preserve focus in a delegate's editor or button.
            var focused = view.Window.window ? view.Window.window.activeFocusItem : null
            for (var item = focused; item; item = item.parent) {
                if (item === view) return
            }
            if (view.focusOnTap) view.forceActiveFocus(Qt.MouseFocusReason)
        }
    }
}
