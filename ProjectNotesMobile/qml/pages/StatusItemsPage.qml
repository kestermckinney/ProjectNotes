// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// StatusItemsPage — mirrors the Status tab in the desktop project details view.
// Columns from statusreportitemsmodel.cpp:
//   0=id, 1=project_id, 2=task_category, 3=task_description

Page {
    id: root
    title: qsTr("Status Items")

    property string projectId:    ""
    property string projectTitle: ""

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            Item { Layout.fillWidth: true }
            ToolButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addStatusItem(root.projectId)
                    if (newRow < 0) return
                    var d = AppController.getStatusItemData(newRow)
                    root.StackView.view.push(Qt.resolvedUrl("StatusItemDetailPage.qml"), {
                        itemRow:            newRow,
                        initialCategory:    (d.task_category    || "").toString(),
                        initialDescription: (d.task_description || "").toString()
                    })
                }
            }
        }
    }

    function categoryColor(cat) {
        switch (cat) {
            case "In Progress": return "#e07000"
            case "Completed":   return "#228822"
            case "Starting":    return "#0055cc"
            default:            return palette.mid
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.statusReportItemsModel
        clip: true

        delegate: ItemDelegate {
            width: listView.width

            contentItem: ColumnLayout {
                spacing: 3

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: model.task_category || ""
                        font.bold: true
                        font.pixelSize: 13
                        color: root.categoryColor(model.task_category || "")
                        Layout.fillWidth: true
                    }
                }

                Label {
                    visible: (model.task_description || "") !== ""
                    text: model.task_description || ""
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("StatusItemDetailPage.qml"), {
                    itemRow:             index,
                    initialCategory:     model.task_category    || "",
                    initialDescription:  model.task_description || ""
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No status items.")
        color: palette.mid
    }
}
