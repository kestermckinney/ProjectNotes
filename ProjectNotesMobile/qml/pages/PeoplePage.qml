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
            contentItem: ColumnLayout {
                spacing: 2

                Label {
                    // name = column 1
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
                    color: palette.mid
                    elide: Text.ElideRight
                    Layout.fillWidth: true
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

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No people found.")
        horizontalAlignment: Text.AlignHCenter
        color: palette.mid
    }
}
