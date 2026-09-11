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
    title: {
        var base = qsTr("Tracker Items")
        if (root.projectId === "") return base
        return base + " — " + AppController.projectNumberForId(root.projectId)
                    + " " + AppController.projectNameForId(root.projectId).substring(0, 20)
    }

    property string projectId:    ""
    property string projectTitle: ""
    property StackView stackView: StackView.view

    StackView.onActivated: AppController.refreshTrackerItems()

    Connections {
        target: AppController
        function onViewOptionsChanged() {
            if (root.StackView.status !== StackView.Active) return
            AppController.refreshTrackerItems()
        }
    }

    Component.onDestruction: {
        root.forceActiveFocus()
        Qt.inputMethod.hide()
    }

    function statusColor(status) {
        switch (status) {
            case "New":      return "#cc0000"
            case "Assigned": return "#e07000"
            case "Resolved": return Theme.accentGreenDark
            default:         return Theme.mutedText
        }
    }

    // Quick Filter entries for a long-pressed item row (project-scoped, so no
    // client shortcut — every row already shares this project's client), led
    // by "Assigned to Me" (the Project Manager configured in Preferences,
    // omitted when none is set) so this project's own items narrow down the
    // same way the All Items list does. item_overdue is a computed "Yes"/"No"
    // column (see trackeritemsmodel.cpp).
    function _quickFiltersForRow(m) {
        var qf = []
        var pmId = AppController.projectManagerId()
        if (pmId !== "")
            qf.push({ label: qsTr("Assigned to Me"), field: "assigned_to", values: [pmId] })
        var assignedId = (m.assigned_to || "").toString()
        // Identical field/values to "Assigned to Me" when the pressed row is
        // the PM's own — skip it rather than list the same shortcut twice.
        if (assignedId !== "" && assignedId !== pmId)
            qf.push({ label: qsTr("This Assigned To"), field: "assigned_to", values: [assignedId] })
        var identifiedId = (m.identified_by || "").toString()
        if (identifiedId !== "")
            qf.push({ label: qsTr("This Identified By"), field: "identified_by", values: [identifiedId] })
        qf.push({ label: qsTr("Overdue Items"),   field: "item_overdue", values: ["Yes"] })
        qf.push({ label: qsTr("High Priority"),   field: "priority", values: ["High"] })
        qf.push({ label: qsTr("Medium Priority"), field: "priority", values: ["Medium"] })
        qf.push({ label: qsTr("Low Priority"),    field: "priority", values: ["Low"] })
        return qf
    }

    FilterSheet     { id: filterSheet }
    SortSheet       { id: sortSheet }
    QuickFilterDialog { id: qfDialog }

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
                rightPadding: clearBtn.visible ? clearBtn.width + 4 : 0

                Label {
                    id: clearBtn
                    visible: searchField.text.length > 0
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 6 }
                    text: "✕"
                    font.pixelSize: 18
                    color: palette.text
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -6
                        onClicked: searchField.clear()
                    }
                }
            }
            ToolButton {
                icon.name: "line.3.horizontal.decrease.circle"
                icon.color: filterBadge.iconColor
                onClicked: filterSheet.openFor("trackeritems", qsTr("Tracker Items"))
                ActiveIndicator {
                    id: filterBadge
                    active: AppController.filterRev >= 0 && AppController.hasActiveColumnFilters(AppController.trackerItemsModel)
                }
            }
            ToolButton {
                icon.name: "arrow.up.arrow.down"
                icon.color: sortBadge.iconColor
                onClicked: sortSheet.openFor("trackeritems", qsTr("Tracker Items"))
                ActiveIndicator {
                    id: sortBadge
                    active: AppController.sortRev >= 0 && (AppController.activeSort(AppController.trackerItemsModel).field || "") !== ""
                }
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
                        isNewRecord:          true,
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
                        initialLastUpdate:    (d.last_update       || "").toString(),
                        initialDateResolved:  (d.date_resolved     || "").toString(),
                        initialNoteId:        (d.note_id           || "").toString(),
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

            // Long press → Quick Filter. The delegate's own pressAndHold, not a
            // TapHandler: only the button's hold timer (armed solely by
            // connecting to this signal) suppresses the clicked() that would
            // otherwise navigate to this row when you let go — see
            // AllItemsPage.qml.
            onPressAndHold: qfDialog.openWith(AppController.trackerItemsModel, root._quickFiltersForRow(model))

            contentItem: ColumnLayout {
                spacing: 3

                Label {
                    text: {
                        var num  = model.item_number || ""
                        var name = model.item_name   || ""
                        return num ? num + "  " + name : name
                    }
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Label {
                        visible: (model.status || "") !== ""
                        text: model.status || ""
                        font.pixelSize: 12
                        color: root.statusColor(model.status || "")
                    }

                    Label {
                        visible: (model.priority || "") !== ""
                        text: {
                            var sep = (model.status || "") !== "" ? "  ·  " : ""
                            return sep + (model.priority || "")
                        }
                        font.pixelSize: 12
                        color: model.priority_foreground || Theme.mutedText
                    }

                    Label {
                        visible: (model.assigned_to || "") !== ""
                        text: {
                            var sep = (model.status || model.priority) ? "  ·  " : ""
                            return sep + AppController.peopleNameForId(model.assigned_to || "")
                        }
                        font.pixelSize: 12
                        color: Theme.mutedText
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: (model.date_due || "") !== ""
                        text: {
                            var sep = (model.status || model.priority || model.assigned_to) ? "  ·  " : ""
                            return sep + "Due: " + (model.date_due || "")
                        }
                        font.pixelSize: 12
                        color: model.date_due_foreground || Theme.mutedText
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
                    initialLastUpdate:    (d.last_update        || "").toString(),
                    initialDateResolved:  (d.date_resolved      || "").toString(),
                    initialNoteId:        (d.note_id            || "").toString(),
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
            color: Theme.mutedText
        }
    }
}
