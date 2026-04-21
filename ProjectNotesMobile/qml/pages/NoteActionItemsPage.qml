// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// NoteActionItemsPage — action items linked to a single meeting note.
// Uses AppController.notesActionItemsModel (filtered by setNoteFilter).
// Tapping an item calls openTrackerItem() then pushes TrackerItemDetailPage.
// Columns: same layout as item_tracker (0=id, 1=item_number, 2=item_type,
//   3=item_name, 8=priority, 9=status, 10=date_due, 15=internal_item)

Page {
    id: root
    title: qsTr("Action Items")

    property string noteId:    ""
    property string projectId: ""

    StackView.onActivated: AppController.setNoteFilter(root.noteId, root.projectId)

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
                placeholderText: qsTr("Search action items…")
                onTextChanged: AppController.setQuickSearch(AppController.notesActionItemsModel, text)
                inputMethodHints: Qt.ImhNoPredictiveText
            }
            ToolButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addNoteActionItem(root.noteId, root.projectId)
                    if (newRow < 0) return
                    var newId = AppController.trackerItemIdAtRow(0)
                    var d = AppController.getTrackerItemDetailData(0)
                    root.StackView.view.push(Qt.resolvedUrl("TrackerItemDetailPage.qml"), {
                        itemRow:              0,
                        itemId:               newId,
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
        model: AppController.notesActionItemsModel
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
                    color: palette.placeholderText
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            onClicked: {
                var itemId = model.id || ""
                if (!itemId) return
                AppController.openTrackerItem(itemId)
                root.StackView.view.push(Qt.resolvedUrl("TrackerItemDetailPage.qml"), {
                    itemRow:              0,
                    itemId:               itemId,
                    initialType:          model.item_type         || "",
                    initialName:          model.item_name         || "",
                    initialDescription:   model.description       || "",
                    initialIdentifiedBy:  model.identified_by     || "",
                    initialAssignedTo:    model.assigned_to       || "",
                    initialPriority:      model.priority          || "",
                    initialStatus:        model.status            || "",
                    initialDateIdentified:model.date_identified    || "",
                    initialDateDue:       model.date_due          || "",
                    initialInternal:      (model.internal_item    || "0") !== "0"
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No action items.")
        color: palette.placeholderText
    }
}
