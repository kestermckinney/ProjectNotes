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
    property string projectTitle:               ""
    property string initialPeopleId:            ""
    property string initialRole:                ""
    property bool   initialReceiveStatusReport: false
    property string initialEmail:               ""
    property bool   _skipSave:                  false
    property bool   _hasChanges:                false
    property bool   isNewRecord:                false

    function _isBlankNew() { return isNewRecord && personCombo.currentIndex < 0 }
    function _discardNew()  { AppController.deleteTeamMember(root.memberRow) }

    function _saveNow() {
        if (!root._hasChanges) return true
        var peopleId = (personCombo.currentIndex >= 0)
            ? AppController.peopleIdAtRow(personCombo.currentIndex) : ""
        var result = AppController.saveTeamMember(root.memberRow, peopleId, roleField.text, statusSwitch.checked)
        if (result) root._hasChanges = false
        return result
    }

    function _reloadData() {
        var d = AppController.getTeamMemberData(root.memberRow)
        var row = AppController.peopleRowForId((d.people_id || "").toString())
        personCombo.currentIndex = row >= 0 ? row : -1
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
                        projectTitle:               root.projectTitle,
                        initialPeopleId:            (d.people_id              || "").toString(),
                        initialRole:                (d.role                   || "").toString(),
                        initialReceiveStatusReport: (d.receive_status_report  || "0") !== "0",
                        initialEmail:               (d.email                  || "").toString()
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    if (AppController.deleteTeamMember(root.memberRow)) {
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
                    model: AppController.peopleModel
                    textRole: "name"
                    Component.onCompleted: {
                        var row = AppController.peopleRowForId(root.initialPeopleId)
                        currentIndex = (row >= 0) ? row : -1
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
        font.pixelSize: 13
        font.weight: 600
        color: Theme.navyMid
        background: Rectangle { color: Theme.sectionBg }
    }
}
