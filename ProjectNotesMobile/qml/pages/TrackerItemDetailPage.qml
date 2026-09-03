// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// TrackerItemDetailPage — view/edit a single tracker item.
// Accessible from ProjectTrackerPage, AllItemsPage, and NoteActionItemsPage.
// Uses AppController.trackerItemDetailModel (filtered to one item by openTrackerItem()).
// Columns from trackeritemsmodel.cpp:
//   0=id, 1=item_number, 2=item_type, 3=item_name, 4=identified_by,
//   5=date_identified, 6=description, 7=assigned_to, 8=priority,
//   9=status, 10=date_due, 11=last_update, 12=date_resolved,
//   13=note_id, 14=project_id, 15=internal_item,
//   19=project_name, 20=project_number

Page {
    id: root
    title: {
        var base = qsTr("Item Detail")
        if (root.initialProjectNumber === "" && root.initialProjectName === "") return base
        return base + " — " + root.initialProjectNumber
                    + " " + root.initialProjectName.substring(0, 20)
    }

    property int    itemRow:              -1
    property string itemId:               ""
    property string initialItemNumber:    ""
    property string initialProjectNumber: ""
    property string initialProjectName:   ""
    property string initialType:          ""
    property string initialName:          ""
    property string initialDescription:   ""
    property string initialIdentifiedBy:  ""
    property string initialAssignedTo:    ""
    property string initialPriority:      ""
    property string initialStatus:        ""
    property string initialDateIdentified: ""
    property string initialDateDue:       ""
    property string initialLastUpdate:    ""
    property string initialDateResolved:  ""
    property bool   initialInternal:      false
    property string initialNoteId:        ""
    property bool   _skipSave:            false
    property bool   _hasChanges:          false
    property bool   isNewRecord:          false
    // Last committed status, so a status change can tell it moved into or out of
    // "Resolved". Synced in Component.onCompleted and _reloadData().
    property string _prevStatus:          ""
    // Set true once the page has finished building — the field bindings fire
    // onTextChanged during load, and this guards the live date preview from
    // stamping today's date then.
    property bool   _ready:               false
    // property string _validatedItemNumber: root.initialItemNumber

    // Stable {id,name} snapshot backing identifiedByCombo/assignedToCombo —
    // deliberately NOT a live model proxy, which a Sort elsewhere can reorder
    // or reset out from under a ComboBox bound directly to it (see
    // AppController::teamMemberList doc comment).
    property var _people: []
    function _peopleNames() { return root._people.map(function(p){ return p.name }) }
    function _personIndexForId(id) {
        for (var i = 0; i < root._people.length; i++)
            if (root._people[i].id === id) return i
        return -1
    }

    // Stable {id,name} snapshot backing meetingCombo — deliberately NOT the
    // live actionitemsdetailsmeetingsmodelproxy, which sort() elsewhere can
    // reorder out from under a ComboBox bound directly to it (see the
    // AppController teamMemberList/clientList doc comments).
    property var _meetings: []
    function _meetingNames() { return root._meetings.map(function(m){ return m.name }) }
    function _meetingIndexForId(id) {
        for (var i = 0; i < root._meetings.length; i++)
            if (root._meetings[i].id === id) return i
        return -1
    }

    // Identified By / Assigned To offer this project's team only, plus whoever
    // either field already names so an existing value keeps displaying — the
    // same rule as desktop's ItemDetailPage. The project id is read off the item
    // row; none of the pages that push this one carry one of their own.
    function _loadPeople(identifiedBy, assignedTo) {
        var d = AppController.getTrackerItemDetailData(root.itemRow)
        root._people = AppController.teamMemberList((d.project_id || "").toString(),
                                                    [identifiedBy, assignedTo])
    }

    // Id of the entry a combo is sitting on, or "" for "(none)"/no selection.
    function _idAt(list, i) { return (i >= 0 && i < list.length) ? list[i].id : "" }

    function _isBlankNew() { return isNewRecord && nameField.text.trim() === "" }
    function _discardNew()  { AppController.deleteTrackerItemDetail(root.itemRow) }

    // function _releaseInputFocus() {
    //     root.forceActiveFocus()
    //     Qt.inputMethod.hide()
    // }

    function _today() { return Qt.formatDate(new Date(), "MM/dd/yyyy") }

    // Preview TrackerItemsModel::setData's date bookkeeping before the save
    // round-trip: any field edit restamps Last Updated to today. Last Updated /
    // Date Resolved are read-only labels bound to these properties, so writing
    // them updates the screen live.
    function _touch() {
        root._hasChanges = true
        if (root._ready)
            root.initialLastUpdate = root._today()
    }

    // Mirror the resolved-date half of that bookkeeping: moving Status to
    // "Resolved" stamps Date Resolved, moving away clears it.
    function _applyStatusDates(newStatus) {
        if (!root._ready) return
        if (newStatus === "Resolved" && root._prevStatus !== "Resolved")
            root.initialDateResolved = root._today()
        else if (newStatus !== "Resolved" && root._prevStatus === "Resolved")
            root.initialDateResolved = ""
        root._prevStatus = newStatus
    }

    function _saveNow() {
        if (!root._hasChanges) return true
        // root._releaseInputFocus()
        dateIdentifiedField.commitPending()
        dateDueField.commitPending()

        var result = AppController.saveTrackerItemDetail(
            root.itemRow,
            root.itemId,
            itemNumber.text,
            typeCombo.selection,
            nameField.text,
            descEdit.text,
            root._idAt(root._people,   identifiedByCombo.optionIndex),
            root._idAt(root._people,   assignedToCombo.optionIndex),
            priorityCombo.selection,
            statusCombo.selection,
            dateIdentifiedField.text,
            dateDueField.text,
            root._idAt(root._meetings, meetingCombo.optionIndex),
            internalSwitch.checked
        )
        if (result) root._hasChanges = false
        return result
    }

    function _reloadData() {
        var d = AppController.getTrackerItemDetailData(root.itemRow)
        // root._validatedItemNumber = (d.item_number || "").toString()
        itemNumber.text =  (d.item_number || "").toString() // root._validatedItemNumber
        typeCombo.selectText((d.item_type || "").toString(), 0)
        nameField.text   = (d.item_name    || "").toString()
        descEdit.text    = (d.description  || "").toString()
        root._loadPeople((d.identified_by || "").toString(), (d.assigned_to || "").toString())
        identifiedByCombo.selectOption(root._personIndexForId((d.identified_by || "").toString()))
        assignedToCombo.selectOption(root._personIndexForId((d.assigned_to   || "").toString()))
        priorityCombo.selectText((d.priority || "").toString())
        statusCombo.selectText((d.status || "").toString())
        root._prevStatus = (d.status || "").toString()
        dateIdentifiedField.text = (d.date_identified || "").toString()
        dateDueField.text        = (d.date_due        || "").toString()
        root.initialLastUpdate   = (d.last_update     || "").toString()
        root.initialDateResolved = (d.date_resolved   || "").toString()
        internalSwitch.checked   = (d.internal_item   || "0") !== "0"
        root._meetings = AppController.meetingList()
        meetingCombo.selectOption(root._meetingIndexForId((d.note_id || "").toString()))
    }

    // Runs after every child combo's own Component.onCompleted (their selectText
    // has already landed), so _prevStatus starts from the shown status and the
    // live date preview only arms once the page is fully built.
    Component.onCompleted: {
        root._prevStatus = root.initialStatus
        root._ready = true
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

    Component.onDestruction: {
        root.forceActiveFocus()
        Qt.inputMethod.hide()
        if (!root._skipSave)
            root._saveNow()
    }

    // ── Header: copy + delete ─────────────────────────────────────────────────
    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            Item { Layout.fillWidth: true }

            ToolButton {
                icon.name: "doc.on.doc"
                onClicked: {
                    if (!root._saveNow()) return
                    root._skipSave = true
                    var newRow = AppController.copyTrackerItemDetail(root.itemRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    // copyTrackerItemDetail already called openTrackerItem; row 0 is the copy.
                    var newId = AppController.trackerItemIdAtRow(0)
                    var d = AppController.getTrackerItemDetailData(0)
                    root.StackView.view.replace(Qt.resolvedUrl("TrackerItemDetailPage.qml"), {
                        itemRow:              0,
                        itemId:               newId,
                        initialItemNumber:    (d.item_number      || "").toString(),
                        initialProjectNumber: root.initialProjectNumber,
                        initialProjectName:   root.initialProjectName,
                        initialType:          (d.item_type        || "").toString(),
                        initialName:          (d.item_name        || "").toString(),
                        initialDescription:   (d.description      || "").toString(),
                        initialIdentifiedBy:  (d.identified_by    || "").toString(),
                        initialAssignedTo:    (d.assigned_to      || "").toString(),
                        initialPriority:      (d.priority         || "").toString(),
                        initialStatus:        (d.status           || "").toString(),
                        initialDateIdentified:(d.date_identified   || "").toString(),
                        initialDateDue:       (d.date_due         || "").toString(),
                        initialLastUpdate:    (d.last_update      || "").toString(),
                        initialDateResolved:  (d.date_resolved    || "").toString(),
                        initialNoteId:        (d.note_id          || "").toString(),
                        initialInternal:      (d.internal_item    || "0") !== "0"
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    if (AppController.deleteTrackerItemDetail(root.itemRow)) {
                        root._skipSave = true
                        root.StackView.view.pop()
                    }
                }
            }
        }
    }

    // ── Footer: Comments icon ─────────────────────────────────────────────────
    footer: ToolBar {
        RowLayout {
            anchors.centerIn: parent
            spacing: 32

            ToolButton {
                icon.name: "text.bubble"
                display: AbstractButton.TextUnderIcon
                onClicked: {
                    if (!root._saveNow()) return
                    root.StackView.view.push(Qt.resolvedUrl("TrackerItemCommentsPage.qml"), {
                        itemId:     root.itemId,
                        itemNumber: itemNumber.text,
                        itemName:   nameField.text
                    })
                }
            }
        }
    }

    // ── Body ──────────────────────────────────────────────────────────────────
    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            SectionHeader { text: qsTr("Project Number") }
            FieldRow {
                FormLabel {
                    text: root.initialProjectNumber !== "" ? root.initialProjectNumber : qsTr("—")
                }
            }

            SectionHeader { text: qsTr("Project Name") }
            FieldRow {
                FormLabel {
                    text: root.initialProjectName !== "" ? root.initialProjectName : qsTr("—")
                    elide: Text.ElideRight
                }
            }

            SectionHeader { text: qsTr("Meeting") }
            FieldRow {
                FormCombo {
                    id: meetingCombo
                    options: root._meetingNames()
                    Component.onCompleted: {
                        root._meetings = AppController.meetingList()
                        selectOption(root._meetingIndexForId(root.initialNoteId))
                    }
                    onActivated: root._touch()
                }
            }

            SectionHeader { text: qsTr("Item Number") }
            FieldRow {
                FormField {
                    id: itemNumber
                    text: root.initialItemNumber
                    inputMethodHints: Qt.ImhNoPredictiveText
                    onTextChanged: root._touch()
                }
            }

            SectionHeader { text: qsTr("Type") }
            FieldRow {
                FormCombo {
                    id: typeCombo
                    options: AppController.trackerItemTypeOptions()
                    Component.onCompleted: selectText(root.initialType, 0)
                    onActivated: root._touch()
                }
            }

            SectionHeader { text: qsTr("Name") }
            FieldRow {

                FormField {
                    id: nameField
                    text: root.initialName
                    inputMethodHints: Qt.ImhNoPredictiveText
                    onTextChanged: root._touch()
                }
            }

            SectionHeader { text: qsTr("Description") }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(120, descEdit.contentHeight + 24)
                color: palette.base

                TextEdit {
                    id: descEdit
                    anchors { fill: parent; margins: 8 }
                    text: root.initialDescription
                    wrapMode: TextEdit.Wrap
                    color: palette.text
                    selectByMouse: true
                    onTextChanged: root._touch()
                }
                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1; color: Theme.mutedText; opacity: 0.3
                }
            }

            SectionHeader { text: qsTr("Identified By") }
            FieldRow {
                FormCombo {
                    id: identifiedByCombo
                    options: root._peopleNames()
                    includeNone: true
                    Component.onCompleted: {
                        root._loadPeople(root.initialIdentifiedBy, root.initialAssignedTo)
                        selectOption(root._personIndexForId(root.initialIdentifiedBy))
                    }
                    onActivated: root._touch()
                }
            }

            SectionHeader { text: qsTr("Assigned To") }
            FieldRow {
                FormCombo {
                    id: assignedToCombo
                    options: root._peopleNames()
                    includeNone: true
                    Component.onCompleted: {
                        root._loadPeople(root.initialIdentifiedBy, root.initialAssignedTo)
                        selectOption(root._personIndexForId(root.initialAssignedTo))
                    }
                    onActivated: {
                        root._touch()
                        // Assigning a still-New item moves it on to Assigned;
                        // clearing the assignment leaves the status alone.
                        if (optionIndex >= 0 && statusCombo.selection === "New") {
                            statusCombo.selectText("Assigned")
                            root._prevStatus = "Assigned"
                        }
                    }
                }
            }

            SectionHeader { text: qsTr("Priority") }
            FieldRow {
                FormCombo {
                    id: priorityCombo
                    options: AppController.trackerItemPriorityOptions()
                    Component.onCompleted: selectText(root.initialPriority, 0)
                    onActivated: root._touch()
                }
            }

            SectionHeader { text: qsTr("Status") }
            FieldRow {
                FormCombo {
                    id: statusCombo
                    options: AppController.trackerItemStatusOptions()
                    Component.onCompleted: selectText(root.initialStatus, 0)
                    onActivated: { root._applyStatusDates(statusCombo.selection); root._touch() }
                }
            }

            SectionHeader { text: qsTr("Date Identified") }
            DateFieldRow { id: dateIdentifiedField; text: root.initialDateIdentified; onTextChanged: root._touch() }

            SectionHeader { text: qsTr("Date Due") }
            DateFieldRow { id: dateDueField; text: root.initialDateDue; onTextChanged: root._touch() }

            SectionHeader { text: qsTr("Last Updated") }
            FieldRow {
                FormLabel {
                    text: root.initialLastUpdate !== "" ? root.initialLastUpdate : qsTr("—")
                }
            }

            SectionHeader { text: qsTr("Date Resolved") }
            FieldRow {
                FormLabel {
                    text: root.initialDateResolved !== "" ? root.initialDateResolved : qsTr("—")
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
                    onToggled: root._touch()
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
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

    Dialog {
        id: duplicateNumberDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Duplicate Item Number")
        modal: true
        standardButtons: Dialog.Ok
        Label {
            text: qsTr("This item number is already used by another item in this project. Please enter a unique number.")
            wrapMode: Text.Wrap
            width: 240
        }
    }
}
