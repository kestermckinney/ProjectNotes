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
    property string attendeeId:    ""
    property string projectId:     ""
    property string initialPerson: ""
    property bool   _skipSave:     false
    property bool   _hasChanges:   false
    property bool   isNewRecord:   false

    // Stable {id,name} snapshot backing personCombo — deliberately NOT the
    // live AppController.projectTeamMembersModel proxy, which the Team tab's
    // Sort feature can reorder/reset out from under a ComboBox bound directly
    // to it (see AppController::teamMemberList doc comment).
    property var _people: []
    function _peopleNames() { return root._people.map(function(p){ return p.name }) }
    function _personIndexForId(id) {
        for (var i = 0; i < root._people.length; i++)
            if (root._people[i].id === id) return i
        return -1
    }

    function _isBlankNew() { return isNewRecord && personCombo.currentIndex < 0 }
    function _discardNew()  {
        var row = AppController.rowForId(AppController.meetingAttendeesModel, root.attendeeId)
        if (row < 0) return
        AppController.deleteAttendee(row)
    }

    // Re-resolve attendeeRow from the stable attendeeId before every write —
    // Sort/refresh elsewhere in the app can reorder or reset the shared
    // meetingAttendeesModel proxy while this page is open.
    function _saveNow() {
        if (!root._hasChanges) return true
        var row = AppController.rowForId(AppController.meetingAttendeesModel, root.attendeeId)
        if (row < 0) return false   // record no longer exists
        root.attendeeRow = row
        var personId = (personCombo.currentIndex >= 0 && personCombo.currentIndex < root._people.length)
            ? root._people[personCombo.currentIndex].id : ""
        var result = AppController.saveAttendee(root.attendeeRow, personId)
        if (result) root._hasChanges = false
        return result
    }

    function _reloadData() {
        var d = AppController.getAttendeeData(root.attendeeRow)
        var personId = (d.person_id || "").toString()
        root._people = AppController.teamMemberList(root.projectId, [personId])
        personCombo.currentIndex = root._personIndexForId(personId)
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
                    var row = AppController.rowForId(AppController.meetingAttendeesModel, root.attendeeId)
                    if (row >= 0 && AppController.deleteAttendee(row)) {
                        root._skipSave = true
                        root.StackView.view.pop()
                    }
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
                    model: root._peopleNames()
                    Component.onCompleted: {
                        root._people = AppController.teamMemberList(root.projectId, [root.initialPerson])
                        currentIndex = root._personIndexForId(root.initialPerson)
                    }
                    onActivated: root._hasChanges = true
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
}
