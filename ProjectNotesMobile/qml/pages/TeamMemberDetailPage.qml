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
    property string initialPeopleId:            ""
    property string initialRole:                ""
    property bool   initialReceiveStatusReport: false
    property bool   _skipSave:                  false

    function _saveNow() {
        var peopleId = (personCombo.currentIndex >= 0)
            ? AppController.peopleIdAtRow(personCombo.currentIndex) : ""
        AppController.saveTeamMember(root.memberRow, peopleId, roleField.text, statusSwitch.checked)
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

    // ── Toolbar: copy + delete ────────────────────────────────────────────────
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
                    var newRow = AppController.copyTeamMember(root.memberRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getTeamMemberData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("TeamMemberDetailPage.qml"), {
                        memberRow:                  newRow,
                        initialPeopleId:            (d.people_id              || "").toString(),
                        initialRole:                (d.role                   || "").toString(),
                        initialReceiveStatusReport: (d.receive_status_report  || "0") !== "0"
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    root._skipSave = true
                    AppController.deleteTeamMember(root.memberRow)
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
                    model: AppController.peopleModel
                    textRole: "name"
                    Component.onCompleted: {
                        var row = AppController.peopleRowForId(root.initialPeopleId)
                        currentIndex = (row >= 0) ? row : -1
                    }
                }
            }

            SectionHeader { text: qsTr("Role") }
            FieldRow {
                TextField {
                    id: roleField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialRole
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Status Reports") }
            FieldRow {
                Switch {
                    id: statusSwitch
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 12 }
                    checked: root.initialReceiveStatusReport
                    text: qsTr("Receives Status Report")
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
        font.weight: Font.Medium
        color: palette.mid
        background: Rectangle { color: palette.window }
    }

    component FieldRow: Rectangle {
        default property alias content: innerItem.data
        Layout.fillWidth: true
        Layout.preferredHeight: 44
        color: palette.base
        Item { id: innerItem; anchors.fill: parent }
        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
            height: 1; color: palette.mid; opacity: 0.3
        }
    }
}
