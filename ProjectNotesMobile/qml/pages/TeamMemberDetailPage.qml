// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Team Member")

    property int    memberRow:                  -1
    property string memberId:                   ""
    property string projectTitle:               ""
    property string initialContactId:           ""
    property string initialRole:                ""
    property bool   initialReceiveStatusReport: false
    property string initialEmail:               ""
    property bool   _skipSave:                  false
    property bool   _hasChanges:                false
    property bool   isNewRecord:                false

    // Stable {id,name} snapshot backing contactCombo — deliberately NOT the
    // live AppController.contactsModel proxy, which the Contacts tab's Sort
    // feature can reorder/reset out from under a ComboBox bound directly to
    // it (see AppController::teamMemberList doc comment).
    property var _contacts: []
    function _contactNames() { return root._contacts.map(function(p){ return p.name }) }
    function _contactIndexForId(id) {
        for (var i = 0; i < root._contacts.length; i++)
            if (root._contacts[i].id === id) return i
        return -1
    }

    function _isBlankNew() { return isNewRecord && contactCombo.optionIndex < 0 }
    function _discardNew()  {
        var row = AppController.rowForId(AppController.projectTeamMembersModel, root.memberId)
        if (row < 0) return
        AppController.deleteTeamMember(row)
    }

    // Re-resolve memberRow from the stable memberId before every write —
    // Sort/refresh elsewhere in the app can reorder or reset the shared
    // projectTeamMembersModel proxy while this page is open.
    function _saveNow() {
        if (!root._hasChanges) return true
        var row = AppController.rowForId(AppController.projectTeamMembersModel, root.memberId)
        if (row < 0) return false   // record no longer exists
        root.memberRow = row
        var pi = contactCombo.optionIndex
        var contactId = (pi >= 0 && pi < root._contacts.length) ? root._contacts[pi].id : ""
        var result = AppController.saveTeamMember(root.memberRow, contactId, roleField.text, statusSwitch.checked)
        if (result) root._hasChanges = false
        return result
    }

    function _reloadData() {
        var d = AppController.getTeamMemberData(root.memberRow)
        root._contacts = AppController.contactList()
        contactCombo.selectOption(root._contactIndexForId((d.people_id || "").toString()))
        roleField.text = (d.role || "").toString()
        statusSwitch.checked = (d.receive_status_report || "0") !== "0"
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

    // ── Toolbar: email + copy + delete ───────────────────────────────────────
    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            Item { Layout.fillWidth: true }

            ToolButton {
                icon.name: "envelope"
                visible: root.initialEmail !== ""
                onClicked: {
                    var subject = root.projectTitle + " -"
                    Qt.openUrlExternally("mailto:" + root.initialEmail + "?subject=" + encodeURIComponent(subject))
                }
            }

            ToolButton {
                icon.name: "doc.on.doc"
                onClicked: {
                    if (!root._saveNow()) return
                    root._skipSave = true
                    var newRow = AppController.copyTeamMember(root.memberRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getTeamMemberData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("TeamMemberDetailPage.qml"), {
                        memberRow:                  newRow,
                        memberId:                   (d.id                     || "").toString(),
                        projectTitle:               root.projectTitle,
                        initialContactId:           (d.people_id              || "").toString(),
                        initialRole:                (d.role                   || "").toString(),
                        initialReceiveStatusReport: (d.receive_status_report  || "0") !== "0",
                        initialEmail:               (d.email                  || "").toString()
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    var row = AppController.rowForId(AppController.projectTeamMembersModel, root.memberId)
                    if (row >= 0 && AppController.deleteTeamMember(row)) {
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

            SectionHeader { text: qsTr("Contact") }
            FieldRow {
                FormCombo {
                    id: contactCombo
                    options: root._contactNames()
                    Component.onCompleted: {
                        root._contacts = AppController.contactList()
                        selectOption(root._contactIndexForId(root.initialContactId))
                    }
                    onActivated: root._hasChanges = true
                }
            }

            SectionHeader { text: qsTr("Role") }
            FieldRow {
                FormField {
                    id: roleField
                    text: root.initialRole
                    inputMethodHints: Qt.ImhNoPredictiveText
                    onTextChanged: root._hasChanges = true
                }
            }

            SectionHeader { text: qsTr("Status Reports") }
            FieldRow {
                Switch {
                    id: statusSwitch
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 12 }
                    checked: root.initialReceiveStatusReport
                    text: qsTr("Receives Status Report")
                    onToggled: root._hasChanges = true
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
        font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
        font.weight: 600
        color: Theme.navyMid
        background: Rectangle { color: Theme.sectionBg }
    }
}
