// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// ProjectTrackerPage — tracker items filtered to the current project.
// Mirrors the Tracker Items tab in the desktop project details view.
// Uses AppController.trackerItemsModel (filtered by setProjectFilter).

Page {
    id: root
    title: qsTr("Tracker Items")

    property string projectId:    ""
    property string projectTitle: ""

    function statusColor(status) {
        switch (status) {
            case "New":      return "#cc0000"
            case "Assigned": return "#e07000"
            case "Resolved": return "#228822"
            default:         return palette.mid
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.trackerItemsModel
        clip: true

        delegate: ItemDelegate {
            width: listView.width

            contentItem: ColumnLayout {
                spacing: 3

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: {
                            var num  = model.item_number || ""
                            var type = model.item_type   || ""
                            return (num && type) ? num + "  ·  " + type : (num || type)
                        }
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: model.status || ""
                        font.pixelSize: 12
                        color: root.statusColor(model.status || "")
                    }
                }

                Label {
                    visible: (model.item_name || "") !== ""
                    text: model.item_name || ""
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Label {
                    text: {
                        var parts = []
                        if (model.priority || "") parts.push(model.priority)
                        if (model.date_due || "") parts.push("Due: " + model.date_due)
                        return parts.join("  ·  ")
                    }
                    font.pixelSize: 12
                    color: palette.mid
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No tracker items.")
        color: palette.mid
    }
}
