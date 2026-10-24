// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: {
        var base = qsTr("Note")
        if (root.projectId === "") return base
        return base + " — " + AppController.projectNumberForId(root.projectId)
                    + " " + AppController.projectNameForId(root.projectId).substring(0, 20)
    }

    property int    noteRow:         -1
    property string noteId:          ""
    property string projectId:       ""
    property string initialTitle:    ""
    property string initialDate:     ""
    property string initialNote:     ""
    property bool   initialInternal: false
    property bool   _skipSave:       false
    property bool   _hasChanges:     false
    property bool   isNewRecord:     false
    property bool   _formatSheetOpen: false
    property bool   _fontPickerOpen: false

    // Saved selection for format sheet (focus leaves noteEdit when sheet opens)
    property int _selStart: 0
    property int _selEnd:   0

    function _isBlankNew() { return isNewRecord && titleField.text.trim() === "" }
    function _discardNew()  {
        var row = AppController.rowForId(AppController.projectNotesModel, root.noteId)
        if (row < 0) return
        AppController.deleteProjectNote(row)
    }

    Component.onCompleted: {
        if (root.noteId !== "")
            AppController.setNoteFilter(root.noteId, root.projectId)
        // Reload fresh data from model and set text imperatively.
        // This breaks the binding on noteEdit.text so that subsequent
        // TextFormatter operations on the QTextDocument are never
        // overwritten by a binding re-evaluation.
        root._reloadData()
    }

    // Re-resolve noteRow from the stable noteId before every write — Sort/
    // refresh elsewhere in the app can reorder or reset the shared
    // projectNotesModel proxy while this page is open.
    function _saveNow() {
        if (!root._hasChanges) return true
        var row = AppController.rowForId(AppController.projectNotesModel, root.noteId)
        if (row < 0) return false   // record no longer exists
        root.noteRow = row
        dateField.commitPending()
        var result = AppController.saveProjectNote(root.noteRow, titleField.text, dateField.text,
                                                    root.scaleFontSizes(TextFormatter.documentHtml(noteEdit.textDocument), 1 / 1.5),
                                                    internalSwitch.checked)
        if (result) root._hasChanges = false
        return result
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
    // ── Actions ──────────────────────────────────────────────────────────────
    // Glass pill floating at the top right, over the form as it scrolls.
    NavButton {
        id: navButton
        anchors { left: parent.left; leftMargin: 12; verticalCenter: topBar.verticalCenter }
    }

    GlassBar {
        id: topBar
        z: 2
        anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 12 }
        height: 44
        width: topBarRow.implicitWidth + 2 * contentPadding
        contentPadding: 6
        blurSource: scrollView
        blurRect: Qt.rect(topBar.x - scrollView.x, topBar.y - scrollView.y,
                          topBar.width, topBar.height)

        RowLayout {
            id: topBarRow
            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
            spacing: 0
            BarButton {
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
                    var body = AppController.htmlToPlainText(TextFormatter.documentHtml(noteEdit.textDocument))
                    var actionItems = AppController.noteActionItemsSummary()
                    if (actionItems !== "")
                        body += "\n\nAction Items:\n" + actionItems
                    var attendees = AppController.attendeeNameList()
                    if (attendees !== "")
                        body += "\n\nAttendees:\n" + attendees
                    AppController.sendMeetingNotesEmail(emails, subject, body)
                }
            }

            BarButton {
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

            BarButton {
                icon.name: "trash"
                focusPolicy: Qt.NoFocus
                onClicked: {
                    var row = AppController.rowForId(AppController.projectNotesModel, root.noteId)
                    if (row >= 0 && AppController.deleteProjectNote(row)) {
                        root._skipSave = true
                        root.StackView.view.pop()
                    }
                }
            }
        }
    }

    // ── Footer: Attendees + Action Items ─────────────────────────────────────
    // ── Bottom bar ───────────────────────────────────────────────────────────
    // Glass capsule floating over the bottom of the form, like the tab bar.
    GlassBar {
        id: bottomBar
        z: 2
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right; bottomMargin: 8; leftMargin: 16; rightMargin: 16 }
        height: 60
        blurSource: scrollView
        blurRect: Qt.rect(bottomBar.x - scrollView.x, bottomBar.y - scrollView.y,
                          bottomBar.width, bottomBar.height)

        RowLayout {
            anchors.centerIn: parent
            spacing: 32

            BarButton {
                icon.name: "person.2"
                // text: qsTr("Attendees")
                display: AbstractButton.TextUnderIcon
                onClicked: {
                    if (!root._saveNow()) return
                    root.StackView.view.push(Qt.resolvedUrl("MeetingAttendeesPage.qml"), {
                        noteId:    root.noteId,
                        noteBody:  AppController.htmlToPlainText(TextFormatter.documentHtml(noteEdit.textDocument)).replace(/\n/g, '\r\n'),
                        noteTitle: titleField.text,
                        noteDate:  dateField.text,
                        projectId: root.projectId
                    })
                }
            }

            BarButton {
                icon.name: "checklist"
                // text: qsTr("Action Items")
                display: AbstractButton.TextUnderIcon
                onClicked: {
                    if (!root._saveNow()) return
                    root.StackView.view.push(Qt.resolvedUrl("NoteActionItemsPage.qml"), {
                        noteId:    root.noteId,
                        noteTitle: titleField.text,
                        noteDate:  dateField.text,
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
            // Room for the floating action pill above the first field.
            Item { Layout.preferredHeight: topBar.height + 6 }


            SectionHeader { text: qsTr("Title") }
            FieldRow {
                FormField {
                    id: titleField
                    text: root.initialTitle
                    inputMethodHints: Qt.ImhNoPredictiveText
                    onTextChanged: root._hasChanges = true
                }
            }

            SectionHeader { text: qsTr("Date") }
            DateFieldRow {
                id: dateField
                text: root.initialDate
                onTextChanged: root._hasChanges = true
            }

            SectionHeader { text: qsTr("Note") }

            // // Explicit Flickable wrapper preserves the touch-event routing that
            // // lets the TextEdit acquire keyboard focus when tapped on iOS.
            // // Word-wrap + auto-height means the Flickable never actually scrolls;
            // // the outer ScrollView handles all vertical scrolling.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(120, noteEdit.contentHeight + 40)
                color: "transparent"
                GlassField {
                    anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 6; bottomMargin: 6 }
                    focused: noteEdit.activeFocus
                }
                TextEdit {
                    id: noteEdit
                    anchors { fill: parent; leftMargin: 22; rightMargin: 22; topMargin: 14; bottomMargin: 14 }
                    color: palette.text
                    selectByMouse: true

                    text: root.toRichText(root.scaleFontSizes(root.initialNote, 1.5))
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.WordWrap
                    font.family: "Arial"
                    onTextChanged: root._hasChanges = true
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
                    onToggled: root._hasChanges = true
                }
            }

            // Extra space at the bottom so content is reachable above the keyboard
            // + keyboard accessory bar (44 px) when the keyboard is open.
            Item {
                Layout.preferredHeight: Qt.inputMethod.visible
                                        ? Qt.inputMethod.keyboardRectangle.height + 44 + 24
                                        : 24
            }
            // Room for the floating bottom bar below the last field.
            Item { Layout.preferredHeight: bottomBar.visible ? bottomBar.height + 16 : 0 }
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

        // padding: 0 on its own is not enough: the iOS style sets topPadding
        // (23) as well as padding, and a whole-control padding never overrides
        // a per-edge one. On a bar this short the leftover 23px halved the
        // usable height of the button row.
        padding: 0
        topPadding: 0
        leftPadding: 10
        rightPadding: 10

        // A glass capsule inset from the screen edges, floating just above
        // the keyboard like the tab bar floats above the lists.
        x: 8
        y: Qt.inputMethod.visible
           ? Qt.inputMethod.keyboardRectangle.y - height - 8
           : (parent ? parent.height - height - 8 : 0)
        width:  parent ? parent.width - 16 : 374
        height: 52

        visible: !_formatSheetOpen && !_fontPickerOpen && (Qt.inputMethod.visible || noteEdit.selectionStart !== noteEdit.selectionEnd)

        // Frosts the note text scrolling behind the bar. blurRect is the
        // bar's area in page coordinates (the popup lives in the overlay).
        // mapFromItem() isn't tracked by the binding, so it also re-runs on
        // open and on page resize: its first run comes before layout settles.
        background: GlassBar {
            blurSource: root
            blurRect: {
                keyboardAccessory.opened; root.width; root.height
                var p = root.mapFromItem(Overlay.overlay, keyboardAccessory.x, keyboardAccessory.y)
                return Qt.rect(p.x, p.y, keyboardAccessory.width, keyboardAccessory.height)
            }
        }

        contentItem: RowLayout {
            spacing: 4

            // Aa — open the Format sheet
            BarButton {
                text: "Aa"
                font.pixelSize: 18
                font.weight: Font.DemiBold
                focusPolicy: Qt.NoFocus
                onClicked: {
                    root._selStart = noteEdit.selectionStart
                    root._selEnd   = noteEdit.selectionEnd
                    // Collapse selection before open() triggers
                    // Qt.inputMethod.hide() (via onAboutToShow), which on iOS
                    // deletes selected text when the keyboard resigns.
                    noteEdit.select(root._selStart, root._selStart)
                    formatSheet.textDocument = noteEdit.textDocument
                    formatSheet.selStart     = root._selStart
                    formatSheet.selEnd       = root._selEnd
                    formatSheet.open()
                }
            }

            // Font — open advanced font picker
            BarButton {
                text: "ƒ"
                font.pixelSize: 18
                focusPolicy: Qt.NoFocus
                onClicked: {
                    root._selStart = noteEdit.selectionStart
                    root._selEnd   = noteEdit.selectionEnd
                    // Collapse selection (same iOS workaround as Aa button).
                    noteEdit.select(root._selStart, root._selStart)
                    var currentFamily = TextFormatter.currentFontFamily(noteEdit.textDocument, root._selStart)
                    var currentSize   = TextFormatter.currentFontPointSize(noteEdit.textDocument, root._selStart)
                    var currentColor  = TextFormatter.currentFontColor(noteEdit.textDocument, root._selStart)

                    fontPicker.openWithFormat(currentFamily, currentSize, currentColor)
                }
            }

            Rectangle { width: 1; height: 22; color: palette.mid; opacity: 0.4; Layout.alignment: Qt.AlignVCenter }

            // Font size bump controls
            BarButton {
                text: "A+"; font.pixelSize: 18; font.weight: Font.DemiBold; focusPolicy: Qt.NoFocus
                onClicked: {
                    var ss = noteEdit.selectionStart
                    var se = noteEdit.selectionEnd
                    TextFormatter.increaseFontSize(noteEdit.textDocument, ss, se)
                    noteEdit.forceActiveFocus()
                    noteEdit.select(ss, se)
                }
            }
            BarButton {
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

            BarButton {
                icon.name: "list.bullet"; focusPolicy: Qt.NoFocus
                onClicked: {
                    TextFormatter.applyStyle(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd, 1)
                    noteEdit.forceActiveFocus()
                }
            }
            BarButton {
                icon.name: "list.number"; focusPolicy: Qt.NoFocus
                onClicked: {
                    TextFormatter.applyStyle(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd, 4)
                    noteEdit.forceActiveFocus()
                }
            }

            Rectangle { width: 1; height: 22; color: palette.mid; opacity: 0.4; Layout.alignment: Qt.AlignVCenter }

            BarButton {
                text: "⇥"; font.pixelSize: 18; focusPolicy: Qt.NoFocus
                onClicked: {
                    TextFormatter.indentText(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd)
                    noteEdit.forceActiveFocus()
                }
            }
            BarButton {
                text: "⇤"; font.pixelSize: 18; focusPolicy: Qt.NoFocus
                onClicked: {
                    TextFormatter.unindentText(noteEdit.textDocument, noteEdit.selectionStart, noteEdit.selectionEnd)
                    noteEdit.forceActiveFocus()
                }
            }

            Item { Layout.fillWidth: true }

            // Dismiss keyboard
            BarButton {
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
        // Detect any common HTML tag — covers full documents (<html>)
        // and fragments (<p>, <body>, <div>, <b>, <li>, etc.).
        if (/<(html|body|p|div|span|b|i|u|h[1-6]|ul|ol|li|br|table|tr|td|th|a|img|blockquote|pre|code|style)[\s>]/i.test(s))
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
        Layout.topMargin: 10
        leftPadding: 16
        bottomPadding: 4
        font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
        font.weight: 600
        color: Theme.sectionText
    }
}
