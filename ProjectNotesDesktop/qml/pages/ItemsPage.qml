// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// All tracker items (risks / issues / action items) across projects.
Item {
    id: page
    signal itemActivated(string itemId)
    signal exportRequested(string table, string id)
    signal filterRequested()
    signal sortRequested(real sx, real sy)
    signal moveToRequested(string itemId)

    property string _ctxId: ""

    // Fires on every push of this (cached) page, including the first: loads
    // lazily on first visit, then re-queries only if the DB changed since.
    StackView.onActivated: DesktopAppController.ensureAllItemsLoaded()

    RecordContextMenu {
        id: ctxMenu
        recordType: qsTr("Item")
        model: DesktopAppController.allItemsModel
        recordId: page._ctxId
        canDelete: false            // master list is a read-only view
        canDuplicate: true          // Copy Item (Widgets parity)
        canMoveTo: true
        onOpenRequested:   page.itemActivated(page._ctxId)
        onNewRequested:    page.itemActivated(page._ctxId)
        onDuplicateRequested: {
            var newId = DesktopAppController.copyTrackerItem(page._ctxId)
            if (newId !== "") page.itemActivated(newId)
        }
        onMoveToRequested: page.moveToRequested(page._ctxId)
        onExportRequested: page.exportRequested("item_tracker", page._ctxId)
        onFilterRequested: page.filterRequested()
        onSortRequested: (sx, sy) => page.sortRequested(sx, sy)
        onRefreshRequested: DesktopAppController.refreshAllItems()
    }

    function _statusColor(s) {
        s = (s || "").toLowerCase()
        if (s.indexOf("resolved") >= 0 || s.indexOf("closed") >= 0) return Theme.green
        if (s.indexOf("assigned") >= 0) return Theme.amber
        if (s.indexOf("new") >= 0) return Theme.red
        return Theme.text3
    }

    // Resolve a people id to a display name (blank for empty).
    function _pname(id) {
        var s = (id || "").toString()
        return s !== "" ? DesktopAppController.peopleNameForId(s) : ""
    }

    // Quick Filter entries for a right-clicked item row: pre-filled from that
    // row's own client/assigned-to/identified-by (omitted when unset), plus
    // the always-available overdue toggle and the three priority shortcuts.
    // item_overdue is a computed "Yes"/"No" column (date_due in the past and
    // not Resolved/Cancelled — see trackeritemsmodel.cpp); priority values
    // are the exact case-sensitive strings DesktopAppController.itemPriorityOptions()
    // returns ("Low"/"Medium"/"High").
    function _quickFiltersForRow(m) {
        var qf = []
        var clientId = (m.client_id || "").toString()
        if (clientId !== "")
            qf.push({ icon: "apartment", label: qsTr("This Client"), field: "client_id", values: [clientId] })
        var assignedId = (m.assigned_to || "").toString()
        if (assignedId !== "")
            qf.push({ icon: "person_pin", label: qsTr("This Assigned To"), field: "assigned_to", values: [assignedId] })
        var identifiedId = (m.identified_by || "").toString()
        if (identifiedId !== "")
            qf.push({ icon: "badge", label: qsTr("This Identified By"), field: "identified_by", values: [identifiedId] })
        qf.push({ icon: "event_busy", label: qsTr("Overdue Items"), field: "item_overdue", values: ["Yes"] })
        qf.push({ icon: "priority_high", label: qsTr("High Priority"),   field: "priority", values: ["High"] })
        qf.push({ icon: "flag",          label: qsTr("Medium Priority"), field: "priority", values: ["Medium"] })
        qf.push({ icon: "low_priority",  label: qsTr("Low Priority"),    field: "priority", values: ["Low"] })
        return qf
    }

    // "LABEL value" metadata pair; hides itself when the value is empty.
    component MetaPair: RowLayout {
        id: mp
        property string label: ""
        property string value: ""
        visible: value !== ""
        spacing: 4
        Text { text: mp.label.toUpperCase(); color: Theme.text3; font.pixelSize: 9; font.weight: Font.Bold }
        Text { text: mp.value; color: Theme.text2; font.pixelSize: 11 }
    }

    // Virtualized list — only visible cards (plus cacheBuffer) are instantiated,
    // and reuseItems recycles delegates while scrolling.
    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: 16
        clip: true
        spacing: 8
        model: DesktopAppController.allItemsModel
        reuseItems: true
        cacheBuffer: 800
        boundsBehavior: Flickable.StopAtBounds
        footer: Item { width: 1; height: 8 }
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        delegate: Card {
                    id: card
                    required property int index
                    required property var model
                    readonly property string iid: model.id !== undefined ? model.id : ""
                    // Re-evaluates whenever `iid` changes — including when
                    // ListView's reuseItems rebinds this delegate to a
                    // different row while scrolling — so the preview always
                    // matches the card it's currently showing.
                    readonly property var _recentComments: DesktopAppController.recentCommentsForItem(card.iid, 2)
                    width: ListView.view ? ListView.view.width : 0
                    implicitHeight: itCol.implicitHeight + 24
                    color: hover.hovered ? Theme.raise : Theme.surface

                    ColumnLayout {
                        id: itCol
                        anchors.left: parent.left;  anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.leftMargin: 16; anchors.rightMargin: 16; anchors.topMargin: 12
                        spacing: 5

                        // Title row: type badge · number · name · status
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Rectangle {
                                implicitWidth: 30; implicitHeight: 30; radius: 8
                                color: Theme.accentSoft
                                Layout.alignment: Qt.AlignVCenter
                                MaterialIcon {
                                    anchors.centerIn: parent
                                    name: {
                                        var t = (card.model.item_type || "").toString().toLowerCase()
                                        if (t.indexOf("risk") >= 0) return "warning"
                                        if (t.indexOf("issue") >= 0) return "error"
                                        return "task_alt"
                                    }
                                    size: 16; color: Theme.accent
                                }
                            }
                            Text {
                                text: (card.model.item_number || "").toString()
                                color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Text {
                                text: (card.model.item_name || qsTr("(unnamed)")).toString()
                                color: Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold
                                elide: Text.ElideRight; Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Rectangle {
                                readonly property color c: page._statusColor((card.model.status || "").toString())
                                visible: (card.model.status || "").toString() !== ""
                                radius: 5
                                color: Qt.rgba(c.r, c.g, c.b, 0.14)
                                implicitHeight: 18; implicitWidth: stt.implicitWidth + 14
                                Layout.alignment: Qt.AlignVCenter
                                Text {
                                    id: stt; anchors.centerIn: parent
                                    text: (card.model.status || "").toString()
                                    color: parent.c; font.pixelSize: 10; font.weight: Font.DemiBold
                                }
                            }
                            KebabButton {
                                Layout.alignment: Qt.AlignVCenter
                                onClicked: (sx, sy) => card._openMenu(sx, sy)
                            }
                            MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                        }

                        // Project · type · priority
                        Text {
                            Layout.fillWidth: true
                            color: Theme.text3; font.pixelSize: 11
                            elide: Text.ElideRight
                            text: {
                                var proj = ((card.model.project_number || "") + " " + (card.model.project_name || "")).trim()
                                var parts = [proj, (card.model.item_type || "").toString(),
                                             (card.model.project_status || "").toString(),
                                             (card.model.priority || "").toString()]
                                return parts.filter(function(x){ return x && x.toString().trim() !== "" }).join("  ·  ")
                            }
                        }

                        // Field-value metadata (same fields the Widgets All Items grid shows).
                        Flow {
                            Layout.fillWidth: true
                            spacing: 14
                            MetaPair { label: qsTr("Assigned");   value: page._pname(card.model.assigned_to) }
                            MetaPair { label: qsTr("Identified By"); value: page._pname(card.model.identified_by) }
                            MetaPair { label: qsTr("Identified"); value: (card.model.date_identified || "").toString() }
                            MetaPair { label: qsTr("Due");        value: (card.model.date_due || "").toString() }
                            MetaPair { label: qsTr("Updated");    value: (card.model.last_update || "").toString() }
                            MetaPair { label: qsTr("Resolved");   value: (card.model.date_resolved || "").toString() }
                        }

                        // Description, with a compact preview of the most
                        // recent comments alongside it so they're visible
                        // without opening the item.
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14
                            Text {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 2
                                visible: text !== ""
                                text: (card.model.description || "").toString()
                                color: Theme.text2; font.pixelSize: 12
                                wrapMode: Text.WordWrap; elide: Text.ElideRight
                                maximumLineCount: 2
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                Layout.alignment: Qt.AlignTop
                                visible: card._recentComments.length > 0
                                spacing: 2
                                Repeater {
                                    model: card._recentComments
                                    delegate: RowLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        spacing: 5
                                        MaterialIcon { name: "forum"; size: 11; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                                        Text {
                                            text: (modelData.note || "").toString()
                                            color: Theme.text3; font.pixelSize: 11
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                            }
                        }
                    }
                    // Populate the shared context menu for this row and open it at
                    // the given scene coordinates — shared by right-click and kebab.
                    function _openMenu(sx, sy) {
                        page._ctxId = card.iid
                        ctxMenu.recordLabel = (card.model.item_name || "").toString()
                        ctxMenu.quickFilters = page._quickFiltersForRow(card.model)
                        ctxMenu.openAt(sx, sy)
                    }

                    HoverHandler { id: hover }
                    TapHandler { onTapped: page.itemActivated(card.iid) }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (ev) => card._openMenu(ev.scenePosition.x, ev.scenePosition.y)
                    }
        }
    }
}
