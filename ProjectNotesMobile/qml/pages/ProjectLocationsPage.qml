// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// ProjectLocationsPage — files & folders for the current project.
// Mirrors the Locations tab in the desktop project details view.
// Columns from projectlocationsmodel.cpp:
//   0=id, 1=project_id, 2=location_type, 3=location_description, 4=full_path

Page {
    id: root
    title: qsTr("Files && Folders")

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
                    var newRow = AppController.addProjectLocation(root.projectId)
                    if (newRow < 0) return
                    var d = AppController.getProjectLocationData(newRow)
                    root.StackView.view.push(Qt.resolvedUrl("ProjectLocationDetailPage.qml"), {
                        locationRow:         newRow,
                        initialType:         (d.location_type        || "").toString(),
                        initialDescription:  (d.location_description || "").toString(),
                        initialPath:         (d.full_path            || "").toString()
                    })
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.projectLocationsModel
        clip: true

        delegate: ItemDelegate {
            width: listView.width

            contentItem: ColumnLayout {
                spacing: 3

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: model.location_description || model.full_path || ""
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: model.location_type || ""
                        font.pixelSize: 12
                        color: palette.mid
                    }
                }

                Label {
                    visible: (model.full_path || "") !== "" && (model.location_description || "") !== ""
                    text: model.full_path || ""
                    font.pixelSize: 12
                    color: palette.mid
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("ProjectLocationDetailPage.qml"), {
                    locationRow:         index,
                    initialType:         model.location_type        || "",
                    initialDescription:  model.location_description || "",
                    initialPath:         model.full_path            || ""
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No files or folders.")
        color: palette.mid
    }
}
