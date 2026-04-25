// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// MeetingAttendeeDetailPage — select a person for a meeting attendee record.
// Columns: 0=id, 1=note_id, 2=person_id (editable), 3=name, 4=project_name,
//          5=email, 6=client_name, 7=project_id, 8=project_number

Page {
    id: root
    title: qsTr("Attendee")

    property int    attendeeRow:   -1
    property string initialPerson: ""
    property bool   _skipSave:     false

    function _saveNow() {
        var personId = personCombo.currentIndex >= 0
            ? AppController.teamMemberPersonIdAtRow(personCombo.currentIndex) : ""
        return AppController.saveAttendee(root.attendeeRow, personId)
    }

    function _reloadData() {
        var d = AppController.getAttendeeData(root.attendeeRow)
        var row = AppController.teamMemberRowForPersonId((d.person_id || "").toString())
        personCombo.currentIndex = row >= 0 ? row : -1
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            Item { Layout.fillWidth: true }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    root._skipSave = true
                    AppController.deleteAttendee(root.attendeeRow)
                    root.StackView.view.pop()
                }
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            SectionHeader { text: qsTr("Person") }
            FieldRow {
                ComboBox {
                    id: personCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.projectTeamMembersModel
                    textRole: "name"
                    Component.onCompleted: {
                        var row = AppController.teamMemberRowForPersonId(root.initialPerson)
                        currentIndex = row >= 0 ? row : -1
                    }
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
