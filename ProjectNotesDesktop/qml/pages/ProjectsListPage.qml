// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Master projects list, rendered as cards (mockup style). Bound to the shared
// projects proxy model from DesktopAppController. Rows are drag sources so a
// project can be dragged onto a sidebar folder.
Item {
    id: page
    property string selectedProjectId: ""
    property var    dragLayer: null
    signal projectActivated(string projectId)
    signal exportRequested(string table, string id)
    signal filterRequested()
    signal sortRequested(real sx, real sy)

    property string _ctxId: ""

    // Quick Filter entries for a right-clicked project row: pre-filled from
    // that row's own client/contact (omitted when the row has none set),
    // plus the two always-available overdue toggles — status_overdue/
    // invoicing_overdue are computed "Yes"/"No" columns mirroring the same
    // red-highlight logic the Status/Invoice Date chips already show (see
    // projectsmodel.cpp).
    function _quickFiltersForRow(m) {
        var qf = []
        var clientId = (m.client_id || "").toString()
        if (clientId !== "")
            qf.push({ icon: "apartment", label: qsTr("This Client"), field: "client_id", values: [clientId] })
        var contactId = (m.primary_contact || "").toString()
        if (contactId !== "")
            qf.push({ icon: "person", label: qsTr("This Contact"), field: "primary_contact", values: [contactId] })
        qf.push({ icon: "event_busy", label: qsTr("Status Overdue"), field: "status_overdue", values: ["Yes"] })
        qf.push({ icon: "receipt_long", label: qsTr("Invoicing Overdue"), field: "invoicing_overdue", values: ["Yes"] })
        return qf
    }

    RecordContextMenu {
        id: ctxMenu
        recordType: qsTr("Project")
        model: DesktopAppController.projectsListModel
        recordId: page._ctxId
        canMoveTo: true
        onOpenRequested:   page.projectActivated(page._ctxId)
        onNewRequested: {
            var r = DesktopAppController.addProject()
            if (r >= 0) page.projectActivated(DesktopAppController.projectIdAtRow(r))
        }
        onDeleteRequested: DesktopAppController.deleteProject(DesktopAppController.projectRowForId(page._ctxId))
        onMoveToRequested: moveToFolderDialog.openFor(page._ctxId, ctxMenu.recordLabel)
        onExportRequested: page.exportRequested("projects", page._ctxId)
        onFilterRequested: page.filterRequested()
        onSortRequested: (sx, sy) => page.sortRequested(sx, sy)
        onRefreshRequested: DesktopAppController.refreshModel(DesktopAppController.projectsListModel)
    }

    MoveToFolderDialog { id: moveToFolderDialog }

    // Virtualized list — only visible cards (plus cacheBuffer) are instantiated,
    // and reuseItems recycles delegates while scrolling.
    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: 16
        clip: true
        spacing: 10
        model: DesktopAppController.projectsListModel
        reuseItems: true
        cacheBuffer: 800
        boundsBehavior: Flickable.StopAtBounds
        footer: Item { width: 1; height: 8 }
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        // Header row: result count + Show Internal toggle (mockup parity).
        // Scrolls with the content, same as the old in-column header.
        header: Item {
            width: ListView.view ? ListView.view.width : 0
            height: headerRow.implicitHeight + 12

            RowLayout {
                id: headerRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                spacing: 10

                Text {
                    text: list.count + (list.count === 1 ? qsTr(" project")
                                                         : qsTr(" projects"))
                    color: Theme.text3
                    font.pixelSize: 12
                }

                Item { Layout.fillWidth: true }

                // Show-internal (budget/financials) pill toggle
                Rectangle {
                    id: internalToggle
                    readonly property bool on: DesktopAppController.showInternalItems
                    implicitHeight: 32
                    implicitWidth: togRow.implicitWidth + 24
                    radius: 16
                    color: internalToggle.on ? Theme.accentSoft
                                             : (togHover.hovered ? Theme.surface2 : Theme.surface)
                    border.color: internalToggle.on ? Theme.accent : Theme.border

                    RowLayout {
                        id: togRow
                        anchors.centerIn: parent
                        spacing: 8
                        MaterialIcon {
                            name: "attach_money"; size: 16
                            color: internalToggle.on ? Theme.accent : Theme.text3
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Text {
                            text: qsTr("Show Internal")
                            color: internalToggle.on ? Theme.accent : Theme.text2
                            font.pixelSize: 12; font.weight: Font.DemiBold
                            verticalAlignment: Text.AlignVCenter
                            Layout.alignment: Qt.AlignVCenter
                        }
                        // switch track
                        Rectangle {
                            implicitWidth: 32; implicitHeight: 18; radius: 9
                            Layout.alignment: Qt.AlignVCenter
                            color: internalToggle.on ? Theme.accent : Theme.surface2
                            border.color: internalToggle.on ? Theme.accent : Theme.border
                            Rectangle {
                                width: 14; height: 14; radius: 7
                                y: 2; x: internalToggle.on ? parent.width - width - 2 : 2
                                color: "#ffffff"
                                Behavior on x { NumberAnimation { duration: 110 } }
                            }
                        }
                    }
                    HoverHandler { id: togHover }
                    TapHandler {
                        onTapped: DesktopAppController.showInternalItems = !DesktopAppController.showInternalItems
                    }
                }
            }
        }

        delegate: Card {
                    id: card
                    required property int index
                    required property var model
                    readonly property string projId: model.id !== undefined ? model.id : ""
                    readonly property bool showFin: DesktopAppController.showInternalItems

                    width: ListView.view ? ListView.view.width : 0
                    // Height follows content: taller when the financial strip
                    // (which may wrap to two rows) is visible.
                    implicitHeight: contentCol.implicitHeight + 20
                    color: (projId === page.selectedProjectId)
                           ? Theme.accentSoft
                           : (hover.hovered ? Theme.raise : Theme.surface)

                    ColumnLayout {
                        id: contentCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        anchors.topMargin: 10
                        spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 14

                        // Number + status
                        ColumnLayout {
                            spacing: 4
                            Layout.preferredWidth: 70
                            Layout.alignment: Qt.AlignTop
                            Text {
                                text: (card.model.project_number || "").toString()
                                color: Theme.accent
                                font.pixelSize: 14
                                font.weight: Font.Bold
                            }
                            Rectangle {
                                readonly property string st:
                                    (card.model.project_status || "").toString()
                                readonly property color pillColor: {
                                    var s = st.toLowerCase()
                                    if (s.indexOf("active") >= 0) return Theme.green
                                    if (s.indexOf("hold") >= 0)   return Theme.amber
                                    if (s.indexOf("closed") >= 0) return Theme.text3
                                    return Theme.accent
                                }
                                visible: st !== ""
                                radius: 4
                                color: Qt.rgba(pillColor.r, pillColor.g, pillColor.b, 0.14)
                                implicitHeight: 16
                                implicitWidth: pill.implicitWidth + 12
                                Text {
                                    id: pill
                                    anchors.centerIn: parent
                                    text: parent.st
                                    color: parent.pillColor
                                    font.pixelSize: 10
                                    font.weight: Font.DemiBold
                                }
                            }
                        }

                        // Name + client
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 3
                            Text {
                                text: (card.model.project_name || "").toString()
                                color: Theme.text
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            RowLayout {
                                spacing: 6
                                visible: clientText.text !== ""
                                MaterialIcon { name: "apartment"; size: 14; color: Theme.text3 }
                                Text {
                                    id: clientText
                                    text: DesktopAppController.clientNameForId(
                                              (card.model.client_id || "").toString())
                                    color: Theme.text2
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                spacing: 6
                                visible: contactText.text !== ""
                                MaterialIcon { name: "person"; size: 14; color: Theme.text3 }
                                Text {
                                    id: contactText
                                    text: DesktopAppController.peopleNameForId(
                                              (card.model.primary_contact || "").toString())
                                    color: Theme.text2
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        KebabButton {
                            Layout.alignment: Qt.AlignVCenter
                            onClicked: (sx, sy) => card._openMenu(sx, sy)
                        }
                        MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                    }

                    // Status/invoice dates + reporting periods — always visible (not
                    // gated behind "Show Internal"): these drive whether a project
                    // needs attention and are checked constantly. Date colors come
                    // straight from the abstract data model's ForegroundRole (red/
                    // yellow when a status or invoice is overdue for its period),
                    // via the SqlQueryModel-generated <col>_foreground roles.
                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: 2
                        spacing: 10
                        MetricChip {
                            label: qsTr("Status Date")
                            value: (card.model.last_status_date || "").toString()
                            accentColor: card.model.last_status_date_foreground || Theme.text
                        }
                        MetricChip {
                            label: qsTr("Invoice Date")
                            value: (card.model.last_invoice_date || "").toString()
                            accentColor: card.model.last_invoice_date_foreground || Theme.text
                        }
                        MetricChip { label: qsTr("Invoice Period"); value: (card.model.invoicing_period || "").toString() }
                        MetricChip { label: qsTr("Report Period");  value: (card.model.status_report_period || "").toString() }
                    }

                    // Financial strip — the same "internal" columns the Widgets
                    // app reveals under View ▸ Internal Items. Wraps to a second
                    // row on narrow cards; each chip elides so nothing bleeds over.
                    // Colors likewise come from the model's ForegroundRole where it
                    // defines one (Consumed/CV/SV/Complete/CPI threshold flags);
                    // BCWP/EAC keep their fixed accents since the model has no
                    // color opinion on those two.
                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: 2
                        visible: card.showFin
                        spacing: 10
                        MetricChip { label: qsTr("Budget");   value: (card.model.budget || "").toString() }
                        MetricChip { label: qsTr("Actual");   value: (card.model.actual || "").toString() }
                        MetricChip { label: qsTr("BCWP");     value: (card.model.bcwp || "").toString(); accentColor: Theme.green }
                        MetricChip { label: qsTr("BCWS");     value: (card.model.bcws || "").toString() }
                        MetricChip { label: qsTr("BAC");      value: (card.model.bac || "").toString() }
                        MetricChip {
                            label: qsTr("Consumed")
                            value: (card.model.pct_consumed || "").toString()
                            accentColor: card.model.pct_consumed_foreground || Theme.text
                        }
                        MetricChip { label: qsTr("EAC");      value: (card.model.eac || "").toString(); accentColor: Theme.amber }
                        MetricChip {
                            label: qsTr("CV")
                            value: (card.model.cv || "").toString()
                            accentColor: card.model.cv_foreground || Theme.text
                        }
                        MetricChip {
                            label: qsTr("SV")
                            value: (card.model.sv || "").toString()
                            accentColor: card.model.sv_foreground || Theme.text
                        }
                        MetricChip {
                            label: qsTr("CPI")
                            value: (card.model.cpi || "").toString()
                            accentColor: card.model.cpi_foreground || Theme.text
                        }
                        MetricChip {
                            label: qsTr("Complete")
                            value: (card.model.pct_complete || "").toString()
                            accentColor: card.model.pct_complete_foreground || Theme.text
                        }
                    }
                    }

                    // Populate the shared context menu for this row and open it at
                    // the given scene coordinates — shared by right-click and kebab.
                    function _openMenu(sx, sy) {
                        page._ctxId = card.projId
                        ctxMenu.recordLabel = (card.model.project_number || "") + " "
                                              + (card.model.project_name || "")
                        ctxMenu.quickFilters = page._quickFiltersForRow(card.model)
                        ctxMenu.openAt(sx, sy)
                    }

                    HoverHandler { id: hover }
                    TapHandler { onTapped: page.projectActivated(card.projId) }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (ev) => card._openMenu(ev.scenePosition.x, ev.scenePosition.y)
                    }
                    // Drag-to-folder is available from the sidebar's project rows
                    // (incl. the "All Projects" group). List-card drag is a later
                    // enhancement — see plan Phase 2.
        }
    }

    // Compact financial figure (label + value) used in the card financial strip.
    // Fixed width so it tiles cleanly in a Flow; both lines elide so a long
    // currency value never bleeds over the neighbouring chip.
    component MetricChip: Column {
        id: chip
        property string label: ""
        property string value: ""
        property color accentColor: Theme.text
        readonly property bool _has: value !== undefined && value.toString().trim() !== ""
        width: 96
        spacing: 1
        Text {
            width: parent.width; elide: Text.ElideRight
            text: chip.label.toUpperCase(); color: Theme.text3
            font.pixelSize: 9; font.weight: Font.Bold
        }
        Text {
            width: parent.width; elide: Text.ElideRight
            text: chip._has ? chip.value : "—"
            color: chip._has ? chip.accentColor : Theme.text3
            font.pixelSize: 12; font.weight: Font.DemiBold
        }
    }
}
