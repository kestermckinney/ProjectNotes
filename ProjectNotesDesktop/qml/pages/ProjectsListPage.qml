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

            Repeater {
                model: DesktopAppController.projectsListModel

                delegate: Card {
                    id: card
                    required property int index
                    required property var model
                    readonly property string projId: model.id !== undefined ? model.id : ""

                    Layout.fillWidth: true
                    implicitHeight: 74
                    color: (projId === page.selectedProjectId)
                           ? Theme.accentSoft
                           : (hover.hovered ? Theme.raise : Theme.surface)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
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

                        MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3 }
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
}
