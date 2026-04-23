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
    title: qsTr("Item Detail")

    property int    itemRow:              0    // always row 0 of the filtered detail model
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
    property bool   initialInternal:      false
    property bool   _skipSave:            false

    function _releaseInputFocus() {
        root.forceActiveFocus()
        Qt.inputMethod.hide()
    }

    function _saveNow() {
        root._releaseInputFocus()
        dateIdentifiedField.commitPending()
        dateDueField.commitPending()

        AppController.saveTrackerItemDetail(
            root.itemRow,
            itemNumberField.text,
            typeCombo.currentIndex >= 0 ? typeCombo.model[typeCombo.currentIndex] : "",
            nameField.text,
            descEdit.text,
            identifiedByCombo.currentIndex >= 0
                ? AppController.teamMemberPersonIdAtRow(identifiedByCombo.currentIndex) : "",
            assignedToCombo.currentIndex >= 0
                ? AppController.teamMemberPersonIdAtRow(assignedToCombo.currentIndex) : "",
            priorityCombo.currentIndex >= 0 ? priorityCombo.model[priorityCombo.currentIndex] : "",
            statusCombo.currentIndex >= 0   ? statusCombo.model[statusCombo.currentIndex]     : "",
            dateIdentifiedField.text,
            dateDueField.text,
            internalSwitch.checked
        )
    }

    StackView.onActivated: root._skipSave = false

    StackView.onDeactivating: {
        if (!root._skipSave) {
            root._saveNow()
            root._skipSave = true
        }
    }

    StackView.onRemoved: {
        if (!root._skipSave) {
            root._saveNow()
            root._skipSave = true
        }
    }

    Component.onDestruction: {
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
                    root._saveNow()
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
                        initialInternal:      (d.internal_item    || "0") !== "0"
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    root._skipSave = true
                    AppController.deleteTrackerItemDetail(root.itemRow)
                    root.StackView.view.pop()
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
                text: qsTr("Comments")
                display: AbstractButton.TextUnderIcon
                onClicked: {
                    root._saveNow()
                    root.StackView.view.push(Qt.resolvedUrl("TrackerItemCommentsPage.qml"), {
                        itemId: root.itemId
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
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialProjectNumber !== "" ? root.initialProjectNumber : qsTr("—")
                    color: root.initialProjectNumber !== "" ? palette.text : palette.placeholderText
                    font.pixelSize: 16
                }
            }

            SectionHeader { text: qsTr("Project Name") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialProjectName !== "" ? root.initialProjectName : qsTr("—")
                    color: root.initialProjectName !== "" ? palette.text : palette.placeholderText
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }
            }

            SectionHeader { text: qsTr("Item Number") }
            FieldRow {
                TextField {
                    id: itemNumberField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialItemNumber
                    horizontalAlignment: TextInput.AlignLeft
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Type") }
            FieldRow {
                ComboBox {
                    id: typeCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.trackerItemTypeOptions()
                    Component.onCompleted: {
                        var idx = model.indexOf(root.initialType)
                        currentIndex = idx >= 0 ? idx : 0
                    }
                }
            }

            SectionHeader { text: qsTr("Name") }
            FieldRow {
                TextField {
                    id: nameField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialName
                    horizontalAlignment: TextInput.AlignLeft
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
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
                }
                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1; color: palette.placeholderText; opacity: 0.3
                }
            }

            SectionHeader { text: qsTr("Identified By") }
            FieldRow {
                ComboBox {
                    id: identifiedByCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.projectTeamMembersModel
                    textRole: "name"
                    Component.onCompleted: {
                        var row = AppController.teamMemberRowForPersonId(root.initialIdentifiedBy)
                        currentIndex = row >= 0 ? row : -1
                    }
                }
            }

            SectionHeader { text: qsTr("Assigned To") }
            FieldRow {
                ComboBox {
                    id: assignedToCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.projectTeamMembersModel
                    textRole: "name"
                    Component.onCompleted: {
                        var row = AppController.teamMemberRowForPersonId(root.initialAssignedTo)
                        currentIndex = row >= 0 ? row : -1
                    }
                    onActivated: function(idx) {
                        if (idx >= 0) {
                            var curStatus = statusCombo.currentIndex >= 0
                                ? statusCombo.model[statusCombo.currentIndex] : ""
                            if (curStatus === "New") {
                                var assignedIdx = statusCombo.model.indexOf("Assigned")
                                if (assignedIdx >= 0) statusCombo.currentIndex = assignedIdx
                            }
                        }
                    }
                }
            }

            SectionHeader { text: qsTr("Priority") }
            FieldRow {
                ComboBox {
                    id: priorityCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.trackerItemPriorityOptions()
                    Component.onCompleted: {
                        var idx = model.indexOf(root.initialPriority)
                        currentIndex = idx >= 0 ? idx : 0
                    }
                }
            }

            SectionHeader { text: qsTr("Status") }
            FieldRow {
                ComboBox {
                    id: statusCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.trackerItemStatusOptions()
                    Component.onCompleted: {
                        var idx = model.indexOf(root.initialStatus)
                        currentIndex = idx >= 0 ? idx : 0
                    }
                }
            }

            SectionHeader { text: qsTr("Date Identified") }
            DateFieldRow { id: dateIdentifiedField; text: root.initialDateIdentified }

            SectionHeader { text: qsTr("Date Due") }
            DateFieldRow { id: dateDueField; text: root.initialDateDue }

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
