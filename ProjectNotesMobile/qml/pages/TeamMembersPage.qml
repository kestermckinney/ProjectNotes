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
    title: {
        var base = qsTr("Team Members")
        if (root.projectId === "") return base
        return base + " — " + AppController.projectNumberForId(root.projectId)
                    + " " + AppController.projectNameForId(root.projectId).substring(0, 20)
    }

    property string projectId:    ""
    property string projectTitle: ""

    StackView.onActivated: AppController.setProjectFilter(root.projectId)

    FilterSheet { id: filterSheet }
    SortSheet   { id: sortSheet }

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
                rightPadding: clearBtn.visible ? clearBtn.width + 4 : 0

                Label {
                    id: clearBtn
                    visible: searchField.text.length > 0
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 6 }
                    text: "✕"
                    font.pixelSize: 18
                    color: palette.text
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -6
                        onClicked: searchField.clear()
                    }
                }
            }
            ToolButton {
                icon.name: "line.3.horizontal.decrease.circle"
                onClicked: filterSheet.openFor("team", qsTr("Team"))
                Rectangle {
                    visible: { AppController.filterRev; return AppController.hasActiveColumnFilters(AppController.projectTeamMembersModel) }
                    width: 8; height: 8; radius: 4; color: palette.highlight
                    anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 6 }
                }
            }
            ToolButton {
                icon.name: "arrow.up.arrow.down"
                onClicked: sortSheet.openFor("team", qsTr("Team"))
                Rectangle {
                    visible: { AppController.sortRev; return (AppController.activeSort(AppController.projectTeamMembersModel).field || "") !== "" }
                    width: 8; height: 8; radius: 4; color: palette.highlight
                    anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 6 }
                }
            }
            ToolButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addTeamMember(root.projectId)
                    if (newRow < 0) return
                    var d = AppController.getTeamMemberData(newRow)
                    root.StackView.view.push(Qt.resolvedUrl("TeamMemberDetailPage.qml"), {
                        memberRow:                  newRow,
                        memberId:                   (d.id                     || "").toString(),
                        isNewRecord:                true,
                        projectTitle:               root.projectTitle,
                        initialPeopleId:            (d.people_id              || "").toString(),
                        initialRole:                (d.role                   || "").toString(),
                        initialReceiveStatusReport: (d.receive_status_report  || "0") !== "0",
                        initialEmail:               (d.email                  || "").toString()
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
            id: delegateRoot
            required property int index
            required property var model
            width: listView.width

            contentItem: RowLayout {
                spacing: 8

                ColumnLayout {
                    spacing: 3
                    Layout.fillWidth: true

                    Label {
                        text: delegateRoot.model.name || ""
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: {
                            var parts = []
                            if (delegateRoot.model.client_name || "") parts.push(delegateRoot.model.client_name)
                            if (delegateRoot.model.role        || "") parts.push(delegateRoot.model.role)
                            return parts.join("  ·  ")
                        }
                        font.pixelSize: 12
                        color: Theme.mutedText
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: (delegateRoot.model.receive_status_report || "0") !== "0"
                        text: qsTr("Receives Status Report")
                        font.pixelSize: 11
                        color: palette.link
                    }
                }

                RowLayout {
                    spacing: 0
                    Layout.alignment: Qt.AlignVCenter

                    ToolButton {
                        visible: (delegateRoot.model.cell_phone || "").length > 0
                        icon.name: "iphone"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("tel:" + (delegateRoot.model.cell_phone || "").replace(/[^\d+]/g, ""))
                    }

                    ToolButton {
                        visible: (delegateRoot.model.office_phone || "").length > 0
                        icon.name: "phone.fill"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("tel:" + (delegateRoot.model.office_phone || "").replace(/[^\d+]/g, ""))
                    }

                    ToolButton {
                        visible: (delegateRoot.model.email || "").length > 0
                        icon.name: "envelope"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("mailto:" + (delegateRoot.model.email || "")
                            + "?subject=" + encodeURIComponent(root.projectTitle + " -"))
                    }
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("TeamMemberDetailPage.qml"), {
                    memberRow:                  delegateRoot.index,
                    memberId:                   delegateRoot.model.id                   || "",
                    projectTitle:               root.projectTitle,
                    initialPeopleId:            delegateRoot.model.people_id            || "",
                    initialRole:                delegateRoot.model.role                 || "",
                    initialReceiveStatusReport: (delegateRoot.model.receive_status_report || "0") !== "0",
                    initialEmail:               delegateRoot.model.email                || ""
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No team members.")
        color: Theme.mutedText
    }
}
