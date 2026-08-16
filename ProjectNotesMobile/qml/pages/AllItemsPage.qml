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
            case "Resolved": return Theme.accentGreenDark
            default:         return Theme.mutedText
        }
    }

    // Quick Filter entries for a long-pressed item row: leads with "Assigned
    // to Me" — the Project Manager configured in Preferences, omitted when
    // none is set — then entries pre-filled from that row's own client/
    // assigned-to/identified-by (omitted when unset), then the overdue toggle
    // and the three priority shortcuts. "Assigned to Me" goes first so the one
    // entry that doesn't depend on the pressed row keeps a stable position in
    // the sheet. item_overdue is a computed "Yes"/"No" column (see
    // trackeritemsmodel.cpp). Mirrors the desktop Master Item List's set —
    // ProjectNotesDesktop/qml/pages/ItemsPage.qml.
    function _quickFiltersForRow(m) {
        var qf = []
        var pmId = AppController.projectManagerId()
        if (pmId !== "")
            qf.push({ label: qsTr("Assigned to Me"), field: "assigned_to", values: [pmId] })
        var clientId = (m.client_id || "").toString()
        if (clientId !== "")
            qf.push({ label: qsTr("This Client"), field: "client_id", values: [clientId] })
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
                onClicked: filterSheet.openFor("items", qsTr("Items"))
                Rectangle {
                    visible: { AppController.filterRev; return AppController.hasActiveColumnFilters(AppController.allItemsModel) }
                    width: 8; height: 8; radius: 4; color: palette.highlight
                    anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 6 }
                }
            }

            ToolButton {
                icon.name: "arrow.up.arrow.down"
                onClicked: sortSheet.openFor("items", qsTr("Items"))
                Rectangle {
                    visible: { AppController.sortRev; return (AppController.activeSort(AppController.allItemsModel).field || "") !== "" }
                    width: 8; height: 8; radius: 4; color: palette.highlight
                    anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 6 }
                }
            }
        }
    }

    // ── Item list ─────────────────────────────────────────────────────────────
    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.allItemsModel
        clip: true
        reuseItems: true

        delegate: ItemDelegate {
            id: delegateRoot
            required property int index
            required property var model
            width: listView.width

            // Long press → Quick Filter. Uses the delegate's own pressAndHold
            // rather than a TapHandler's longPressed: an ItemDelegate arms its
            // hold timer only when something is connected to this signal, and
            // only that timer firing suppresses the clicked() it would
            // otherwise emit on release — via a TapHandler the dialog opened
            // but letting go still counted as a tap and navigated to the row
            // underneath it.
            onPressAndHold: qfDialog.openWith(AppController.allItemsModel, root._quickFiltersForRow(delegateRoot.model))

            contentItem: ColumnLayout {
                spacing: 3

                // Row 1: project_number + project_name (bold)
                Label {
                    text: {
                        var num  = delegateRoot.model.project_number || ""
                        var name = delegateRoot.model.project_name   || ""
                        return num ? num + "  " + name : name
                    }
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                // Row 2: item_number + item_name
                Label {
                    text: {
                        var num  = delegateRoot.model.item_number || ""
                        var name = delegateRoot.model.item_name   || ""
                        return num ? num + "  " + name : name
                    }
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                // Row 3: status · priority · assigned_to · due date
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Label {
                        visible: (delegateRoot.model.status || "") !== ""
                        text: delegateRoot.model.status || ""
                        font.pixelSize: 12
                        color: root.statusColor(delegateRoot.model.status || "")
                    }

                    Label {
                        visible: (delegateRoot.model.priority || "") !== ""
                        text: {
                            var sep = (delegateRoot.model.status || "") !== "" ? "  ·  " : ""
                            return sep + (delegateRoot.model.priority || "")
                        }
                        font.pixelSize: 12
                        color: delegateRoot.model.priority_foreground || Theme.mutedText
                    }

                    Label {
                        visible: (delegateRoot.model.assigned_to || "") !== ""
                        text: {
                            var sep = (delegateRoot.model.status || delegateRoot.model.priority) ? "  ·  " : ""
                            return sep + AppController.peopleNameForId(delegateRoot.model.assigned_to || "")
                        }
                        font.pixelSize: 12
                        color: Theme.mutedText
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: (delegateRoot.model.date_due || "") !== ""
                        text: {
                            var sep = (delegateRoot.model.status || delegateRoot.model.priority || delegateRoot.model.assigned_to) ? "  ·  " : ""
                            return sep + "Due: " + (delegateRoot.model.date_due || "")
                        }
                        font.pixelSize: 12
                        color: delegateRoot.model.date_due_foreground || Theme.mutedText
                        elide: Text.ElideRight
                    }
                }
            }

            onClicked: {
                var itemId = delegateRoot.model.id || ""

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
            color: Theme.mutedText
        }
    }
}
