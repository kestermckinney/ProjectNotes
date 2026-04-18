// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Clients")

    property StackView stackView: null

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search clients…")
                onTextChanged: AppController.setQuickSearch(AppController.clientsModel, text)
                inputMethodHints: Qt.ImhNoPredictiveText
            }

            ToolButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addClient()
                    if (newRow < 0) return
                    var d = AppController.getClientData(newRow)
                    root.stackView.push(Qt.resolvedUrl("ClientDetailPage.qml"), {
                        clientRow:         newRow,
                        initialClientName: (d.client_name || "").toString()
                    })
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.clientsModel
        clip: true

        delegate: ItemDelegate {
            width: listView.width
            contentItem: RowLayout {
                spacing: 12

                Rectangle {
                    width: 38; height: 38
                    radius: 10
                    color: "#5856D6"
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        anchors.centerIn: parent
                        text: (model.client_name || "?").charAt(0).toUpperCase()
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                    }
                }

                Label {
                    text: model.client_name || ""
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }
            }
            onClicked: {
                root.stackView.push(Qt.resolvedUrl("ClientDetailPage.qml"), {
                    clientRow:         index,
                    initialClientName: model.client_name || ""
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
            text: "\uD83C\uDFE2"
            font.pixelSize: 52
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No Clients")
            font.pixelSize: 17
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tap + to add a client.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: palette.placeholderText
        }
    }
}
