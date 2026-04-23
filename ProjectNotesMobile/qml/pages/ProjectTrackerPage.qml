// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// ProjectTrackerPage — tracker items filtered to the current project.
// Mirrors the Tracker Items tab in the desktop project details view.
// Uses AppController.trackerItemsModel (filtered by setProjectFilter).
// Detail editing is handled by TrackerItemDetailPage.

Page {
    id: root
    title: qsTr("Tracker Items")

    property string projectId:    ""
    property string projectTitle: ""
    property StackView stackView: StackView.view

    StackView.onActivated: AppController.refreshTrackerItems()

    Component.onDestruction: {
        root.forceActiveFocus()
        Qt.inputMethod.hide()
    }

    function statusColor(status) {
        switch (status) {
            case "New":      return "#cc0000"
            case "Assigned": return "#e07000"
            case "Resolved": return Theme.accentGreenDark
            default:         return palette.placeholderText
        }
    }

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search items…")
                onTextChanged: AppController.setQuickSearch(AppController.trackerItemsModel, text)
                inputMethodHints: Qt.ImhNoPredictiveText
            }
            ToolButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addTrackerItem(root.projectId)
                    if (newRow < 0) return
                    var newId = AppController.trackerItemIdAtRow(0)
                    var d = AppController.getTrackerItemDetailData(0)
                    root.StackView.view.push(Qt.resolvedUrl("TrackerItemDetailPage.qml"), {
                        itemRow:              0,
                        itemId:               newId,
                        initialItemNumber:    (d.item_number        || "").toString(),
                        initialProjectNumber: (d.project_number     || "").toString(),
                        initialProjectName:   (d.project_name       || "").toString(),
                        initialType:          (d.item_type         || "").toString(),
                        initialName:          (d.item_name         || "").toString(),
                        initialDescription:   (d.description       || "").toString(),
                        initialIdentifiedBy:  (d.identified_by     || "").toString(),
                        initialAssignedTo:    (d.assigned_to       || "").toString(),
                        initialPriority:      (d.priority          || "").toString(),
                        initialStatus:        (d.status            || "").toString(),
                        initialDateIdentified:(d.date_identified    || "").toString(),
                        initialDateDue:       (d.date_due          || "").toString(),
                        initialInternal:      (d.internal_item     || "0") !== "0"
                    })
                }
            }
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

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Label {
                        visible: (model.priority || "") !== ""
                        text: model.priority || ""
                        font.pixelSize: 12
                        color: model.priority_foreground || palette.placeholderText
                    }

                    Label {
                        visible: (model.date_due || "") !== ""
                        text: {
                            var sep = (model.priority || "") !== "" ? "  ·  " : ""
                            return sep + "Due: " + (model.date_due || "")
                        }
                        font.pixelSize: 12
                        color: model.date_due_foreground || palette.placeholderText
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
            }

            onClicked: {
                var itemId = model.id || ""

                if (!itemId) return
                AppController.openTrackerItem(itemId)
                var d = AppController.getTrackerItemDetailData(0)
                root.stackView.push(Qt.resolvedUrl("TrackerItemDetailPage.qml"), {
                    itemRow:              0,
                    itemId:               itemId,
                    initialItemNumber:    (d.item_number        || "").toString(),
                    initialProjectNumber: (d.project_number     || "").toString(),
                    initialProjectName:   (d.project_name       || "").toString(),
                    initialType:          (d.item_type          || "").toString(),
                    initialName:          (d.item_name          || "").toString(),
                    initialDescription:   (d.description        || "").toString(),
                    initialIdentifiedBy:  (d.identified_by      || "").toString(),
                    initialAssignedTo:    (d.assigned_to        || "").toString(),
                    initialPriority:      (d.priority           || "").toString(),
                    initialStatus:        (d.status             || "").toString(),
                    initialDateIdentified:(d.date_identified     || "").toString(),
                    initialDateDue:       (d.date_due           || "").toString(),
                    initialInternal:      (d.internal_item      || "0") !== "0"
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
            text: "\u26A0\uFE0F"
            font.pixelSize: 52
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No Tracker Items")
            font.pixelSize: 17
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tap + to log an issue, risk, or action item.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: palette.placeholderText
        }
    }
}
