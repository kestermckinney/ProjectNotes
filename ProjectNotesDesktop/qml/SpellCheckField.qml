// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Drop-in inline spell-check for any TextArea/TextField. Place it *inside* the
// editor. It draws a red squiggle under unrecognized words (via SpellSquiggle,
// since Qt Quick won't render a wavy/coloured underline itself) and adds a
// right-click menu — styled to match RecordContextMenu — with suggestions,
// "Add to Dictionary", and "Check Spelling…" (which opens the shared full-field
// SpellCheckDialog passed in as `dialog`).
//
//   TextArea {
//       id: body
//       SpellCheckField { dialog: spellDialog }
//   }
Item {
    id: root
    anchors.fill: parent

    // The editor to check. Defaults to the parent (the usual inline placement).
    property Item target: parent
    // Shared full-field dialog; if null the "Check Spelling…" item is hidden.
    property var  dialog: null
    // Exposed so a toolbar button can open the dialog for this same field.
    property alias spell: spell

    // Right-click menu state.
    property int    _pos: -1
    property string _word: ""
    property bool   _bad: false
    property var    _suggestions: []
    property bool   _hasSelection: false
    property bool   _canPaste: false
    property bool   _canUndo: false
    property bool   _canRedo: false

    // Paste the clipboard's plain text at the cursor, discarding any rich
    // formatting — mirrors TextEdit::slotPasteUnformated() in the Widgets app.
    // Replaces the current selection, if any, same as a normal paste() would.
    function _pasteUnformatted() {
        var t = root.target
        if (!t) return
        var s = t.selectionStart, e = t.selectionEnd
        var txt = DesktopAppController.clipboardPlainText()
        if (e > s) t.remove(s, e)
        t.insert(s, txt)
        t.cursorPosition = s + txt.length
    }

    SpellCheck {
        id: spell
        editor: root.target
        enabled: true
    }

    // The visible red squiggle (Qt Quick won't draw it via QTextCharFormat).
    SpellSquiggle {
        anchors.fill: parent
        spell: spell
        editor: root.target
    }

    // Right-click: resolve the word under the pointer, then pop the themed menu
    // at the cursor (scene coordinates, like RecordContextMenu).
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (ep) => {
            if (!root.target)
                return
            var pos = root.target.positionAt(ep.position.x, ep.position.y)
            var w = spell.wordAt(pos)
            root._pos = pos
            root._word = w
            root._bad = (w !== "" && spell.isMisspelled(w))
            root._suggestions = root._bad ? spell.suggestionsFor(w).slice(0, 8) : []
            root._hasSelection = root.target.selectedText.length > 0
            root._canPaste = root.target.canPaste === true
            root._canUndo = root.target.canUndo === true
            root._canRedo = root.target.canRedo === true
            menu.openAt(ep.scenePosition.x, ep.scenePosition.y)
        }
    }

    // Themed context menu (mirrors RecordContextMenu: surface popup, header,
    // dividers, hover rows) instead of the default Controls Menu styling.
    Popup {
        id: menu
        modal: true
        dim: false
        padding: 5
        width: 232
        // Deliberately an in-scene popup (default Popup.Item) — popupType:
        // Popup.Window (Qt 6.10) forwards clicks on the rows to the main window
        // underneath instead of applying the suggestion.
        parent: Overlay.overlay
        scale: Theme.uiScale
        transformOrigin: Item.TopLeft

        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border
        }

        function openAt(sx, sy) {
            var maxX = (parent ? parent.width : sx + width) - width - 6
            var maxY = (parent ? parent.height : sy + 280) - 60
            x = Math.max(6, Math.min(sx, maxX))
            y = Math.max(6, Math.min(sy, maxY))
            open()
        }
        onHeightChanged: if (visible && parent) y = Math.max(6, Math.min(y, parent.height - height - 6))

        contentItem: ColumnLayout {
            spacing: 0

            // Header — the misspelled word (only when one is under the cursor).
            RowLayout {
                visible: root._bad
                Layout.fillWidth: true
                Layout.leftMargin: 9; Layout.rightMargin: 9
                Layout.topMargin: 2; Layout.bottomMargin: 5
                spacing: 8
                Text {
                    text: qsTr("SPELLING"); color: Theme.text3
                    font.pixelSize: 10; font.weight: Font.Bold
                }
                Text {
                    text: root._word; color: Theme.red; font.pixelSize: 12
                    Layout.fillWidth: true; elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Rectangle {
                visible: root._bad
                Layout.fillWidth: true; Layout.preferredHeight: 1
                color: Theme.borderSoft; Layout.bottomMargin: 3
            }

            // Suggestions (text-only rows, like word choices).
            Repeater {
                model: root._suggestions
                delegate: SpellRow {
                    required property var modelData
                    label: modelData
                    onActivated: { menu.close(); spell.replaceWord(root._pos, modelData) }
                }
            }
            Text {
                visible: root._bad && root._suggestions.length === 0
                text: qsTr("(no suggestions)"); color: Theme.text3
                font.pixelSize: 12; font.italic: true
                Layout.leftMargin: 9; Layout.topMargin: 4; Layout.bottomMargin: 4
            }

            Rectangle {
                visible: root._bad
                Layout.fillWidth: true; Layout.preferredHeight: 1
                color: Theme.borderSoft; Layout.topMargin: 3; Layout.bottomMargin: 3
            }

            // Actions.
            SpellRow {
                visible: root._bad
                icon: "library_add"; label: qsTr("Add to Dictionary")
                onActivated: { menu.close(); spell.addToDictionary(root._word) }
            }
            SpellRow {
                visible: root.dialog !== null
                icon: "spellcheck"; label: qsTr("Check Spelling…")
                onActivated: { menu.close(); if (root.dialog) root.dialog.openFor(spell, root.target) }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 1
                color: Theme.borderSoft; Layout.topMargin: 3; Layout.bottomMargin: 3
            }

            // Undo/Redo — same ordering as TextEdit::contextMenuEvent() in the
            // Widgets app (Undo, Redo, divider, then Cut/Copy/Paste/…). Ctrl+Z
            // already works natively while the field has focus (Qt Quick's text
            // editing controls handle it internally); these rows just expose the
            // same operation via the menu, for parity with the Widgets menu and
            // with reach for anyone who doesn't know the key.
            SpellRow {
                enabled: root._canUndo
                icon: "undo"; label: qsTr("Undo")
                onActivated: { menu.close(); root.target.undo() }
            }
            SpellRow {
                enabled: root._canRedo
                icon: "redo"; label: qsTr("Redo")
                onActivated: { menu.close(); root.target.redo() }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 1
                color: Theme.borderSoft; Layout.topMargin: 3; Layout.bottomMargin: 3
            }

            // Standard edit actions — parity with TextEdit::contextMenuEvent()
            // in the Widgets app (Cut/Copy/Paste/Paste Unformatted/Delete/
            // Select All).
            SpellRow {
                enabled: root._hasSelection
                icon: "content_cut"; label: qsTr("Cut")
                onActivated: { menu.close(); root.target.cut() }
            }
            SpellRow {
                enabled: root._hasSelection
                icon: "content_copy"; label: qsTr("Copy")
                onActivated: { menu.close(); root.target.copy() }
            }
            SpellRow {
                enabled: root._canPaste
                icon: "content_paste"; label: qsTr("Paste")
                onActivated: { menu.close(); root.target.paste() }
            }
            SpellRow {
                enabled: root._canPaste
                icon: "content_paste_go"; label: qsTr("Paste Unformatted")
                onActivated: { menu.close(); root._pasteUnformatted() }
            }
            SpellRow {
                enabled: root._hasSelection
                icon: "delete"; label: qsTr("Delete")
                onActivated: { menu.close(); root.target.remove(root.target.selectionStart, root.target.selectionEnd) }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 1
                color: Theme.borderSoft; Layout.topMargin: 3; Layout.bottomMargin: 3
            }

            SpellRow {
                icon: "select_all"; label: qsTr("Select All")
                onActivated: { menu.close(); root.target.selectAll() }
            }
        }

        // One menu row — matches RecordContextMenu's MenuRow. `icon` optional:
        // when empty (suggestion words) the label sits at the left margin.
        // `enabled` (Item's built-in property) dims the row and blocks its
        // hover/tap, for actions like Cut/Paste that need a selection or
        // clipboard content — parity with the Widgets menu's setEnabled().
        component SpellRow: Rectangle {
            id: sr
            property string icon: ""
            property string label: ""
            signal activated()
            Layout.fillWidth: true
            implicitHeight: 32
            radius: Theme.radiusSm
            opacity: sr.enabled ? 1.0 : 0.45
            color: srHover.hovered ? Theme.surface2 : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 9; anchors.rightMargin: 9
                spacing: 10
                MaterialIcon {
                    name: sr.icon; size: 17; color: Theme.text2
                    visible: sr.icon !== ""
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    text: sr.label; color: Theme.text; font.pixelSize: 13
                    Layout.fillWidth: true; elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
            HoverHandler { id: srHover }
            TapHandler { onTapped: sr.activated() }
        }
    }
}
