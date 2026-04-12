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
                onTextChanged: AppController.setClientsFilter(text)
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
            contentItem: ColumnLayout {
                spacing: 2

                Label {
                    // client_name = column 1
                    text: model.client_name || ""
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
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

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No clients found.")
        horizontalAlignment: Text.AlignHCenter
        color: palette.mid
    }
}
