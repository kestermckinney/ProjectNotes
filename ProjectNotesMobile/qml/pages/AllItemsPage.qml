// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// AllItemsPage — master item list across all projects.
// Mirrors the desktop All Items view (allitemspage.cpp / allitemsview.cpp).
// Column indices from trackeritemsmodel.cpp:
//   0=id, 1=item_number, 2=item_type, 3=item_name, 4=identified_by,
//   5=date_identified, 6=description, 7=assigned_to, 8=priority,
//   9=status, 10=date_due, 11=last_update, 12=date_resolved,
//   13=note_id, 14=project_id, 15=internal_item, 16=comments,
//   17=project_status, 18=client_id, 19=project_name, 20=project_number

Page {
    id: root
    title: qsTr("Items")

    property StackView stackView: null

    StackView.onActivated: AppController.refreshAllItems()

    Component.onDestruction: {
        root.forceActiveFocus()
        Qt.inputMethod.hide()
    }

    // ── Status colour helper ──────────────────────────────────────────────────
    function statusColor(status) {
        switch (status) {
            case "New":      return "#cc0000"
            case "Assigned": return "#e07000"
            case "Resolved": return Theme.accentGreenDark
            default:         return palette.placeholderText
        }
    }

    function priorityLabel(priority) {
        if (!priority) return ""
        return "Priority: " + priority
    }

    // ── Search / filter toolbar ───────────────────────────────────────────────
    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search items…")
                onTextChanged: AppController.setQuickSearch(AppController.allItemsModel, text)
                inputMethodHints: Qt.ImhNoPredictiveText
            }
        }
    }

    // ── Item list ─────────────────────────────────────────────────────────────
    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.allItemsModel
        clip: true

        delegate: ItemDelegate {
            width: listView.width

            contentItem: ColumnLayout {
                spacing: 3

                // Row 1: item_number · item_type  |  status (coloured)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

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

                // Row 2: item_name
                Label {
                    visible: (model.item_name || "") !== ""
                    text: model.item_name || ""
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                // Row 3: project_number · priority (model color) · due date (model color)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Label {
                        visible: (model.project_number || "") !== ""
                        text: model.project_number || ""
                        font.pixelSize: 12
                        color: palette.placeholderText
                    }

                    Label {
                        visible: (model.project_number || "") !== "" && (model.priority || "") !== ""
                        text: "  ·  "
                        font.pixelSize: 12
                        color: palette.placeholderText
                    }

                    Label {
                        visible: (model.priority || "") !== ""
                        text: model.priority || ""
                        font.pixelSize: 12
                        color: model.priority_foreground || palette.placeholderText
                    }

                    Label {
                        visible: (model.date_due || "") !== ""
                        text: {
                            var sep = (model.project_number || model.priority) ? "  ·  " : ""
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

                console.log("All Items Page: ", itemId)

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

    // ── Empty state ───────────────────────────────────────────────────────────
    Column {
        anchors.centerIn: parent
        visible: listView.count === 0
        spacing: 10

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "\uD83D\uDCCB"
            font.pixelSize: 52
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No Items")
            font.pixelSize: 17
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tracker items from all projects appear here.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: palette.placeholderText
        }
    }
}
