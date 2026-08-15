// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Full-field spell-check dialog — the QML port of the legacy spellcheckdialog.
// Declare one per page and open it with openFor(spellObject, editorItem); it
// walks the field word by word offering Change / Change All / Ignore Once /
// Ignore All / Add to Dictionary.
Dialog {
    id: dlg
    anchors.centerIn: parent
    width: 400
    scale: Theme.uiScale   // match the zoomed workspace (centered origin)
    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape

    // Bound target for the current run.
    property var spell:  null
    property var target: null

    // Current misspelled word being reviewed.
    property int    _start:  -1
    property int    _length: 0
    property string _word:   ""
    property var    _suggestions: []
    property bool   _finished: false

    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

    function openFor(spellObject, editor) {
        dlg.spell  = spellObject
        dlg.target = editor
        dlg._finished = false
        dlg._advance(0)
        dlg.open()
    }

    // Find the next misspelled word at/after `fromPos` and present it. When none
    // remain, switch to the "complete" state.
    function _advance(fromPos) {
        if (!dlg.spell) { dlg._finished = true; return }
        var r = dlg.spell.nextMisspelled(fromPos)
        if (r && r.found) {
            dlg._start  = r.start
            dlg._length = r.length
            dlg._word   = r.word
            dlg._suggestions = dlg.spell.suggestionsFor(r.word)
            changeField.text = r.word
            suggestList.currentIndex = dlg._suggestions.length > 0 ? 0 : -1
            // Reveal + select the word in the editor while we review it.
            if (dlg.target) {
                dlg.target.forceActiveFocus()
                dlg.target.cursorPosition = dlg._start
                dlg.target.select(dlg._start, dlg._start + dlg._length)
            }
        } else {
            dlg._finished = true
            dlg._word = ""
            if (dlg.target)
                dlg.target.deselect()
        }
    }

    function _change() {
        if (dlg._start < 0) return
        var repl = changeField.text
        dlg.spell.replaceRange(dlg._start, dlg._length, repl)
        dlg._advance(dlg._start + repl.length)
    }
    function _changeAll() {
        if (dlg._start < 0) return
        dlg.spell.replaceAll(dlg._word, changeField.text)
        dlg._advance(dlg._start)
    }
    function _ignoreOnce() { dlg._advance(dlg._start + dlg._length) }
    function _ignoreAll()  { dlg.spell.ignoreAll(dlg._word);      dlg._advance(dlg._start) }
    function _addToDict()  { dlg.spell.addToDictionary(dlg._word); dlg._advance(dlg._start) }

    contentItem: ColumnLayout {
        spacing: 0

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            MaterialIcon { name: "spellcheck"; size: 17; color: Theme.accent }
            Text {
                text: qsTr("Check Spelling")
                color: Theme.text; font.pixelSize: 14; font.weight: Font.Bold
                Layout.fillWidth: true
            }
            MaterialIcon {
                name: "close"; size: 16; color: Theme.text3
                TapHandler { onTapped: dlg.close() }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        // ── Completed state ────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 9
            visible: dlg._finished
            RowLayout {
                spacing: 6
                MaterialIcon { name: "task_alt"; size: 17; color: Theme.green }
                Text {
                    text: qsTr("Spell check complete.")
                    color: Theme.text; font.pixelSize: 13
                }
            }
            Item {
                Layout.fillWidth: true
                implicitHeight: 26
                DialogButton {
                    anchors.right: parent.right
                    label: qsTr("Done"); primary: true
                    onClicked: dlg.close()
                }
            }
        }

        // ── Review state ───────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 13
            spacing: 8
            visible: !dlg._finished

            Text { text: qsTr("Not in dictionary:"); color: Theme.text3; font.pixelSize: 10; font.weight: Font.DemiBold }
            Text {
                text: dlg._word
                color: Theme.red; font.pixelSize: 14; font.weight: Font.DemiBold
            }

            Text { text: qsTr("Change to:"); color: Theme.text3; font.pixelSize: 10; font.weight: Font.DemiBold }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 30
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: changeField.activeFocus ? Theme.accent : Theme.border
                TextField {
                    id: changeField
                    anchors.fill: parent
                    anchors.leftMargin: 7
                    anchors.rightMargin: 7
                    topPadding: 0
                    bottomPadding: 0
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.text
                    font.pixelSize: 12
                    background: null
                    selectByMouse: true
                    onAccepted: dlg._change()
                }
            }

            Text { text: qsTr("Suggestions:"); color: Theme.text3; font.pixelSize: 10; font.weight: Font.DemiBold }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: Theme.border
                ListView {
                    id: suggestList
                    anchors.fill: parent
                    anchors.margins: 3
                    clip: true
                    model: dlg._suggestions
                    delegate: ItemDelegate {
                        required property int index
                        required property var modelData
                        width: suggestList.width
                        height: 25
                        contentItem: Text {
                            text: modelData; color: Theme.text; font.pixelSize: 12
                            leftPadding: 7; verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 4
                            color: suggestList.currentIndex === index ? Theme.accentSoft
                                   : hovered ? Theme.surface2 : "transparent"
                        }
                        onClicked: { suggestList.currentIndex = index; changeField.text = modelData }
                        onDoubleClicked: { changeField.text = modelData; dlg._change() }
                    }
                }
            }

            // Action buttons
            GridLayout {
                Layout.fillWidth: true
                Layout.topMargin: 3
                columns: 3
                columnSpacing: 6
                rowSpacing: 6
                DialogButton { Layout.fillWidth: true; label: qsTr("Ignore Once");     onClicked: dlg._ignoreOnce() }
                DialogButton { Layout.fillWidth: true; label: qsTr("Ignore All");      onClicked: dlg._ignoreAll() }
                DialogButton { Layout.fillWidth: true; label: qsTr("Add to Dictionary"); onClicked: dlg._addToDict() }
                DialogButton { Layout.fillWidth: true; label: qsTr("Change");  primary: true; onClicked: dlg._change() }
                DialogButton { Layout.fillWidth: true; label: qsTr("Change All"); primary: true; onClicked: dlg._changeAll() }
                DialogButton { Layout.fillWidth: true; label: qsTr("Cancel");   onClicked: dlg.close() }
            }
        }
    }

    // Small themed pushbutton used throughout the dialog.
    component DialogButton: Rectangle {
        property string label: ""
        property bool   primary: false
        signal clicked()
        implicitHeight: 26
        implicitWidth: btnText.implicitWidth + 18
        radius: Theme.radiusSm
        color: primary ? (bHover.hovered ? Theme.accentStrong : Theme.accent)
                       : (bHover.hovered ? Theme.surface2 : Theme.surface)
        border.color: primary ? "transparent" : Theme.border
        Text {
            id: btnText
            anchors.centerIn: parent
            text: parent.label
            color: parent.primary ? "#ffffff" : Theme.text
            font.pixelSize: 11; font.weight: Font.DemiBold
        }
        HoverHandler { id: bHover }
        TapHandler { onTapped: parent.clicked() }
    }
}
