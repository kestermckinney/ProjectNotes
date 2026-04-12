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

    // ── Status colour helper ──────────────────────────────────────────────────
    function statusColor(status) {
        switch (status) {
            case "New":      return "#cc0000"
            case "Assigned": return "#e07000"
            case "Resolved": return "#228822"
            default:         return palette.mid
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
                onTextChanged: AppController.setAllItemsFilter(text)
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

                // Row 3: project_number  |  priority  |  due date
                Label {
                    text: {
                        var parts = []
                        var proj  = model.project_number || ""
                        var pri   = model.priority       || ""
                        var due   = model.date_due       || ""
                        if (proj) parts.push(proj)
                        if (pri)  parts.push(pri)
                        if (due)  parts.push("Due: " + due)
                        return parts.join("  ·  ")
                    }
                    font.pixelSize: 12
                    color: palette.mid
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            onClicked: {
                // TODO: push item detail page
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    // ── Empty state ───────────────────────────────────────────────────────────
    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No items found.")
        horizontalAlignment: Text.AlignHCenter
        color: palette.mid
    }
}
