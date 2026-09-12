// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Right-click menu for a read-only text view (LogViewerWindow's log tabs, or
// any other TextArea/TextEdit that only ever displays text): Copy and Select
// All, styled to match RecordContextMenu/SpellCheckField instead of the
// platform's native edit-menu popup that a read-only Qt Quick TextArea shows
// by default on right-click.
//
// Usage — place inside the target TextArea so it overlays it and intercepts
// the right-click before the native menu does:
//     TextArea { id: logText; readOnly: true; ReadOnlyTextContextMenu { } }
Item {
    id: root
    anchors.fill: parent

    // The editor to act on. Defaults to the parent (the usual placement).
    property Item target: parent

    property bool _hasSelection: false

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (ep) => {
            if (!root.target)
                return
            root._hasSelection = root.target.selectedText.length > 0
            menu.openAt(ep.scenePosition.x, ep.scenePosition.y)
        }
    }

    Popup {
        id: menu
        modal: true
        dim: false
        padding: 3
        width: 176
        // Deliberately an in-scene popup (default Popup.Item) — popupType:
        // Popup.Window (Qt 6.10) forwards clicks on the rows to the main window
        // underneath instead of applying the action — see SpellCheckField.qml.
        parent: Overlay.overlay
        scale: Theme.uiScale
        transformOrigin: Item.TopLeft

        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border
        }

        // Clicking away dismisses the menu and nothing else — see ClickShield.qml.
        ClickShield { host: menu }

        function openAt(sx, sy) {
            var maxX = (parent ? parent.width : sx + width) - width - 6
            var maxY = (parent ? parent.height : sy + 80) - 60
            x = Math.max(6, Math.min(sx, maxX))
            y = Math.max(6, Math.min(sy, maxY))
            open()
        }
        onHeightChanged: if (visible && parent) y = Math.max(6, Math.min(y, parent.height - height - 6))

        contentItem: ColumnLayout {
            spacing: 0
            MenuRow {
                icon: "content_copy"; label: qsTr("Copy")
                enabled: root._hasSelection
                opacity: enabled ? 1.0 : 0.45
                onActivated: { menu.close(); root.target.copy() }
            }
            MenuRow {
                icon: "select_all"; label: qsTr("Select All")
                onActivated: { menu.close(); root.target.selectAll() }
            }
        }
    }
}
