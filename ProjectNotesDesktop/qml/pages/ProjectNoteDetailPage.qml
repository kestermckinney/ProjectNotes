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

    // Consumed by the TopBar's Delete action (Main.deleteCurrent()).
    readonly property bool canDelete: true

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
                    spellCheck: true
                    spellDialog: spellDialog
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
                dialog: spellDialog
                spell: noteSpell.spell
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

                    // Inline spell-check: red squiggle + right-click suggestions +
                    // "Check Spelling…" (opens the shared full-field dialog).
                    SpellCheckField { id: noteSpell; dialog: spellDialog }
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
                                id: attRow
                                required property int index
                                required property var model
                                Layout.fillWidth: true
                                spacing: 8
                                function _menu(sx, sy) {
                                    rowMenu.openFor(DesktopAppController.meetingAttendeesModel,
                                        (attRow.model.id || "").toString(), qsTr("Attendee"),
                                        (attRow.model.name || "").toString(), sx, sy)
                                }
                                TapHandler {
                                    acceptedButtons: Qt.RightButton
                                    onTapped: (ev) => attRow._menu(ev.scenePosition.x, ev.scenePosition.y)
                                }
                                Rectangle {
                                    implicitWidth: 26; implicitHeight: 26; radius: 13
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
                                KebabButton {
                                    implicitWidth: 24; implicitHeight: 24
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
                            delegate: ColumnLayout {
                                id: ai
                                required property int index
                                required property var model
                                property bool expanded: false
                                property string _assignedId: ""
                                property string _identifiedId: ""
                                Layout.fillWidth: true
                                spacing: 6

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
                                    aiDesc.text       = (ai.model.description || "").toString()
                                    ai.expanded = true
                                }
                                // Persist all fields (in-place setData → summary updates live,
                                // no refresh, so the editor stays open). Uses the editor values,
                                // which _edit() populated from the model.
                                function _save() {
                                    DesktopAppController.saveNoteActionItem(ai.index,
                                        aiNameInline.text, aiType.value, aiPriority.value, aiStatus.value,
                                        ai._assignedId, ai._identifiedId, aiDateId.text, aiDateDue.text,
                                        aiDesc.text)
                                }
                                // Persist an inline name edit without disturbing the other fields:
                                // read everything except the name straight from the model, so this
                                // is safe even when the editor was never expanded.
                                function _saveName() {
                                    DesktopAppController.saveNoteActionItem(ai.index,
                                        aiNameInline.text,
                                        (ai.model.item_type || "").toString(),
                                        (ai.model.priority || "").toString(),
                                        (ai.model.status || "").toString(),
                                        (ai.model.assigned_to || "").toString(),
                                        (ai.model.identified_by || "").toString(),
                                        (ai.model.date_identified || "").toString(),
                                        (ai.model.date_due || "").toString(),
                                        (ai.model.description || "").toString())
                                }
                                function _menu(sx, sy) {
                                    rowMenu.openFor(DesktopAppController.notesActionItemsModel,
                                        (ai.model.id || "").toString(), qsTr("Action Item"),
                                        (ai.model.item_name || "").toString(), sx, sy)
                                }
                                TapHandler {
                                    acceptedButtons: Qt.RightButton
                                    onTapped: (ev) => ai._menu(ev.scenePosition.x, ev.scenePosition.y)
                                }

                                // Summary row (click to expand/collapse the editor)
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    MaterialIcon {
                                        name: {
                                            var s = (ai.model.status || "").toString().toLowerCase()
                                            if (s.indexOf("resolved") >= 0 || s.indexOf("closed") >= 0) return "check_circle"
                                            return "radio_button_unchecked"
                                        }
                                        size: 16
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
                                            font.pixelSize: 13
                                            background: null
                                            padding: 0
                                            selectByMouse: true
                                            placeholderText: qsTr("(unnamed item)")
                                            placeholderTextColor: Theme.text3
                                            Component.onCompleted: text = (ai.model.item_name || "").toString()
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
                                            color: Theme.text3; font.pixelSize: 11; elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                    // Edit / collapse toggle
                                    Rectangle {
                                        implicitWidth: 26; implicitHeight: 26; radius: 6
                                        color: eHover.hovered ? Theme.surface2 : "transparent"
                                        Layout.alignment: Qt.AlignVCenter
                                        MaterialIcon {
                                            anchors.centerIn: parent
                                            name: ai.expanded ? "expand_less" : "edit"
                                            size: 15; color: Theme.text2
                                        }
                                        HoverHandler { id: eHover }
                                        TapHandler { onTapped: { if (ai.expanded) { ai._save(); ai.expanded = false } else ai._edit() } }
                                    }
                                    KebabButton {
                                        implicitWidth: 26; implicitHeight: 26
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
                                    Layout.leftMargin: 24
                                    visible: ai.expanded
                                    spacing: 8

                                    GridLayout {
                                        Layout.fillWidth: true; columns: 2; columnSpacing: 10; rowSpacing: 8
                                        ComboField {
                                            id: aiType; label: qsTr("Type")
                                            options: DesktopAppController.itemTypeOptions()
                                            onActivated: ai._save()
                                        }
                                        ComboField {
                                            id: aiPriority; label: qsTr("Priority")
                                            options: DesktopAppController.itemPriorityOptions()
                                            onActivated: ai._save()
                                        }
                                        ComboField {
                                            id: aiStatus; label: qsTr("Status")
                                            options: DesktopAppController.itemStatusOptions()
                                            onActivated: ai._save()
                                        }
                                        ComboField {
                                            id: aiAssigned; label: qsTr("Assigned To")
                                            options: page._peopleNames()
                                            includeNone: true
                                            searchable: true
                                            onActivated: (v) => { ai._assignedId = page._idForName(v); ai._save() }
                                        }
                                        ComboField {
                                            id: aiIdentified; label: qsTr("Identified By")
                                            options: page._peopleNames()
                                            includeNone: true
                                            searchable: true
                                            onActivated: (v) => { ai._identifiedId = page._idForName(v); ai._save() }
                                        }
                                        DateField { id: aiDateId; label: qsTr("Date Identified"); onEdited: ai._save() }
                                        DateField { id: aiDateDue; label: qsTr("Date Due"); onEdited: ai._save() }
                                    }
                                    Text { text: qsTr("Description"); color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold }
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: Math.max(64, aiDesc.contentHeight + 18)
                                        radius: Theme.radiusSm
                                        color: Theme.surface
                                        border.color: aiDesc.activeFocus ? Theme.accent : Theme.border
                                        TextArea {
                                            id: aiDesc
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            color: Theme.text
                                            wrapMode: TextEdit.WordWrap
                                            selectByMouse: true
                                            background: null
                                            font.pixelSize: 13
                                            onEditingFinished: ai._save()
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

    // Routed to Main.exportRecord when a sub-list row's menu exports XML.
    signal exportRequested(string table, string id)

    // Shared record/plugin menu for the Attendees and Action Items lists.
    RecordRowMenu {
        id: rowMenu
        onExportRecord: (table, id) => page.exportRequested(table, id)
    }

    // Shared full-field spell-check dialog (opened by fields / the toolbar).
    SpellCheckDialog { id: spellDialog }

    // ── People picker (for adding an attendee) ────────────────────────────────
    Dialog {
        id: peoplePicker
        anchors.centerIn: parent
        width: 360
        height: 420
        modal: true
        padding: 0
        scale: Theme.uiScale   // match the zoomed workspace (centered origin)
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        // Type-to-search text (lower-cased match target). Empty = show everyone.
        property string _filter: ""

        // Reset and focus the search box each time the picker opens.
        onOpened: {
            _filter = ""; attendeeSearch.text = ""; attendeeSearch.forceActiveFocus()
            peopleList.model = DesktopAppController.teamMemberList(page.projectId)
        }

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
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

            // Search field — filters the list below as you type.
            Rectangle {
                Layout.fillWidth: true
                Layout.margins: 12
                implicitHeight: 34
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: attendeeSearch.activeFocus ? Theme.accent : Theme.border
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10; anchors.rightMargin: 10
                    spacing: 6
                    MaterialIcon { name: "search"; size: 16; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                    TextField {
                        id: attendeeSearch
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search people…")
                        placeholderTextColor: Theme.text3
                        color: Theme.text
                        font.pixelSize: 13
                        background: null
                        verticalAlignment: Text.AlignVCenter
                        selectByMouse: true
                        onTextChanged: peoplePicker._filter = text
                    }
                }
            }

            ListView {
                id: peopleList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                // Only this project's team members may be added as attendees
                // (matches the Widgets app). Refreshed on open so roster changes
                // made while the note is open are reflected.
                model: DesktopAppController.teamMemberList(page.projectId)
                delegate: ItemDelegate {
                    id: attendeeDelegate
                    required property int index
                    required property var modelData
                    // Collapse rows that don't contain the search text.
                    readonly property bool _match: peoplePicker._filter === ""
                        || String(modelData.name).toLowerCase().indexOf(peoplePicker._filter.toLowerCase()) >= 0
                    visible: _match
                    width: peopleList.width
                    height: _match ? 40 : 0
                    contentItem: Text {
                        text: modelData.name
                        color: Theme.text
                        font.pixelSize: 13
                        leftPadding: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { color: attendeeDelegate.hovered ? Theme.surface2 : "transparent" }
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
