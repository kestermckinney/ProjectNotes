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

    // Consumed by the TopBar's Delete action (Main.deleteCurrent()) and Export
    // button, and by IconRail's hamburger menu (Main.qml binds these into
    // AppMenu's pluginMenuTable/pluginMenuRecordId) so plugins whose dataexport
    // is "project_notes" show up while a note is open — same convention as
    // ItemDetailPage.
    readonly property bool canDelete: true
    readonly property string exportTable: "project_notes"
    readonly property string exportId: noteId

    // Routed to Main's confirm-delete flow (root.confirmDelete()), same as
    // ProjectDetailPage — fired by the page-level kebab/right-click menu below.
    // The TopBar's own Delete button instead calls _deleteRecord() directly via
    // canDelete/Main.deleteCurrent().
    signal deleteRequested()

    // Delete this note. Removes the row from the shared project-notes model, so the
    // project's Notes tab refreshes itself once Main pops back to it.
    function _deleteRecord() {
        return DesktopAppController.deleteProjectNote(page.noteRow)
    }

    // People list for the action-item Assigned/Identified combos.
    property var    _people: []
    function _peopleNames() { return _people.map(function(p){ return p.name }) }
    function _idForName(n){ for (var i=0;i<_people.length;i++) if (_people[i].name===n) return _people[i].id; return "" }
    function _nameForId(id){ for (var i=0;i<_people.length;i++) if (_people[i].id===id) return _people[i].name; return "" }

    Component.onCompleted: {
        // Action-item Assigned/Identified combos list only this project's team
        // members (mirrors the Widgets team combos). _edit() re-loads this with the
        // row's current values included so an existing assignment keeps displaying.
        _people = DesktopAppController.teamMemberList(page.projectId)
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
        else _reload()   // revert fields to last valid values when an edit is rejected
        return ok
    }

    // Open the note's own record/plugin menu (kebab or right-click) at scene
    // coords — parity with ProjectDetailPage._openSelfMenu().
    function _openSelfMenu(sx, sy) {
        selfMenu.recordLabel = (titleField.text || "").toString()
        selfMenu.openAt(sx, sy)
    }

    // Right-click anywhere on the page background opens the note menu.
    // Declared beneath the page content so field/button clicks still reach
    // their own handlers first — see ProjectDetailPage's identical pattern.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: (mouse) => page._openSelfMenu(mouse.x, mouse.y)
    }

    // Ctrl+F opens the note's find & replace bar while this page is active.
    Shortcut {
        sequences: [ StandardKey.Find ]
        onActivated: findBar.open()
    }

    ScrollView {
        id: pageScroll
        anchors.fill: parent
        anchors.margins: 13
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: pageScroll.availableWidth
            spacing: 10

            // Title + date + internal
            RowLayout {
                Layout.fillWidth: true
                spacing: 9
                FormField {
                    label: qsTr("Title")
                    id: titleField
                    spellCheck: true
                    spellDialog: spellDialog
                    onEdited: page._changed = true
                }
                DateField {
                    label: qsTr("Date")
                    id: dateField
                    Layout.preferredWidth: 160
                    Layout.maximumWidth: 160
                    onEdited: page._changed = true
                }
                // Page-level record menu — exposes plugins whose dataexport is
                // "project_notes" (Export/Send Meeting Notes/etc.), plus
                // Delete/Export/Refresh. Parity with ProjectDetailPage's kebab.
                KebabButton {
                    Layout.alignment: Qt.AlignBottom
                    Layout.bottomMargin: 5
                    onClicked: (sx, sy) => page._openSelfMenu(sx, sy)
                }
            }

            RowLayout {
                spacing: 7
                CheckBox {
                    id: internalCheck
                    onToggled: page._changed = true
                    indicator: Rectangle {
                        implicitWidth: 16; implicitHeight: 16
                        radius: 4
                        x: internalCheck.leftPadding
                        y: parent.height / 2 - height / 2
                        color: internalCheck.checked ? Theme.accent : Theme.surface
                        border.color: internalCheck.checked ? Theme.accent : Theme.border
                        MaterialIcon {
                            anchors.centerIn: parent
                            visible: internalCheck.checked
                            name: "check"; size: 12; color: "#ffffff"
                        }
                    }
                    contentItem: Text {
                        text: qsTr("Internal item")
                        color: Theme.text
                        font.pixelSize: Theme.fontBody
                        leftPadding: internalCheck.indicator.width + 7
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // Note body: toolbar + editor
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Text {
                    text: qsTr("Note")
                    color: Theme.text3
                    font.pixelSize: Theme.fontXs
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }
                // Find & Replace toggle (also Ctrl+F while editing the note).
                Rectangle {
                    implicitHeight: 22
                    implicitWidth: frRow.implicitWidth + 12
                    radius: Theme.radiusSm
                    color: findBar.visible ? Theme.accentSoft : (frHover.hovered ? Theme.surface2 : "transparent")
                    RowLayout {
                        id: frRow
                        anchors.centerIn: parent
                        spacing: 3
                        MaterialIcon { name: "find_replace"; size: 12; color: findBar.visible ? Theme.accent : Theme.text2 }
                        Text {
                            text: qsTr("Find / Replace")
                            color: findBar.visible ? Theme.accent : Theme.text2
                            font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold
                        }
                    }
                    HoverHandler { id: frHover }
                    TapHandler { onTapped: findBar.toggle() }
                }
            }
            FindReplaceBar {
                id: findBar
                editor: noteEdit
            }
            NoteFormatToolbar {
                Layout.fillWidth: true
                editor: noteEdit
                dialog: spellDialog
                spell: noteSpell.spell
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(190, noteEdit.contentHeight + 32)
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: noteEdit.activeFocus ? Theme.accent : Theme.border
                TextArea {
                    id: noteEdit
                    anchors.fill: parent
                    anchors.margins: 9
                    anchors.bottomMargin: 14
                    color: Theme.text
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.WordWrap
                    selectByMouse: true
                    persistentSelection: true
                    background: null
                    font.family: "Arial"
                    // Match TextFormatter's Normal Text style exactly. Using
                    // pixelSize here made the unformatted default (~9pt at
                    // 96 DPI) visibly smaller than Normal Text's 12pt.
                    font.pointSize: 12
                    font.weight: Font.Normal
                    onTextChanged: page._changed = true

                    // Inline spell-check: red squiggle + right-click suggestions +
                    // "Check Spelling…" (opens the shared full-field dialog).
                    SpellCheckField { id: noteSpell; dialog: spellDialog }
                }
            }

            // Panels
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 5
                spacing: 10

                // Attendees
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    implicitHeight: attCol.implicitHeight + 18
                    ColumnLayout {
                        id: attCol
                        anchors.fill: parent
                        anchors.margins: 9
                        spacing: 6
                        RowLayout {
                            Layout.fillWidth: true
                            MaterialIcon { name: "groups"; size: 16; color: Theme.text2 }
                            Text {
                                text: qsTr("Attendees"); color: Theme.text
                                font.pixelSize: Theme.fontLg; font.weight: Font.Bold
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
                                id: attRow
                                required property int index
                                required property var model
                                Layout.fillWidth: true
                                spacing: 6
                                function _menu(sx, sy) {
                                    rowMenu.openFor(DesktopAppController.meetingAttendeesModel,
                                        (attRow.model.id || "").toString(), qsTr("Attendee"),
                                        (attRow.model.name || "").toString(), sx, sy,
                                        /*allowMoveTo*/ false,
                                        (attRow.model.person_id || "").toString())
                                }
                                TapHandler {
                                    acceptedButtons: Qt.RightButton
                                    onTapped: (ev) => attRow._menu(ev.scenePosition.x, ev.scenePosition.y)
                                }
                                Rectangle {
                                    implicitWidth: 22; implicitHeight: 22; radius: 11
                                    color: Theme.accentSoft
                                    Text {
                                        anchors.centerIn: parent
                                        text: {
                                            var n = (model.name || "").toString().trim()
                                            if (n === "") return "?"
                                            var parts = n.split(" ")
                                            return (parts[0][0] || "") + (parts.length > 1 ? parts[parts.length-1][0] : "")
                                        }
                                        color: Theme.accent; font.pixelSize: Theme.font2xs; font.weight: Font.Bold
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Text {
                                        text: (model.name || qsTr("(no name)")).toString()
                                        color: Theme.text; font.pixelSize: Theme.fontBody; elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: (model.email || "").toString()
                                        visible: text !== ""
                                        color: Theme.text3; font.pixelSize: Theme.fontXs; elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                                KebabButton {
                                    implicitWidth: 22; implicitHeight: 22
                                    Layout.alignment: Qt.AlignVCenter
                                    onClicked: (sx, sy) => attRow._menu(sx, sy)
                                }
                                DeleteButton {
                                    onClicked: {
                                        DesktopAppController.deleteAttendee(attRow.index)
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
                    implicitHeight: aiCol.implicitHeight + 18
                    ColumnLayout {
                        id: aiCol
                        anchors.fill: parent
                        anchors.margins: 9
                        spacing: 6
                        RowLayout {
                            Layout.fillWidth: true
                            MaterialIcon { name: "task_alt"; size: 16; color: Theme.text2 }
                            Text {
                                text: qsTr("Action Items"); color: Theme.text
                                font.pixelSize: Theme.fontLg; font.weight: Font.Bold
                                Layout.fillWidth: true
                            }
                            SmallAddButton {
                                text: qsTr("Add")
                                onClicked: {
                                    page._saveNow()
                                    // Adding a row refreshes the model, which rebuilds
                                    // every delegate — flush each row's uncommitted
                                    // inline edits first so a name typed but not yet
                                    // blurred isn't wiped when its delegate is rebuilt.
                                    for (var i = 0; i < aiRepeater.count; ++i) {
                                        var d = aiRepeater.itemAt(i)
                                        if (d) d._commitPending()
                                    }
                                    DesktopAppController.addNoteActionItem(page.noteId, page.projectId)
                                    DesktopAppController.refreshNoteActionItems()
                                }
                            }
                        }
                        Repeater {
                            id: aiRepeater
                            model: DesktopAppController.notesActionItemsModel
                            delegate: ColumnLayout {
                                id: ai
                                required property int index
                                required property var model
                                property bool expanded: false
                                property string _assignedId: ""
                                property string _identifiedId: ""
                                // NotesActionItemsModel does no date bookkeeping (unlike
                                // TrackerItemsModel), so this editor stamps Date Updated /
                                // Date Resolved itself, mirroring that model's rules. Flags
                                // track a hand-typed date so it isn't overwritten; _prevStatus
                                // spots a move into or out of "Resolved". All reset by _edit().
                                property string _prevStatus: ""
                                property bool _lastUpdateEdited: false
                                property bool _resolvedEdited: false
                                Layout.fillWidth: true
                                spacing: 5

                                // Pull the current model values into the editors, then open.
                                // The name lives in the always-visible inline field, so it is
                                // not re-populated here.
                                function _edit() {
                                    aiType.value      = (ai.model.item_type || "").toString()
                                    aiPriority.value  = (ai.model.priority || "").toString()
                                    aiStatus.value    = (ai.model.status || "").toString()
                                    ai._assignedId    = (ai.model.assigned_to || "").toString()
                                    ai._identifiedId  = (ai.model.identified_by || "").toString()
                                    // Refresh the team list so this row's current
                                    // people are present even if they left the team.
                                    page._people = DesktopAppController.teamMemberList(
                                        page.projectId, [ai._assignedId, ai._identifiedId])
                                    aiAssigned.value  = page._nameForId(ai._assignedId)
                                    aiIdentified.value= page._nameForId(ai._identifiedId)
                                    aiDateId.text     = (ai.model.date_identified || "").toString()
                                    aiDateDue.text    = (ai.model.date_due || "").toString()
                                    aiLastUpdate.text = (ai.model.last_update || "").toString()
                                    aiDateResolved.text = (ai.model.date_resolved || "").toString()
                                    aiDesc.text       = (ai.model.description || "").toString()
                                    ai._prevStatus       = (ai.model.status || "").toString()
                                    ai._lastUpdateEdited = false
                                    ai._resolvedEdited   = false
                                    ai.expanded = true
                                }

                                function _today() { return Qt.formatDate(new Date(), "MM/dd/yyyy") }

                                // Stamp Date Updated to today for any edit (unless hand-typed).
                                // Call before ai._save() so the new value is persisted.
                                function _touchDates() {
                                    if (!ai._lastUpdateEdited)
                                        aiLastUpdate.setText(ai._today())
                                }

                                // Moving Status into "Resolved" stamps Date Resolved; moving
                                // out clears it. Skipped if the user hand-typed that field.
                                function _applyStatusDates(newStatus) {
                                    if (!ai._resolvedEdited) {
                                        if (newStatus === "Resolved" && ai._prevStatus !== "Resolved")
                                            aiDateResolved.setText(ai._today())
                                        else if (newStatus !== "Resolved" && ai._prevStatus === "Resolved")
                                            aiDateResolved.setText("")
                                    }
                                    ai._prevStatus = newStatus
                                }
                                // Persist all fields (in-place setData → summary updates live,
                                // no refresh, so the editor stays open). Uses the editor values,
                                // which _edit() populated from the model.
                                function _save() {
                                    DesktopAppController.saveNoteActionItem(ai.index,
                                        aiNameInline.text, aiType.value, aiPriority.value, aiStatus.value,
                                        ai._assignedId, ai._identifiedId, aiDateId.text, aiDateDue.text,
                                        aiDesc.text, aiLastUpdate.text, aiDateResolved.text)
                                }
                                // Persist an inline name edit without disturbing the other fields:
                                // read everything except the name straight from the model, so this
                                // is safe even when the editor was never expanded.
                                function _saveName() {
                                    // Only bump Date Updated when the name really changed —
                                    // onEditingFinished also fires on a focus-out with no edit.
                                    var nameChanged = aiNameInline.text !== (ai.model.item_name || "").toString()
                                    DesktopAppController.saveNoteActionItem(ai.index,
                                        aiNameInline.text,
                                        (ai.model.item_type || "").toString(),
                                        (ai.model.priority || "").toString(),
                                        (ai.model.status || "").toString(),
                                        (ai.model.assigned_to || "").toString(),
                                        (ai.model.identified_by || "").toString(),
                                        (ai.model.date_identified || "").toString(),
                                        (ai.model.date_due || "").toString(),
                                        (ai.model.description || "").toString(),
                                        nameChanged ? ai._today() : (ai.model.last_update || "").toString(),
                                        (ai.model.date_resolved || "").toString())
                                }
                                // Flush edits that are still only in the editors (not
                                // yet saved on blur) before a model refresh rebuilds
                                // this delegate. If the inline editor is open, save the
                                // whole row; otherwise just persist the inline name.
                                function _commitPending() {
                                    if (ai.expanded)
                                        ai._save()
                                    else if (aiNameInline.text !== (ai.model.item_name || "").toString())
                                        ai._saveName()
                                }
                                function _menu(sx, sy) {
                                    rowMenu.openFor(DesktopAppController.notesActionItemsModel,
                                        (ai.model.id || "").toString(), qsTr("Action Item"),
                                        (ai.model.item_name || "").toString(), sx, sy,
                                        /*allowMoveTo*/ true)
                                }
                                TapHandler {
                                    acceptedButtons: Qt.RightButton
                                    onTapped: (ev) => ai._menu(ev.scenePosition.x, ev.scenePosition.y)
                                }

                                // Summary row (click to expand/collapse the editor)
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    MaterialIcon {
                                        name: {
                                            var s = (ai.model.status || "").toString().toLowerCase()
                                            if (s.indexOf("resolved") >= 0 || s.indexOf("closed") >= 0) return "check_circle"
                                            return "radio_button_unchecked"
                                        }
                                        size: 14
                                        color: {
                                            var s = (ai.model.status || "").toString().toLowerCase()
                                            return (s.indexOf("resolved") >= 0 || s.indexOf("closed") >= 0)
                                                   ? Theme.green : Theme.text3
                                        }
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 0
                                        // Inline, always-editable name — type it without expanding.
                                        TextField {
                                            id: aiNameInline
                                            Layout.fillWidth: true
                                            color: Theme.text
                                            font.pixelSize: Theme.fontBody
                                            horizontalAlignment: Text.AlignLeft
                                            background: null
                                            padding: 0
                                            selectByMouse: true
                                            placeholderText: qsTr("(unnamed item)")
                                            placeholderTextColor: Theme.text3
                                            Component.onCompleted: {
                                                text = (ai.model.item_name || "").toString()
                                                // TextInput scrolls to keep the cursor visible; setting
                                                // text moves the cursor to the end, which can leave a
                                                // long name scrolled so it appears right-aligned.
                                                cursorPosition = 0
                                            }
                                            onEditingFinished: ai._saveName()
                                        }
                                        Text {
                                            text: {
                                                var p = (ai.model.priority || "").toString()
                                                var a = (ai.model.assigned_to || "").toString()
                                                var who = a !== "" ? DesktopAppController.peopleNameForId(a) : ""
                                                return [p, who].filter(function(x){ return x !== "" }).join(" · ")
                                            }
                                            visible: text !== ""
                                            color: Theme.text3; font.pixelSize: Theme.fontXs; elide: Text.ElideRight
                                            horizontalAlignment: Text.AlignLeft
                                            Layout.fillWidth: true
                                        }
                                    }
                                    // Edit / collapse toggle
                                    Rectangle {
                                        implicitWidth: 24; implicitHeight: 24; radius: Theme.radiusSm
                                        color: eHover.hovered ? Theme.surface2 : "transparent"
                                        Layout.alignment: Qt.AlignVCenter
                                        MaterialIcon {
                                            anchors.centerIn: parent
                                            name: ai.expanded ? "expand_less" : "edit"
                                            size: 13; color: Theme.text2
                                        }
                                        HoverHandler { id: eHover }
                                        TapHandler { onTapped: { if (ai.expanded) { ai._save(); ai.expanded = false } else ai._edit() } }
                                    }
                                    KebabButton {
                                        implicitWidth: 24; implicitHeight: 24
                                        Layout.alignment: Qt.AlignVCenter
                                        onClicked: (sx, sy) => ai._menu(sx, sy)
                                    }
                                    DeleteButton {
                                        onClicked: {
                                            DesktopAppController.deleteNoteActionItem(ai.index)
                                            DesktopAppController.refreshNoteActionItems()
                                        }
                                    }
                                }

                                // Inline editor — all editable action-item fields.
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 20
                                    visible: ai.expanded
                                    spacing: 6

                                    GridLayout {
                                        Layout.fillWidth: true; columns: 2; columnSpacing: 9; rowSpacing: 6
                                        ComboField {
                                            id: aiType; label: qsTr("Type")
                                            options: DesktopAppController.itemTypeOptions()
                                            onActivated: { ai._touchDates(); ai._save() }
                                        }
                                        ComboField {
                                            id: aiPriority; label: qsTr("Priority")
                                            options: DesktopAppController.itemPriorityOptions()
                                            onActivated: { ai._touchDates(); ai._save() }
                                        }
                                        ComboField {
                                            id: aiStatus; label: qsTr("Status")
                                            options: DesktopAppController.itemStatusOptions()
                                            onActivated: (v) => { ai._applyStatusDates(v); ai._touchDates(); ai._save() }
                                        }
                                        ComboField {
                                            id: aiAssigned; label: qsTr("Assigned To")
                                            options: page._peopleNames()
                                            includeNone: true
                                            searchable: true
                                            onActivated: (v) => { ai._assignedId = page._idForName(v); ai._touchDates(); ai._save() }
                                        }
                                        ComboField {
                                            id: aiIdentified; label: qsTr("Identified By")
                                            options: page._peopleNames()
                                            includeNone: true
                                            searchable: true
                                            onActivated: (v) => { ai._identifiedId = page._idForName(v); ai._touchDates(); ai._save() }
                                        }
                                        DateField { id: aiDateId; label: qsTr("Date Identified"); onEdited: { ai._touchDates(); ai._save() } }
                                        DateField { id: aiDateDue; label: qsTr("Date Due"); onEdited: { ai._touchDates(); ai._save() } }
                                        DateField { id: aiLastUpdate; label: qsTr("Date Updated"); onEdited: { ai._lastUpdateEdited = true; ai._save() } }
                                        DateField { id: aiDateResolved; label: qsTr("Date Resolved"); onEdited: { ai._resolvedEdited = true; ai._touchDates(); ai._save() } }
                                    }
                                    Text { text: qsTr("Description"); color: Theme.text3; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold }
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: Math.max(56, aiDesc.contentHeight + 14)
                                        radius: Theme.radiusSm
                                        color: Theme.surface
                                        border.color: aiDesc.activeFocus ? Theme.accent : Theme.border
                                        TextArea {
                                            id: aiDesc
                                            anchors.fill: parent
                                            anchors.margins: 7
                                            color: Theme.text
                                            wrapMode: TextEdit.WordWrap
                                            selectByMouse: true
                                            background: null
                                            font.pixelSize: Theme.fontBody
                                            onEditingFinished: {
                                                // Fires on focus-out too — only stamp on a real edit.
                                                if (aiDesc.text !== (ai.model.description || "").toString())
                                                    ai._touchDates()
                                                ai._save()
                                            }
                                            SpellCheckField { dialog: spellDialog }
                                        }
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

    // Routed to Main.exportRecord — fired by a sub-list row's menu exporting
    // XML, and by the note's own selfMenu below.
    signal exportRequested(string table, string id)
    // Duplicate on selfMenu opens the copy — routed through Main so it gets a
    // breadcrumb and a history entry like any other note.
    signal noteActivated(int noteRow, string noteId)
    // Routed to Main's shared Move To… dialog, same as ProjectDetailPage /
    // ItemsPage / ItemDetailPage.
    signal moveToRequested(string itemId)
    // Attendee row menus use the attendee's person_id to open the corresponding
    // People detail page, matching ProjectDetailPage's Team member menu.
    signal goToPersonRequested(string personId)

    // Shared record/plugin menu for the Attendees and Action Items lists.
    // Move To… is only ever offered for Action Items (see ai._menu()'s
    // allowMoveTo arg) — attRow._menu() never sets it, so this handler simply
    // never fires for an attendee row. Duplicate is likewise Action Items only:
    // the base menu excludes meeting_attendees, whose (note, person) unique key
    // makes a copy a guaranteed clash.
    RecordRowMenu {
        id: rowMenu
        onExportRecord: (table, id) => page.exportRequested(table, id)
        // Copied action items keep this note, so the copy just turns up in the
        // list — copyTrackerItem() renumbers it and refreshes the models.
        onDuplicateRecord: (table, id) => DesktopAppController.copyTrackerItem(id)
        onMoveToRecord: (id) => page.moveToRequested(id)
        onGoToPersonRequested: (personId) => page.goToPersonRequested(personId)
    }

    // The note's own record/plugin menu — opened by the title row's kebab and
    // by right-clicking the page background. Lists plugins whose dataexport is
    // "project_notes" (e.g. base_plugin's "Send Meeting Notes") below the
    // built-in actions. Parity with ProjectDetailPage's selfMenu; New/Filter/
    // Move To aren't offered — there's no page-level equivalent action for a
    // single open note the way there is for a project.
    RecordContextMenu {
        id: selfMenu
        recordType: qsTr("Note")
        model: DesktopAppController.projectNotesModel
        recordId: page.noteId
        canOpen: false
        canNew: false
        canMoveTo: false
        canFilter: false
        onDeleteRequested: page.deleteRequested()
        onDuplicateRequested: {
            // Copy what's on screen, not what was last written.
            page._saveNow()
            // ProjectNotesModel::copyRecord keeps the project + title and the
            // attendee list, resets the date to now and leaves the body blank.
            var r = DesktopAppController.copyProjectNote(page.noteId)
            if (r >= 0) page.noteActivated(r, DesktopAppController.projectNoteIdAtRow(r))
        }
        onExportRequested: page.exportRequested(page.exportTable, page.exportId)
        onRefreshRequested: page._reload()
    }

    // Shared full-field spell-check dialog (opened by fields / the toolbar).
    SpellCheckDialog { id: spellDialog }

    // ── People picker (for adding an attendee) ────────────────────────────────
    // Only this project's team members may be added as attendees (matches the
    // Widgets app). The roster is reloaded on open so changes made while the
    // note is open are reflected.
    PeoplePickerDialog {
        id: peoplePicker
        headingText: qsTr("Add Attendee")
        reload: () => DesktopAppController.teamMemberList(page.projectId)
        onPicked: (person) => {
            var r = DesktopAppController.addAttendee(page.noteId)
            if (r >= 0) {
                DesktopAppController.saveAttendee(r, person.id)
                DesktopAppController.refreshMeetingAttendees()
            }
        }
    }

    // ── Inline reusable buttons ───────────────────────────────────────────────
    component SmallAddButton: Rectangle {
        property string text: ""
        signal clicked()
        implicitHeight: 24
        implicitWidth: sRow.implicitWidth + 14
        radius: Theme.radiusSm
        color: sHover.hovered ? Theme.accentStrong : Theme.accent
        RowLayout {
            id: sRow
            anchors.centerIn: parent
            spacing: 3
            MaterialIcon { name: "add"; size: 12; color: "#ffffff" }
            Text { text: parent.parent.text; color: "#ffffff"; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold }
        }
        HoverHandler { id: sHover }
        TapHandler { onTapped: parent.clicked() }
    }

    component DeleteButton: Item {
        signal clicked()
        implicitWidth: 24; implicitHeight: 24
        Rectangle {
            anchors.centerIn: parent
            width: 22; height: 22; radius: Theme.radiusSm
            color: dHover.hovered ? Theme.redSoft : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: "close"; size: 13; color: Theme.red }
        }
        HoverHandler { id: dHover }
        TapHandler { onTapped: parent.clicked() }
    }
}
