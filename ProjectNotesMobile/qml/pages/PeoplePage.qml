// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("People")

    property StackView stackView: null

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search people…")
                onTextChanged: AppController.setQuickSearch(AppController.peopleModel, text)
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
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addPerson()
                    if (newRow < 0) return
                    var d = AppController.getPersonData(newRow)
                    root.stackView.push(Qt.resolvedUrl("PersonDetailPage.qml"), {
                        personRow:          newRow,
                        initialName:        (d.name         || "").toString(),
                        initialEmail:       (d.email        || "").toString(),
                        initialOfficePhone: (d.office_phone || "").toString(),
                        initialCellPhone:   (d.cell_phone   || "").toString(),
                        initialClientId:    (d.client_id    || "").toString(),
                        initialRole:        (d.role         || "").toString()
                    })
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.peopleModel
        clip: true

        delegate: ItemDelegate {
            width: listView.width
            contentItem: RowLayout {
                spacing: 12

                Rectangle {
                    width: 38; height: 38
                    radius: 19
                    color: Theme.navyMid
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        anchors.centerIn: parent
                        text: (model.name || "?").charAt(0).toUpperCase()
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                    }
                }

                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true

                    Label {
                        text: model.name || ""
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: {
                            var email = model.email || ""
                            var role  = model.role  || ""
                            if (email && role) return email + "  ·  " + role
                            return email || role
                        }
                        font.pixelSize: 12
                        color: palette.placeholderText
                        elide: Text.ElideRight
                        Layout.fillWidth: true
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
                root.stackView.push(Qt.resolvedUrl("PersonDetailPage.qml"), {
                    personRow:         index,
                    initialName:       model.name          || "",
                    initialEmail:      model.email         || "",
                    initialOfficePhone:model.office_phone  || "",
                    initialCellPhone:  model.cell_phone    || "",
                    initialClientId:   model.client_id     || "",
                    initialRole:       model.role          || ""
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Column {
        anchors.centerIn: parent
        visible: listView.count === 0
        spacing: 10

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "\uD83D\uDC64"
            font.pixelSize: 52
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No People")
            font.pixelSize: 17
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tap + to add a contact.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: palette.placeholderText
        }
    }
}
