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

    Component.onCompleted: {
        if (root.noteId !== "")
            AppController.setNoteFilter(root.noteId, root.projectId)
    }

    function _saveNow() {
        dateField.commitPending()
        AppController.saveProjectNote(root.noteRow, titleField.text, dateField.text,
                                      TextFormatter.documentHtml(noteEdit.textDocument),
                                      internalSwitch.checked)
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

    // Selection range saved before the font picker opens (opening the popup
    // shifts focus away from noteEdit, clearing its selection).
    property int _fontSelStart: 0
    property int _fontSelEnd:   0

    // ── Formatting toolbar (Page header) ──────────────────────────────────────
    header: ToolBar {
        implicitHeight: 88  // two rows × 44 px

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ── Row 1: style, character formatting, font size ─────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                spacing: 0

                Item { implicitWidth: 4 }
                ComboBox {
                    id: styleCombo
                    focusPolicy: Qt.NoFocus
                    implicitWidth: 140
                    popup.width: 220
                    popup.x: 0
                    model: [
                        qsTr("Standard"),
                        qsTr("Bullet (Disc)"),
                        qsTr("Bullet (Circle)"),
                        qsTr("Bullet (Square)"),
                        qsTr("Ordered (1 2 3)"),
                        qsTr("Ordered (a b c)"),
                        qsTr("Ordered (A B C)"),
                        qsTr("Ordered (i ii iii)"),
                        qsTr("Ordered (I II III)"),
                        qsTr("Heading 1"),
                        qsTr("Heading 2"),
                        qsTr("Heading 3"),
                        qsTr("Heading 4"),
                        qsTr("Heading 5"),
                        qsTr("Heading 6")
                    ]
                    currentIndex: 0
                    onActivated: function(idx) {
                        TextFormatter.applyStyle(noteEdit.textDocument,
                                                 noteEdit.selectionStart,
                                                 noteEdit.selectionEnd, idx)
                        noteEdit.forceActiveFocus()
                        Qt.inputMethod.hide()
                        currentIndex = 0   // reset so re-selecting same style always fires
                    }
                }
                ToolButton { text: qsTr("B");  font.bold: true;      font.pixelSize: 16; focusPolicy: Qt.NoFocus
                    onClicked: { TextFormatter.toggleBold(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }
                ToolButton { text: qsTr("I");  font.italic: true;    font.pixelSize: 16; focusPolicy: Qt.NoFocus
                    onClicked: { TextFormatter.toggleItalic(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }
                ToolButton { text: qsTr("U");  font.underline: true; font.pixelSize: 16; focusPolicy: Qt.NoFocus
                    onClicked: { TextFormatter.toggleUnderline(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }
                ToolButton { text: qsTr("A+"); font.pixelSize: 14;   focusPolicy: Qt.NoFocus
                    onClicked: { TextFormatter.increaseFontSize(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }
                ToolButton { text: qsTr("A−"); font.pixelSize: 12;   focusPolicy: Qt.NoFocus
                    onClicked: { TextFormatter.decreaseFontSize(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }
                Item { Layout.fillWidth: true }

                ToolButton {
                    icon.name: "doc.on.doc"
                    focusPolicy: Qt.NoFocus
                    onClicked: {
                        root._saveNow()
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

            // Divider between rows
            Rectangle { Layout.fillWidth: true; height: 1; color: palette.placeholderText; opacity: 0.3 }

            // ── Row 2: indent / unindent, alignment ───────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 43
                spacing: 0

                Item { implicitWidth: 4; Layout.alignment: Qt.AlignVCenter }
                // Indent / unindent
                ToolButton { text: qsTr("⇥");  font.pixelSize: 16; focusPolicy: Qt.NoFocus; Layout.alignment: Qt.AlignVCenter
                    onClicked: { TextFormatter.indentText(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }
                ToolButton { text: qsTr("⇤");  font.pixelSize: 16; focusPolicy: Qt.NoFocus; Layout.alignment: Qt.AlignVCenter
                    onClicked: { TextFormatter.unindentText(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }

                // Separator
                Rectangle { width: 1; height: 24; color: palette.placeholderText; opacity: 0.4; Layout.alignment: Qt.AlignVCenter }

                // Alignment: 0=left 1=center 2=right 3=justify
                ToolButton { icon.name: "text.alignleft";   focusPolicy: Qt.NoFocus; Layout.alignment: Qt.AlignVCenter
                    onClicked: { TextFormatter.setAlignment(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd, 0); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }
                ToolButton { icon.name: "text.aligncenter"; focusPolicy: Qt.NoFocus; Layout.alignment: Qt.AlignVCenter
                    onClicked: { TextFormatter.setAlignment(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd, 1); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }
                ToolButton { icon.name: "text.alignright";  focusPolicy: Qt.NoFocus; Layout.alignment: Qt.AlignVCenter
                    onClicked: { TextFormatter.setAlignment(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd, 2); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }
                ToolButton { icon.name: "text.justify";     focusPolicy: Qt.NoFocus; Layout.alignment: Qt.AlignVCenter
                    onClicked: { TextFormatter.setAlignment(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd, 3); noteEdit.forceActiveFocus(); Qt.inputMethod.hide() } }

                // Separator
                Rectangle { width: 1; height: 24; color: palette.placeholderText; opacity: 0.4; Layout.alignment: Qt.AlignVCenter }

                // Font family / size / color picker
                ToolButton {
                    text: qsTr("Font…")
                    font.pixelSize: 13
                    focusPolicy: Qt.NoFocus
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: {
                        root._fontSelStart = noteEdit.selectionStart
                        root._fontSelEnd   = noteEdit.selectionEnd
                        var family = TextFormatter.currentFontFamily(noteEdit.textDocument, root._fontSelStart)
                        var size   = TextFormatter.currentFontPointSize(noteEdit.textDocument, root._fontSelStart)
                        var col    = TextFormatter.currentFontColor(noteEdit.textDocument, root._fontSelStart)
                        fontPicker.openWithFormat(family, size, col)
                    }
                }

                Item { Layout.fillWidth: true }
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
                    root._saveNow()
                    root.StackView.view.push(Qt.resolvedUrl("MeetingAttendeesPage.qml"), {
                        noteId: root.noteId
                    })
                }
            }

            ToolButton {
                icon.name: "checklist"
                text: qsTr("Action Items")
                display: AbstractButton.TextUnderIcon
                onClicked: {
                    root._saveNow()
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
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Date") }
            DateFieldRow { id: dateField; text: root.initialDate }

            SectionHeader { text: qsTr("Note") }

            // The note area grows with content; minimum 300px so it feels like
            // a real editor even for short notes.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(300, noteEdit.contentHeight + 24)
                color: palette.base

                TextEdit {
                    id: noteEdit
                    anchors { fill: parent; margins: 8 }
                    text: root.toRichText(root.initialNote)
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.Wrap
                    font.pixelSize: 15
                    color: palette.windowText   // default for unstyled text; HTML explicit colors still override
                    selectByMouse: true
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

            Item { Layout.preferredHeight: 24 }
        }
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    // Mirror the desktop ProjectNotesDelegate logic: notes that were saved
    // before rich-text editing existed (or edited on other tools) are stored
    // as plain text with \n line endings.  A RichText TextEdit treats \n as
    // whitespace, collapsing all the newlines.  Convert those notes to minimal
    // HTML so newlines render correctly; pass through anything that already
    // looks like HTML unchanged.
    function toRichText(s) {
        if (!s) return ""
        if (s.indexOf("<html") !== -1 || s.indexOf("<HTML") !== -1) {
            // In dark mode, replace explicitly dark text colors with the system text color
            // so notes typed in light mode don't become invisible.
            // if (palette.window.hslLightness < 0.5) {
            //     var light = palette.windowText.toString()
            //     return s.replace(/color\s*:\s*#([0-9a-fA-F]{6})/gi, function(match, hex) {
            //         var luma = (0.299 * parseInt(hex.substring(0,2),16)
            //                   + 0.587 * parseInt(hex.substring(2,4),16)
            //                   + 0.114 * parseInt(hex.substring(4,6),16)) / 255
            //         return luma < 0.4 ? ("color:" + light) : match
            //     })
            // }
            return s
        }
        return s.replace(/&/g, "&amp;")
                .replace(/</g, "&lt;")
                .replace(/>/g, "&gt;")
                .replace(/\n/g, "<br/>")
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

    // ── Font picker popup ─────────────────────────────────────────────────────
    FontPickerPopup {
        id: fontPicker
        parent: Overlay.overlay
        onFontApplied: function(family, pointSize, textColor) {
            TextFormatter.applyFontFamily(noteEdit.textDocument,
                                          root._fontSelStart, root._fontSelEnd, family)
            TextFormatter.applyFontPointSize(noteEdit.textDocument,
                                             root._fontSelStart, root._fontSelEnd, pointSize)
            TextFormatter.applyFontColor(noteEdit.textDocument,
                                         root._fontSelStart, root._fontSelEnd, textColor)
            noteEdit.forceActiveFocus()
            Qt.inputMethod.hide()
        }
    }
}
