// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Tracker item detail (risk / issue / action item): full field editing plus a
// comments/updates list. The item is opened via openTrackerItem(itemId), which
// filters the detail model so the record is always at row 0.
Item {
    id: page
    property string itemId: ""
    property string projectId: ""
    property bool   _changed: false
    property var    _people: []
    readonly property string exportTable: "item_tracker"
    readonly property string exportId: itemId

    function _peopleNames() { return _people.map(function(p){ return p.name }) }
    function _idForName(n){ for (var i=0;i<_people.length;i++) if (_people[i].name===n) return _people[i].id; return "" }
    function _nameForId(id){ for (var i=0;i<_people.length;i++) if (_people[i].id===id) return _people[i].name; return "" }

    property string _identifiedBy: ""
    property string _assignedTo: ""

    Component.onCompleted: {
        _people = DesktopAppController.peopleList()
        if (page.itemId !== "")
            DesktopAppController.openTrackerItem(page.itemId)
        _reload()
    }

    function _reload() {
        var d = DesktopAppController.getTrackerItemDetailData(0)
        page.itemId  = (d.id || page.itemId).toString()
        page.projectId = (d.project_id || "").toString()
        numberField.text = (d.item_number || "").toString()
        typeCombo.value  = (d.item_type || "").toString()
        nameField.text   = (d.item_name || "").toString()
        descArea.text   = (d.description || "").toString()
        priorityCombo.value = (d.priority || "").toString()
        statusCombo.value   = (d.status || "").toString()
        idDate.text  = (d.date_identified || "").toString()
        dueDate.text = (d.date_due || "").toString()
        internalCheck.checked = (d.internal_item || "0") !== "0"
        page._identifiedBy = (d.identified_by || "").toString()
        page._assignedTo   = (d.assigned_to || "").toString()
        identifiedCombo.value = _nameForId(page._identifiedBy)
        assignedCombo.value   = _nameForId(page._assignedTo)
        page._changed = false
    }

    function _saveNow() {
        if (!page._changed) return true
        var ok = DesktopAppController.saveTrackerItemDetail(0, page.itemId,
            numberField.text, typeCombo.value, nameField.text, descArea.text,
            page._identifiedBy, page._assignedTo, priorityCombo.value, statusCombo.value,
            idDate.text, dueDate.text, internalCheck.checked)
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

            GridLayout {
                Layout.fillWidth: true; columns: 2; columnSpacing: 14; rowSpacing: 12
                FormField { label: qsTr("Item Number"); id: numberField; onEdited: page._changed = true }
                ComboField {
                    label: qsTr("Type"); id: typeCombo
                    options: DesktopAppController.itemTypeOptions()
                    onActivated: page._changed = true
                }
                FormField {
                    label: qsTr("Item Name"); id: nameField
                    Layout.columnSpan: 2
                    onEdited: page._changed = true
                }
            }

            Text { text: qsTr("Description"); color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(90, descArea.contentHeight + 20)
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: descArea.activeFocus ? Theme.accent : Theme.border
                TextArea {
                    id: descArea
                    anchors.fill: parent
                    anchors.margins: 8
                    color: Theme.text
                    wrapMode: TextEdit.WordWrap
                    selectByMouse: true
                    background: null
                    font.pixelSize: 13
                    onTextChanged: page._changed = true
                }
            }

            GridLayout {
                Layout.fillWidth: true; columns: 2; columnSpacing: 14; rowSpacing: 12
                ComboField {
                    label: qsTr("Identified By"); id: identifiedCombo
                    options: page._peopleNames()
                    onActivated: (v) => { page._identifiedBy = page._idForName(v); page._changed = true }
                }
                ComboField {
                    label: qsTr("Assigned To"); id: assignedCombo
                    options: page._peopleNames()
                    onActivated: (v) => { page._assignedTo = page._idForName(v); page._changed = true }
                }
                ComboField {
                    label: qsTr("Priority"); id: priorityCombo
                    options: DesktopAppController.itemPriorityOptions()
                    onActivated: page._changed = true
                }
                ComboField {
                    label: qsTr("Status"); id: statusCombo
                    options: DesktopAppController.itemStatusOptions()
                    onActivated: page._changed = true
                }
                DateField { label: qsTr("Date Identified"); id: idDate; onEdited: page._changed = true }
                DateField { label: qsTr("Date Due"); id: dueDate; onEdited: page._changed = true }
            }

            CheckBox {
                id: internalCheck
                onToggled: page._changed = true
                indicator: Rectangle {
                    implicitWidth: 18; implicitHeight: 18; radius: 4
                    x: internalCheck.leftPadding; y: parent.height/2 - height/2
                    color: internalCheck.checked ? Theme.accent : Theme.surface
                    border.color: internalCheck.checked ? Theme.accent : Theme.border
                    MaterialIcon { anchors.centerIn: parent; visible: internalCheck.checked; name: "check"; size: 14; color: "#ffffff" }
                }
                contentItem: Text {
                    text: qsTr("Internal item"); color: Theme.text; font.pixelSize: 13
                    leftPadding: internalCheck.indicator.width + 8; verticalAlignment: Text.AlignVCenter
                }
            }

            // Comments / updates
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 6
                MaterialIcon { name: "forum"; size: 18; color: Theme.text2 }
                Text {
                    text: qsTr("Comments"); color: Theme.text
                    font.pixelSize: 15; font.weight: Font.Bold; Layout.fillWidth: true
                }
                Rectangle {
                    implicitHeight: 28; implicitWidth: cRow.implicitWidth + 18
                    radius: Theme.radiusSm
                    color: cHover.hovered ? Theme.accentStrong : Theme.accent
                    RowLayout {
                        id: cRow; anchors.centerIn: parent; spacing: 5
                        MaterialIcon { name: "add"; size: 15; color: "#ffffff" }
                        Text { text: qsTr("Add Comment"); color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold }
                    }
                    HoverHandler { id: cHover }
                    TapHandler {
                        onTapped: {
                            page._saveNow()
                            DesktopAppController.addComment(page.itemId)
                            DesktopAppController.refreshTrackerComments()
                        }
                    }
                }
            }

            Repeater {
                model: DesktopAppController.trackerItemCommentsModel
                delegate: Card {
                    id: cCard
                    required property int index
                    required property var model
                    Layout.fillWidth: true
                    implicitHeight: cCol.implicitHeight + 20

                    ColumnLayout {
                        id: cCol
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: (cCard.model.lastupdated_date || "").toString()
                                color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold
                            }
                            Text {
                                text: {
                                    var by = (cCard.model.updated_by || "").toString()
                                    return by !== "" ? "· " + DesktopAppController.peopleNameForId(by) : ""
                                }
                                color: Theme.text3; font.pixelSize: 11; Layout.fillWidth: true
                            }
                            Rectangle {
                                width: 22; height: 22; radius: 6
                                color: dHover.hovered ? Theme.redSoft : "transparent"
                                MaterialIcon { anchors.centerIn: parent; name: "close"; size: 14; color: Theme.red }
                                HoverHandler { id: dHover }
                                TapHandler {
                                    onTapped: {
                                        DesktopAppController.deleteComment(cCard.index)
                                        DesktopAppController.refreshTrackerComments()
                                    }
                                }
                            }
                        }
                        // Editable comment text
                        TextArea {
                            Layout.fillWidth: true
                            text: (cCard.model.update_note || "").toString()
                            color: Theme.text
                            wrapMode: TextEdit.WordWrap
                            selectByMouse: true
                            background: null
                            font.pixelSize: 13
                            onEditingFinished: {
                                DesktopAppController.saveComment(cCard.index,
                                    (cCard.model.lastupdated_date || "").toString(),
                                    text, (cCard.model.updated_by || "").toString())
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }
        }
    }
}
