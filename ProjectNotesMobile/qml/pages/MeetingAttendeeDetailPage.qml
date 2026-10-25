// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// MeetingAttendeeDetailPage — select a contact for a meeting attendee record.
// Columns: 0=id, 1=note_id, 2=person_id (editable), 3=name, 4=project_name,
//          5=email, 6=client_name, 7=project_id, 8=project_number

Page {
    id: root
    title: qsTr("Attendee")

    property int    attendeeRow:    -1
    property string attendeeId:     ""
    property string projectId:      ""
    property string initialContact: ""
    property bool   _skipSave:      false
    property bool   _hasChanges:    false
    property bool   isNewRecord:    false

    // Stable {id,name} snapshot backing contactCombo — deliberately NOT the
    // live AppController.projectTeamMembersModel proxy, which the Team tab's
    // Sort feature can reorder/reset out from under a ComboBox bound directly
    // to it (see AppController::teamMemberList doc comment).
    property var _contacts: []
    function _contactNames() { return root._contacts.map(function(p){ return p.name }) }
    function _contactIndexForId(id) {
        for (var i = 0; i < root._contacts.length; i++)
            if (root._contacts[i].id === id) return i
        return -1
    }

    function _isBlankNew() { return isNewRecord && contactCombo.optionIndex < 0 }
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
        var pi = contactCombo.optionIndex
        var contactId = (pi >= 0 && pi < root._contacts.length) ? root._contacts[pi].id : ""
        var result = AppController.saveAttendee(root.attendeeRow, contactId)
        if (result) root._hasChanges = false
        return result
    }

    function _reloadData() {
        var d = AppController.getAttendeeData(root.attendeeRow)
        var contactId = (d.person_id || "").toString()
        root._contacts = AppController.teamMemberList(root.projectId, [contactId])
        contactCombo.selectOption(root._contactIndexForId(contactId))
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

    // ── Actions ──────────────────────────────────────────────────────────────
    // Glass pill floating at the top right, over the form as it scrolls.
    NavButton {
        id: navButton
        anchors { left: parent.left; leftMargin: 12; verticalCenter: topBar.verticalCenter }
    }

    GlassBar {
        id: topBar
        z: 2
        anchors { top: parent.top; right: parent.right; topMargin: Theme.topInset + 6; rightMargin: 12 }
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
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0
            // Room for the floating action pill above the first field.
            Item { Layout.preferredHeight: topBar.y + topBar.height + 6 }


            SectionHeader { text: qsTr("Contact") }
            FieldRow {
                FormCombo {
                    id: contactCombo
                    options: root._contactNames()
                    Component.onCompleted: {
                        root._contacts = AppController.teamMemberList(root.projectId, [root.initialContact])
                        selectOption(root._contactIndexForId(root.initialContact))
                    }
                    onActivated: root._hasChanges = true
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }
}
