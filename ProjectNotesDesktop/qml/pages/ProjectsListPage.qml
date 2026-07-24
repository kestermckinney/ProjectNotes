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
                    implicitHeight: showFin ? 112 : 74
                    color: (projId === page.selectedProjectId)
                           ? Theme.accentSoft
                           : (hover.hovered ? Theme.raise : Theme.surface)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        anchors.topMargin: 10
                        anchors.bottomMargin: 10
                        spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: !card.showFin
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

                    // Financial strip — shown only when "Show Internal" is on.
                    RowLayout {
                        Layout.fillWidth: true
                        visible: card.showFin
                        spacing: 8
                        MetricChip { label: qsTr("Budget"); value: (card.model.budget || "").toString() }
                        MetricChip { label: qsTr("Actual"); value: (card.model.actual || "").toString() }
                        MetricChip { label: qsTr("BCWP");   accentColor: Theme.green; value: (card.model.bcwp || "").toString() }
                        MetricChip {
                            label: qsTr("EAC"); accentColor: Theme.amber
                            value: (card.model.eac || "").toString() !== "" ? (card.model.eac || "").toString()
                                                                            : (card.model.bac || "").toString()
                        }
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
    // Shares row width equally and elides so long values never bleed over siblings.
    component MetricChip: ColumnLayout {
        id: chip
        property string label: ""
        property string value: ""
        property color accentColor: Theme.text
        readonly property bool _has: value !== undefined && value.toString().trim() !== ""
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        spacing: 1
        Text {
            text: chip.label.toUpperCase(); color: Theme.text3
            font.pixelSize: 9; font.weight: Font.Bold
            Layout.fillWidth: true; elide: Text.ElideRight
        }
        Text {
            text: chip._has ? chip.value : "—"
            color: chip._has ? chip.accentColor : Theme.text3
            font.pixelSize: 12; font.weight: Font.DemiBold
            Layout.fillWidth: true; elide: Text.ElideRight
        }
    }
}
