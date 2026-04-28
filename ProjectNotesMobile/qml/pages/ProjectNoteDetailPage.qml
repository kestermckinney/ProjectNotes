// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Note")

    property int    noteRow:         -1
    property string noteId:          ""
    property string projectId:       ""
    property string initialTitle:    ""
    property string initialDate:     ""
    property string initialNote:     ""
    property bool   initialInternal: false
    property bool   _skipSave:       false
    property bool   _formatSheetOpen: false

    // Saved selection for format sheet (focus leaves noteEdit when sheet opens)
    property int _selStart: 0
    property int _selEnd:   0

    Component.onCompleted: {
        if (root.noteId !== "")
            AppController.setNoteFilter(root.noteId, root.projectId)
    }

    function _saveNow() {
        dateField.commitPending()
        return AppController.saveProjectNote(root.noteRow, titleField.text, dateField.text,
                                             root.scaleFontSizes(TextFormatter.documentHtml(noteEdit.textDocument), 1 / 1.5),
                                             internalSwitch.checked)
    }

    function _reloadData() {
        var d = AppController.getProjectNoteData(root.noteRow)
        titleField.text        = (d.note_title    || "").toString()
        dateField.text         = (d.note_date     || "").toString()
        noteEdit.text          = root.toRichText(root.scaleFontSizes((d.note || "").toString(), 1.5))
        internalSwitch.checked = (d.internal_item || "0") !== "0"
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

    // ── Header: utility buttons only ─────────────────────────────────────────
    header: ToolBar {
        implicitHeight: 44

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Item { Layout.fillWidth: true }

            ToolButton {
                icon.name: "envelope"
                focusPolicy: Qt.NoFocus
                onClicked: {
                    if (!root._saveNow()) return
                    var emails = AppController.attendeeEmailList()
                    if (emails === "") return
                    var projNum  = AppController.projectNumberForId(root.projectId)
                    var projName = AppController.projectNameForId(root.projectId)
                    var dateParts = root.initialDate ? root.initialDate.split("-") : []
                    var formattedDate = dateParts.length === 3
                        ? (parseInt(dateParts[1]) + "/" + parseInt(dateParts[2]) + "/" + dateParts[0])
                        : root.initialDate
                    var subject = projNum + " " + projName + " - " + titleField.text + " - " + formattedDate
                    var body    = AppController.htmlToPlainText(TextFormatter.documentHtml(noteEdit.textDocument)).replace(/\n/g, '\r\n')
                    Qt.openUrlExternally("mailto:" + emails
                        + "?subject=" + encodeURIComponent(subject)
                        + "&body="    + encodeURIComponent(body))
                }
            }

            ToolButton {
                icon.name: "doc.on.doc"
                focusPolicy: Qt.NoFocus
                onClicked: {
                    if (!root._saveNow()) return
                    root._skipSave = true
                    var newRow = AppController.copyProjectNote(root.noteRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getProjectNoteData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("ProjectNoteDetailPage.qml"), {
                        noteRow:         newRow,
                        noteId:          (d.id            || "").toString(),
                        projectId:       root.projectId,
                        initialTitle:    (d.note_title    || "").toString(),
                        initialDate:     (d.note_date     || "").toString(),
                        initialNote:     (d.note          || "").toString(),
                        initialInternal: (d.internal_item || "0") !== "0"
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                focusPolicy: Qt.NoFocus
                onClicked: {
                    root._skipSave = true
                    AppController.deleteProjectNote(root.noteRow)
                    root.StackView.view.pop()
                }
            }
        }
    }

    // ── Footer: Attendees + Action Items ─────────────────────────────────────
    footer: ToolBar {
        RowLayout {
            anchors.centerIn: parent
            spacing: 32

            ToolButton {
                icon.name: "person.2"
                text: qsTr("Attendees")
                display: AbstractButton.TextUnderIcon
                onClicked: {
                    if (!root._saveNow()) return
                    root.StackView.view.push(Qt.resolvedUrl("MeetingAttendeesPage.qml"), {
                        noteId:    root.noteId,
                        noteBody:  AppController.htmlToPlainText(TextFormatter.documentHtml(noteEdit.textDocument)).replace(/\n/g, '\r\n'),
                        noteTitle: titleField.text,
                        noteDate:  dateField.text
                    })
                }
            }

            ToolButton {
                icon.name: "checklist"
                text: qsTr("Action Items")
                display: AbstractButton.TextUnderIcon
                onClicked: {
                    if (!root._saveNow()) return
                    root.StackView.view.push(Qt.resolvedUrl("NoteActionItemsPage.qml"), {
                        noteId:    root.noteId,
                        projectId: root.projectId
                    })
                }
            }
        }
    }

    // ── Page body ─────────────────────────────────────────────────────────────
    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            SectionHeader { text: qsTr("Title") }
            FieldRow {
                TextField {
                    id: titleField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialTitle
                    horizontalAlignment: TextInput.AlignLeft
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Date") }
            DateFieldRow { id: dateField; text: root.initialDate }

            SectionHeader { text: qsTr("Note") }

            // // Explicit Flickable wrapper preserves the touch-event routing that
            // // lets the TextEdit acquire keyboard focus when tapped on iOS.
            // // Word-wrap + auto-height means the Flickable never actually scrolls;
            // // the outer ScrollView handles all vertical scrolling.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(120, noteEdit.contentHeight + 24)
                color: palette.base
                TextEdit {
                    id: noteEdit
                    anchors { fill: parent; margins: 8 }
                    color: palette.text
                    selectByMouse: true

                    // x: 16
                    // y: 8
                    // width: parent.width - 32

                    text: root.toRichText(root.scaleFontSizes(root.initialNote, 1.5))
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.WordWrap
                    // font.pixelSize: 15

                    // onActiveFocusChanged: {
                    //     if (activeFocus) Qt.inputMethod.show()
                    // }

                    // onCursorRectangleChanged: {
                    //     if (!activeFocus) return
                    //     var outerFlick = scrollView.contentItem
                    //     if (!outerFlick) return
                    //     // Cursor absolute position in ScrollView content space
                    //     var cursorTop    = noteFlick.y + noteEdit.y + cursorRectangle.y
                    //     var cursorBottom = cursorTop + cursorRectangle.height
                    //     var viewTop      = outerFlick.contentY
                    //     var viewBottom   = viewTop + scrollView.height

                    //     if (cursorBottom > viewBottom) {
                    //         outerFlick.contentY = Math.max(0, cursorBottom - scrollView.height + 16)
                    //     } else if (cursorTop < viewTop) {
                    //         outerFlick.contentY = Math.max(0, cursorTop - 16)
                    //     }
                    // }
                }
                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1; color: palette.placeholderText; opacity: 0.3
                }
            }

            SectionHeader {
                visible: AppController.showInternalItems
                text: qsTr("Internal")
            }
            FieldRow {
                visible: AppController.showInternalItems
                Switch {
                    id: internalSwitch
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 12 }
                    checked: root.initialInternal
                    text: qsTr("Internal Item")
                }
            }

            // Extra space at the bottom so content is reachable above the keyboard
            // + keyboard accessory bar (44 px) when the keyboard is open.
            Item {
                Layout.preferredHeight: Qt.inputMethod.visible
                                        ? Qt.inputMethod.keyboardRectangle.height + 44 + 24
                                        : 24
            }
        }
    }

    // ── Keyboard accessory toolbar ────────────────────────────────────────────
    // Floats above the iOS keyboard using Overlay.overlay coordinates.
    Popup {
        id: keyboardAccessory
        parent: Overlay.overlay
        modal: false
        dim: false
        closePolicy: Popup.NoAutoClose
        padding: 0

        x: 0
        y: Qt.inputMethod.visible
           ? Qt.inputMethod.keyboardRectangle.y - height
           : (parent ? parent.height - height : 0)
        width:  parent ? parent.width : 390
        height: 44

        visible: !_formatSheetOpen && (Qt.inputMethod.visible || noteEdit.selectionStart !== noteEdit.selectionEnd)

        background: Rectangle {
            color: Qt.rgba(palette.window.r, palette.window.g, palette.window.b, 0.97)
            Rectangle {
                anchors { top: parent.top; left: parent.left; right: parent.right }
                height: 1; color: palette.mid; opacity: 0.4
            }
        }

        contentItem: RowLayout {
            anchors { fill: parent; leftMargin: 4; rightMargin: 4 }
            spacing: 0

            // Aa — open the Format sheet
            ToolButton {
                text: "Aa"
                font.pixelSize: 16
                font.weight: Font.DemiBold
                focusPolicy: Qt.NoFocus
                onClicked: {
                    root._selStart = noteEdit.selectionStart
                    root._selEnd   = noteEdit.selectionEnd
                    formatSheet.textDocument = noteEdit.textDocument
                    formatSheet.selStart     = root._selStart
                    formatSheet.selEnd       = root._selEnd
                    formatSheet.open()
                }
            }

            Rectangle { width: 1; height: 22; color: palette.mid; opacity: 0.4; Layout.alignment: Qt.AlignVCenter }

            ToolButton {
                icon.name: "list.bullet"; focusPolicy: Qt.NoFocus
                onClicked: {
                    TextFormatter.applyStyle(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd, 1)
                    noteEdit.forceActiveFocus()
                }
            }
            ToolButton {
                icon.name: "list.number"; focusPolicy: Qt.NoFocus
                onClicked: {
                    TextFormatter.applyStyle(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd, 4)
                    noteEdit.forceActiveFocus()
                }
            }

            Rectangle { width: 1; height: 22; color: palette.mid; opacity: 0.4; Layout.alignment: Qt.AlignVCenter }

            ToolButton {
                text: "⇥"; font.pixelSize: 15; focusPolicy: Qt.NoFocus
                onClicked: {
                    TextFormatter.indentText(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd)
                    noteEdit.forceActiveFocus()
                }
            }
            ToolButton {
                text: "⇤"; font.pixelSize: 15; focusPolicy: Qt.NoFocus
                onClicked: {
                    TextFormatter.unindentText(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd)
                    noteEdit.forceActiveFocus()
                }
            }

            Item { Layout.fillWidth: true }

            // Dismiss keyboard
            ToolButton {
                icon.name: "keyboard.chevron.compact.down"
                focusPolicy: Qt.NoFocus
                onClicked: Qt.inputMethod.hide()
            }
        }
    }

    // ── Format sheet ──────────────────────────────────────────────────────────
    NoteFormatSheet {
        id: formatSheet
        parent: Overlay.overlay
        onOpened: root._formatSheetOpen = true
        onClosed: { root._formatSheetOpen = false; noteEdit.forceActiveFocus() }
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    // Notes stored as plain text (legacy) are converted to minimal HTML so
    // newlines render correctly in RichText mode.
    function toRichText(s) {
        if (!s) return ""
        if (s.indexOf("<html") !== -1 || s.indexOf("<HTML") !== -1)
            return s
        return s.replace(/&/g, "&amp;")
                .replace(/</g, "&lt;")
                .replace(/>/g, "&gt;")
                .replace(/\n/g, "<br/>")
    }

    function scaleFontSizes(html, factor) {
        if (!html || html.indexOf("font-size") === -1) return html
        return html.replace(/font-size\s*:\s*(\d+(?:\.\d+)?)(pt|px)/gi,
            function(match, size, unit) {
                return "font-size:" + Math.round(parseFloat(size) * factor) + unit
            })
    }

    component SectionHeader: Label {
        Layout.fillWidth: true
        Layout.topMargin: 20
        leftPadding: 16
        bottomPadding: 4
        font.pixelSize: 13
        font.weight: 600
        color: Theme.navyMid
        background: Rectangle { color: Theme.sectionBg }
    }

    component FieldRow: Rectangle {
        default property alias content: innerItem.data
        Layout.fillWidth: true
        Layout.preferredHeight: 44
        color: palette.base
        Item { id: innerItem; anchors.fill: parent }
        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
            height: 1; color: palette.placeholderText; opacity: 0.3
        }
    }
}
