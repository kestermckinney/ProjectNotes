// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Meeting / note detail: title, date, internal flag, a rich-text note body with
// a formatting toolbar (TextFormatter), plus Attendees and Action Items panels.
Item {
    id: page

    property int    noteRow: -1
    property string noteId:  ""
    property string projectId: ""
    property bool   isNewRecord: false
    property bool   _changed: false

    // Right-click spell-check menu state.
    property int    _rcPos: -1
    property string _rcWord: ""
    property var    _rcSuggestions: []

    Component.onCompleted: {
        if (page.noteId !== "")
            DesktopAppController.setNoteFilter(page.noteId)
        _reload()
    }

    function _reload() {
        var d = DesktopAppController.getProjectNoteData(page.noteRow)
        titleField.text = (d.note_title || "").toString()
        dateField.text  = (d.note_date || "").toString()
        internalCheck.checked = (d.internal_item || "0") !== "0"
        // Imperative assignment breaks the binding so TextFormatter edits to the
        // QTextDocument are not overwritten by a re-evaluation.
        noteEdit.text = (d.note || "").toString()
        page._changed = false
    }

    function _saveNow() {
        if (!page._changed) return true
        var ok = DesktopAppController.saveProjectNote(
            page.noteRow, titleField.text, dateField.text,
            TextFormatter.documentHtml(noteEdit.textDocument), internalCheck.checked)
        if (ok) page._changed = false
        return ok
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 18
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: page.width - 36
            spacing: 14

            // Title + date + internal
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                FormField {
                    label: qsTr("Title")
                    id: titleField
                    onEdited: page._changed = true
                }
                DateField {
                    label: qsTr("Date")
                    id: dateField
                    Layout.preferredWidth: 180
                    Layout.maximumWidth: 180
                    onEdited: page._changed = true
                }
            }

            RowLayout {
                spacing: 8
                CheckBox {
                    id: internalCheck
                    onToggled: page._changed = true
                    indicator: Rectangle {
                        implicitWidth: 18; implicitHeight: 18
                        radius: 4
                        x: internalCheck.leftPadding
                        y: parent.height / 2 - height / 2
                        color: internalCheck.checked ? Theme.accent : Theme.surface
                        border.color: internalCheck.checked ? Theme.accent : Theme.border
                        MaterialIcon {
                            anchors.centerIn: parent
                            visible: internalCheck.checked
                            name: "check"; size: 14; color: "#ffffff"
                        }
                    }
                    contentItem: Text {
                        text: qsTr("Internal item")
                        color: Theme.text
                        font.pixelSize: 13
                        leftPadding: internalCheck.indicator.width + 8
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // Note body: toolbar + editor
            Text {
                text: qsTr("Note")
                color: Theme.text3
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
            NoteFormatToolbar {
                Layout.fillWidth: true
                editor: noteEdit
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(220, noteEdit.contentHeight + 24)
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: noteEdit.activeFocus ? Theme.accent : Theme.border
                TextArea {
                    id: noteEdit
                    anchors.fill: parent
                    anchors.margins: 10
                    color: Theme.text
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.WordWrap
                    selectByMouse: true
                    persistentSelection: true
                    background: null
                    font.family: "Arial"
                    font.pixelSize: 13
                    onTextChanged: page._changed = true

                    // Inline spell-check (red squiggle under unknown words).
                    SpellCheck { id: spell; document: noteEdit.textDocument; enabled: true }

                    // Right-click a misspelled word for suggestions.
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (ep) => {
                            var pos = noteEdit.positionAt(ep.position.x, ep.position.y)
                            var w = spell.wordAt(pos)
                            if (w !== "" && spell.isMisspelled(w)) {
                                page._rcPos = pos
                                page._rcWord = w
                                page._rcSuggestions = spell.suggestionsFor(w).slice(0, 8)
                                spellMenu.popup()
                            }
                        }
                    }

                    Menu {
                        id: spellMenu
                        Repeater {
                            model: page._rcSuggestions
                            MenuItem {
                                required property var modelData
                                text: modelData
                                onTriggered: spell.replaceWord(page._rcPos, modelData)
                            }
                        }
                        MenuSeparator { visible: page._rcSuggestions.length > 0 }
                        MenuItem {
                            text: qsTr("Add “%1” to Dictionary").arg(page._rcWord)
                            onTriggered: spell.addToDictionary(page._rcWord)
                        }
                    }
                }
            }

            // Panels
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 6
                spacing: 14

                // Attendees
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    implicitHeight: attCol.implicitHeight + 24
                    ColumnLayout {
                        id: attCol
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8
                        RowLayout {
                            Layout.fillWidth: true
                            MaterialIcon { name: "groups"; size: 18; color: Theme.text2 }
                            Text {
                                text: qsTr("Attendees"); color: Theme.text
                                font.pixelSize: 14; font.weight: Font.Bold
                                Layout.fillWidth: true
                            }
                            SmallAddButton {
                                text: qsTr("Add")
                                onClicked: { page._saveNow(); peoplePicker.open() }
                            }
                        }
                        Repeater {
                            model: DesktopAppController.meetingAttendeesModel
                            delegate: RowLayout {
                                required property int index
                                required property var model
                                Layout.fillWidth: true
                                spacing: 8
                                Rectangle {
                                    width: 26; height: 26; radius: 13
                                    color: Theme.accentSoft
                                    Text {
                                        anchors.centerIn: parent
                                        text: {
                                            var n = (model.name || "").toString().trim()
                                            if (n === "") return "?"
                                            var parts = n.split(" ")
                                            return (parts[0][0] || "") + (parts.length > 1 ? parts[parts.length-1][0] : "")
                                        }
                                        color: Theme.accent; font.pixelSize: 10; font.weight: Font.Bold
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Text {
                                        text: (model.name || qsTr("(no name)")).toString()
                                        color: Theme.text; font.pixelSize: 13; elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: (model.email || "").toString()
                                        visible: text !== ""
                                        color: Theme.text3; font.pixelSize: 11; elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                                DeleteButton {
                                    onClicked: {
                                        DesktopAppController.deleteAttendee(index)
                                        DesktopAppController.refreshMeetingAttendees()
                                    }
                                }
                            }
                        }
                    }
                }

                // Action Items
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    implicitHeight: aiCol.implicitHeight + 24
                    ColumnLayout {
                        id: aiCol
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8
                        RowLayout {
                            Layout.fillWidth: true
                            MaterialIcon { name: "task_alt"; size: 18; color: Theme.text2 }
                            Text {
                                text: qsTr("Action Items"); color: Theme.text
                                font.pixelSize: 14; font.weight: Font.Bold
                                Layout.fillWidth: true
                            }
                            SmallAddButton {
                                text: qsTr("Add")
                                onClicked: {
                                    page._saveNow()
                                    DesktopAppController.addNoteActionItem(page.noteId, page.projectId)
                                    DesktopAppController.refreshNoteActionItems()
                                }
                            }
                        }
                        Repeater {
                            model: DesktopAppController.notesActionItemsModel
                            delegate: RowLayout {
                                required property int index
                                required property var model
                                Layout.fillWidth: true
                                spacing: 8
                                MaterialIcon {
                                    name: {
                                        var s = (model.status || "").toString().toLowerCase()
                                        if (s.indexOf("resolved") >= 0 || s.indexOf("closed") >= 0) return "check_circle"
                                        return "radio_button_unchecked"
                                    }
                                    size: 16
                                    color: {
                                        var s = (model.status || "").toString().toLowerCase()
                                        return (s.indexOf("resolved") >= 0 || s.indexOf("closed") >= 0)
                                               ? Theme.green : Theme.text3
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Text {
                                        text: (model.item_name || qsTr("(unnamed item)")).toString()
                                        color: Theme.text; font.pixelSize: 13; elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: {
                                            var p = (model.priority || "").toString()
                                            var a = (model.assigned_to || "").toString()
                                            var who = a !== "" ? DesktopAppController.peopleNameForId(a) : ""
                                            return [p, who].filter(function(x){ return x !== "" }).join(" · ")
                                        }
                                        visible: text !== ""
                                        color: Theme.text3; font.pixelSize: 11; elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                                DeleteButton {
                                    onClicked: {
                                        DesktopAppController.deleteNoteActionItem(index)
                                        DesktopAppController.refreshNoteActionItems()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }
        }
    }

    // ── People picker (for adding an attendee) ────────────────────────────────
    Dialog {
        id: peoplePicker
        anchors.centerIn: parent
        width: 360
        height: 420
        modal: true
        padding: 0
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        contentItem: ColumnLayout {
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 14
                Text {
                    text: qsTr("Add Attendee"); color: Theme.text
                    font.pixelSize: 15; font.weight: Font.Bold
                    Layout.fillWidth: true
                }
                MaterialIcon {
                    name: "close"; size: 20; color: Theme.text3
                    TapHandler { onTapped: peoplePicker.close() }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
            ListView {
                id: peopleList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: DesktopAppController.peopleList()
                delegate: ItemDelegate {
                    required property int index
                    required property var modelData
                    width: peopleList.width
                    height: 40
                    contentItem: Text {
                        text: modelData.name
                        color: Theme.text
                        font.pixelSize: 13
                        leftPadding: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { color: hovered ? Theme.surface2 : "transparent" }
                    onClicked: {
                        var r = DesktopAppController.addAttendee(page.noteId)
                        if (r >= 0) {
                            DesktopAppController.saveAttendee(r, modelData.id)
                            DesktopAppController.refreshMeetingAttendees()
                        }
                        peoplePicker.close()
                    }
                }
            }
        }
    }

    // ── Inline reusable buttons ───────────────────────────────────────────────
    component SmallAddButton: Rectangle {
        property string text: ""
        signal clicked()
        implicitHeight: 26
        implicitWidth: sRow.implicitWidth + 16
        radius: Theme.radiusSm
        color: sHover.hovered ? Theme.accentStrong : Theme.accent
        RowLayout {
            id: sRow
            anchors.centerIn: parent
            spacing: 4
            MaterialIcon { name: "add"; size: 14; color: "#ffffff" }
            Text { text: parent.parent.text; color: "#ffffff"; font.pixelSize: 11; font.weight: Font.DemiBold }
        }
        HoverHandler { id: sHover }
        TapHandler { onTapped: parent.clicked() }
    }

    component DeleteButton: Item {
        signal clicked()
        implicitWidth: 26; implicitHeight: 26
        Rectangle {
            anchors.centerIn: parent
            width: 24; height: 24; radius: 6
            color: dHover.hovered ? Theme.redSoft : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: "close"; size: 15; color: Theme.red }
        }
        HoverHandler { id: dHover }
        TapHandler { onTapped: parent.clicked() }
    }
}
