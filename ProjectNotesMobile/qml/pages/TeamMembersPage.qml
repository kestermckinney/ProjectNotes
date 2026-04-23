// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// TeamMembersPage — mirrors the Team tab in the desktop project details view.
// Columns from projectteammembersmodel.cpp:
//   0=id, 1=project_id, 2=people_id, 3=name, 4=receive_status_report,
//   5=role, 6=email, 7=project_number, 8=project_name, 9=client_name,
//   10=office_phone, 11=cell_phone

Page {
    id: root
    title: qsTr("Team Members")

    property string projectId:    ""
    property string projectTitle: ""

    StackView.onActivated: AppController.setProjectFilter(root.projectId)

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search team…")
                onTextChanged: AppController.setQuickSearch(AppController.projectTeamMembersModel, text)
                inputMethodHints: Qt.ImhNoPredictiveText
            }
            ToolButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addTeamMember(root.projectId)
                    if (newRow < 0) return
                    var d = AppController.getTeamMemberData(newRow)
                    root.StackView.view.push(Qt.resolvedUrl("TeamMemberDetailPage.qml"), {
                        memberRow:                  newRow,
                        initialPeopleId:            (d.people_id              || "").toString(),
                        initialRole:                (d.role                   || "").toString(),
                        initialReceiveStatusReport: (d.receive_status_report  || "0") !== "0"
                    })
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.projectTeamMembersModel
        clip: true

        delegate: ItemDelegate {
            width: listView.width

            contentItem: RowLayout {
                spacing: 8

                ColumnLayout {
                    spacing: 3
                    Layout.fillWidth: true

                    Label {
                        text: model.name || ""
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: {
                            var parts = []
                            if (model.role  || "") parts.push(model.role)
                            if (model.email || "") parts.push(model.email)
                            return parts.join("  ·  ")
                        }
                        font.pixelSize: 12
                        color: palette.placeholderText
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: (model.receive_status_report || "0") !== "0"
                        text: qsTr("Receives Status Report")
                        font.pixelSize: 11
                        color: palette.link
                    }
                }

                RowLayout {
                    spacing: 0
                    Layout.alignment: Qt.AlignVCenter

                    ToolButton {
                        visible: (model.cell_phone || "").length > 0
                        icon.name: "iphone"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("tel:" + (model.cell_phone || "").replace(/[^\d+]/g, ""))
                    }

                    ToolButton {
                        visible: (model.office_phone || "").length > 0
                        icon.name: "phone.fill"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("tel:" + (model.office_phone || "").replace(/[^\d+]/g, ""))
                    }

                    ToolButton {
                        visible: (model.email || "").length > 0
                        icon.name: "envelope"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("mailto:" + (model.email || ""))
                    }
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("TeamMemberDetailPage.qml"), {
                    memberRow:                  index,
                    initialPeopleId:            model.people_id              || "",
                    initialRole:                model.role                   || "",
                    initialReceiveStatusReport: (model.receive_status_report || "0") !== "0"
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No team members.")
        color: palette.placeholderText
    }
}
