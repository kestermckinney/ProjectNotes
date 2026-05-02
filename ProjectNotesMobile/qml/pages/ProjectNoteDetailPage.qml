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
    property bool   isNewRecord:     false
    property bool   _formatSheetOpen: false
    property bool   _fontPickerOpen: false

    // Saved selection for format sheet (focus leaves noteEdit when sheet opens)
    property int _selStart: 0
    property int _selEnd:   0

    function _isBlankNew() { return isNewRecord && titleField.text.trim() === "" }
    function _discardNew()  { AppController.deleteProjectNote(root.noteRow) }

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
                // text: qsTr("Attendees")
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
                // text: qsTr("Action Items")
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
                FormField {
                    id: titleField
                    text: root.initialTitle
                    inputMethodHints: Qt.ImhNoPredictiveText
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

                    text: root.toRichText(root.scaleFontSizes(root.initialNote, 1.5))
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.WordWrap
                    font.family: "Arial"
                }
                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1; color: Theme.mutedText; opacity: 0.3
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
        height: 48

        visible: !_formatSheetOpen && !_fontPickerOpen && (Qt.inputMethod.visible || noteEdit.selectionStart !== noteEdit.selectionEnd)

        background: Rectangle {
            radius: 16
            color: Qt.rgba(palette.window.r, palette.window.g, palette.window.b, 0.97)
            Rectangle {
                anchors { top: parent.top; left: parent.left; right: parent.right }
                height: 1; color: palette.mid; opacity: 0.4
            }
        }

        contentItem: RowLayout {
            anchors { fill: parent; leftMargin: 4; rightMargin: 4 }
            spacing: 4

            // Aa — open the Format sheet
            ToolButton {
                text: "Aa"
                font.pixelSize: 18
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

            // Font — open advanced font picker
            ToolButton {
                text: "ƒ"
                font.pixelSize: 18
                focusPolicy: Qt.NoFocus
                onClicked: {
                    root._selStart = noteEdit.selectionStart
                    root._selEnd   = noteEdit.selectionEnd
                    var currentFamily = TextFormatter.currentFontFamily(noteEdit.textDocument, root._selStart)
                    var currentSize   = TextFormatter.currentFontPointSize(noteEdit.textDocument, root._selStart)
                    var currentColor  = TextFormatter.currentFontColor(noteEdit.textDocument, root._selStart)
                    console.info("family: ", currentFamily, " size: ", currentSize, " color: ", currentColor)
                    fontPicker.openWithFormat(currentFamily, currentSize, currentColor)
                }
            }

            Rectangle { width: 1; height: 22; color: palette.mid; opacity: 0.4; Layout.alignment: Qt.AlignVCenter }

            // Font size bump controls
            ToolButton {
                text: "A+"; font.pixelSize: 18; font.weight: Font.DemiBold; focusPolicy: Qt.NoFocus
                onClicked: {
                    var ss = noteEdit.selectionStart
                    var se = noteEdit.selectionEnd
                    TextFormatter.increaseFontSize(noteEdit.textDocument, ss, se)
                    noteEdit.forceActiveFocus()
                    noteEdit.select(ss, se)
                }
            }
            ToolButton {
                text: "A-"; font.pixelSize: 18; font.weight: Font.DemiBold; focusPolicy: Qt.NoFocus
                onClicked: {
                    var ss = noteEdit.selectionStart
                    var se = noteEdit.selectionEnd
                    TextFormatter.decreaseFontSize(noteEdit.textDocument, ss, se)
                    noteEdit.forceActiveFocus()
                    noteEdit.select(ss, se)
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
                text: "⇥"; font.pixelSize: 18; focusPolicy: Qt.NoFocus
                onClicked: {
                    TextFormatter.indentText(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd)
                    noteEdit.forceActiveFocus()
                }
            }
            ToolButton {
                text: "⇤"; font.pixelSize: 18; focusPolicy: Qt.NoFocus
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
        onClosed: {
            root._formatSheetOpen = false
            noteEdit.forceActiveFocus()
            noteEdit.select(root._selStart, root._selEnd)
        }
    }

    // ── Advanced font picker ──────────────────────────────────────────────────
    FontPickerPopup {
        id: fontPicker
        parent: Overlay.overlay
        scaleFactor: 1.5
        onOpened: root._fontPickerOpen = true
        onClosed: {
            root._fontPickerOpen = false
            noteEdit.forceActiveFocus()
            noteEdit.select(root._selStart, root._selEnd)
        }
        onFontApplied: function(family, pointSize, textColor) {
            TextFormatter.applyFontFamily(noteEdit.textDocument, root._selStart, root._selEnd, family)
            TextFormatter.applyFontPointSize(noteEdit.textDocument, root._selStart, root._selEnd, pointSize)
            TextFormatter.applyFontColor(noteEdit.textDocument, root._selStart, root._selEnd, textColor)
        }
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
}
