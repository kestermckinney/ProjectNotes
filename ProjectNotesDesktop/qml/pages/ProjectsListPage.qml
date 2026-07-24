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

    property string _ctxId: ""

    RecordContextMenu {
        id: ctxMenu
        recordType: qsTr("Project")
        onOpenRequested:   page.projectActivated(page._ctxId)
        onNewRequested: {
            var r = DesktopAppController.addProject()
            if (r >= 0) page.projectActivated(DesktopAppController.projectIdAtRow(r))
        }
        onDeleteRequested: DesktopAppController.deleteProject(DesktopAppController.projectRowForId(page._ctxId))
        onExportRequested: page.exportRequested("projects", page._ctxId)
        onFilterRequested: page.filterRequested()
        onRefreshRequested: DesktopAppController.refreshModel(DesktopAppController.projectsListModel)
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 16
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: page.width - 32
            spacing: 10

            // Header row: result count + Show Internal toggle (mockup parity)
            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 2
                spacing: 10

                Text {
                    text: projRepeater.count + (projRepeater.count === 1 ? qsTr(" project")
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

            Repeater {
                id: projRepeater
                model: DesktopAppController.projectsListModel

                delegate: Card {
                    id: card
                    required property int index
                    required property var model
                    readonly property string projId: model.id !== undefined ? model.id : ""
                    readonly property bool showFin: DesktopAppController.showInternalItems

                    Layout.fillWidth: true
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
                            Text {
                                text: (card.model.project_number || "").toString()
                                color: Theme.accent
                                font.pixelSize: 15
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
                        }

                        MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                    }

                    // Financial strip — the same "internal" columns the Widgets
                    // app reveals under View ▸ Internal Items. Wraps to a second
                    // row on narrow cards; each chip elides so nothing bleeds over.
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
                        MetricChip { label: qsTr("Consumed"); value: (card.model.pct_consumed || "").toString() }
                        MetricChip { label: qsTr("EAC");      value: (card.model.eac || "").toString(); accentColor: Theme.amber }
                        MetricChip { label: qsTr("CV");       value: (card.model.cv || "").toString() }
                        MetricChip { label: qsTr("SV");       value: (card.model.sv || "").toString() }
                        MetricChip { label: qsTr("CPI");      value: (card.model.cpi || "").toString() }
                        MetricChip { label: qsTr("Complete"); value: (card.model.pct_complete || "").toString() }
                    }
                    }

                    HoverHandler { id: hover }
                    TapHandler { onTapped: page.projectActivated(card.projId) }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (ev) => {
                            page._ctxId = card.projId
                            ctxMenu.recordLabel = (card.model.project_number || "") + " "
                                                  + (card.model.project_name || "")
                            ctxMenu.openAt(ev.scenePosition.x, ev.scenePosition.y)
                        }
                    }
                    // Drag-to-folder is available from the sidebar's project rows
                    // (incl. the "All Projects" group). List-card drag is a later
                    // enhancement — see plan Phase 2.
                }
            }
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
