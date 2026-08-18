// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// A find & replace bar for a TextArea/TextEdit (the note body). Mirrors the
// Widgets Find/Replace dialog: find next/previous (with wrap), replace, replace
// all, and a match-case toggle. Operates on the editor's plain text so it works
// with the rich-text note document; replacements inherit the surrounding format.
//
// Usage: set `editor` to the TextArea, call open()/close()/toggle(). Kept hidden
// (height 0) until opened so it doesn't take space in the layout.
Rectangle {
    id: bar

    property Item editor: null
    property bool matchCase: false
    property string _status: ""

    visible: false
    Layout.fillWidth: true
    implicitHeight: visible ? col.implicitHeight + 12 : 0
    radius: Theme.radiusSm
    color: Theme.raise
    border.color: Theme.border

    function open() {
        bar.visible = true
        // Seed the find box with the current selection, if any.
        if (editor && editor.selectedText && editor.selectedText.length > 0)
            findBox.field.text = editor.selectedText
        bar._status = ""
        findBox.field.forceActiveFocus()
        findBox.field.selectAll()
    }
    function close() {
        bar.visible = false
        if (editor) editor.forceActiveFocus()
    }
    function toggle() { bar.visible ? close() : open() }

    function _plain() { return editor ? editor.getText(0, editor.length) : "" }

    // Index of the next/previous occurrence of the find text from `from`, or -1.
    function _indexFrom(from, forward) {
        var needle = findBox.field.text
        if (needle === "" || !editor) return -1
        var hay = _plain()
        var h = bar.matchCase ? hay : hay.toLowerCase()
        var n = bar.matchCase ? needle : needle.toLowerCase()
        return forward ? h.indexOf(n, from) : h.lastIndexOf(n, from)
    }

    function findNext(forward) {
        var needle = findBox.field.text
        if (needle === "" || !editor) { bar._status = ""; return }
        var idx
        if (forward) {
            idx = _indexFrom(Math.max(editor.selectionEnd, editor.cursorPosition), true)
            if (idx < 0) idx = _indexFrom(0, true)                 // wrap to top
        } else {
            idx = _indexFrom(Math.max(0, editor.selectionStart - 1), false)
            if (idx < 0) idx = _indexFrom(editor.length, false)    // wrap to bottom
        }
        if (idx >= 0) {
            editor.select(idx, idx + needle.length)                // persistentSelection keeps it visible
            bar._status = ""
        } else {
            bar._status = qsTr("Not found")
        }
    }

    function replaceCurrent() {
        if (!editor || findBox.field.text === "") return
        var sel = editor.selectedText
        var isMatch = editor.selectionStart !== editor.selectionEnd
            && (bar.matchCase ? sel === findBox.field.text
                              : sel.toLowerCase() === findBox.field.text.toLowerCase())
        if (isMatch) {
            var s = editor.selectionStart
            editor.remove(editor.selectionStart, editor.selectionEnd)
            editor.insert(s, replaceBox.field.text)
            editor.select(s, s + replaceBox.field.text.length)
        }
        findNext(true)
    }

    function replaceAll() {
        if (!editor || findBox.field.text === "") return
        var needle = findBox.field.text
        var count = 0, from = 0, guard = 0
        while (guard++ < 100000) {
            var hay = _plain()
            var h = bar.matchCase ? hay : hay.toLowerCase()
            var n = bar.matchCase ? needle : needle.toLowerCase()
            var pos = h.indexOf(n, from)
            if (pos < 0) break
            editor.remove(pos, pos + needle.length)
            editor.insert(pos, replaceBox.field.text)
            from = pos + replaceBox.field.text.length
            count++
        }
        bar._status = qsTr("%1 replaced").arg(count)
    }

    // A themed text input used for both the find and replace boxes.
    component Box: Rectangle {
        property alias field: tf
        property string placeholder: ""
        signal accepted()
        signal shiftAccepted()
        Layout.fillWidth: true
        implicitHeight: 27
        radius: Theme.radiusSm
        color: Theme.surface
        border.color: tf.activeFocus ? Theme.accent : Theme.border
        TextField {
            id: tf
            anchors.fill: parent
            anchors.leftMargin: 7; anchors.rightMargin: 7
            verticalAlignment: Text.AlignVCenter
            color: Theme.text
            placeholderText: parent.placeholder
            placeholderTextColor: Theme.text3
            background: null
            selectByMouse: true
            font.pixelSize: Theme.fontBody
            Keys.onReturnPressed: (ev) => {
                if (ev.modifiers & Qt.ShiftModifier) parent.shiftAccepted()
                else parent.accepted()
            }
            Keys.onEscapePressed: bar.close()
        }
    }

    component BarButton: Rectangle {
        property string label: ""
        property bool primary: false
        signal clicked()
        implicitHeight: 27
        implicitWidth: bbText.implicitWidth + 16
        radius: Theme.radiusSm
        color: primary ? (bbHover.hovered ? Theme.accentStrong : Theme.accent)
                       : (bbHover.hovered ? Theme.surface2 : Theme.surface)
        border.color: primary ? "transparent" : Theme.border
        Text {
            id: bbText
            anchors.centerIn: parent
            text: parent.label
            color: parent.primary ? "#ffffff" : Theme.text
            font.pixelSize: Theme.fontSm; font.weight: Font.DemiBold
        }
        HoverHandler { id: bbHover }
        TapHandler { onTapped: parent.clicked() }
    }

    ColumnLayout {
        id: col
        anchors.left: parent.left; anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 7; anchors.rightMargin: 7
        spacing: 5

        // Find row
        RowLayout {
            Layout.fillWidth: true
            spacing: 5
            MaterialIcon { name: "search"; size: 14; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
            Box {
                id: findBox
                placeholder: qsTr("Find")
                onAccepted: bar.findNext(true)
                onShiftAccepted: bar.findNext(false)
            }
            BarButton { label: qsTr("Previous"); onClicked: bar.findNext(false) }
            BarButton { label: qsTr("Next"); primary: true; onClicked: bar.findNext(true) }
            // Match case toggle
            Rectangle {
                implicitHeight: 27; implicitWidth: 30
                radius: Theme.radiusSm
                color: bar.matchCase ? Theme.accent : (mcHover.hovered ? Theme.surface2 : Theme.surface)
                border.color: bar.matchCase ? "transparent" : Theme.border
                Text {
                    anchors.centerIn: parent; text: "Aa"
                    color: bar.matchCase ? "#ffffff" : Theme.text2
                    font.pixelSize: Theme.fontSm; font.weight: Font.DemiBold
                }
                HoverHandler { id: mcHover }
                TapHandler { onTapped: bar.matchCase = !bar.matchCase }
                ToolTip.visible: mcHover.hovered
                ToolTip.text: qsTr("Match case")
            }
            MaterialIcon {
                name: "close"; size: 16; color: Theme.text3
                Layout.alignment: Qt.AlignVCenter
                TapHandler { onTapped: bar.close() }
            }
        }

        // Replace row
        RowLayout {
            Layout.fillWidth: true
            spacing: 5
            MaterialIcon { name: "find_replace"; size: 14; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
            Box {
                id: replaceBox
                placeholder: qsTr("Replace with")
                onAccepted: bar.replaceCurrent()
            }
            BarButton { label: qsTr("Replace"); onClicked: bar.replaceCurrent() }
            BarButton { label: qsTr("Replace All"); onClicked: bar.replaceAll() }
            Text {
                text: bar._status
                visible: text !== ""
                color: Theme.text3; font.pixelSize: Theme.fontXs
                Layout.alignment: Qt.AlignVCenter
            }
            Item { Layout.fillWidth: true }
        }
    }
}
