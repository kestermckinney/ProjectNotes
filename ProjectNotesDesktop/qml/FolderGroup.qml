// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// One collapsible folder group in the project sidebar. Its header + body form a
// DropArea: dropping a project row here adds it to this folder (multi-folder —
// the project keeps its other memberships). The "All Projects" variant
// (isAll = true) lists every project and, on drop, removes all folder
// memberships from the dropped project.
Column {
    id: group

    property string folderId: ""
    property string folderName: "Folder"
    property string folderIcon: "folder"
    property color  folderColor: Theme.amber
    property int    folderCount: 0
    property bool   isAll: false

    // Shared projects proxy model + selection state (owned by ProjectSidebar).
    property var    listModel: null
    property string selectedProjectId: ""

    // Bumped by ProjectSidebar on FolderManager.foldersChanged so membership
    // bindings below re-evaluate (isProjectInFolder is an imperative call).
    property int    membershipRev: 0

    // Overlay layer the dragged row reparents onto while dragging.
    property var    dragLayer: null

    signal projectActivated(string projectId)

    // Handle a project dropped onto this group.
    function _handleDrop(drop) {
        var pid = drop.source ? drop.source.projectId : ""
        if (!pid)
            return
        if (group.isAll)
            FolderManager.removeProjectFromAllFolders(pid)
        else
            FolderManager.addProjectToFolder(pid, group.folderId)
        drop.accept()
    }

    property bool expanded: true

    spacing: 2
    width: parent ? parent.width : 0

    // ── Header (drop target) ──────────────────────────────────────────────────
    Item {
        width: parent.width
        height: 26

        DropArea {
            id: headerDrop
            anchors.fill: parent
            keys: ["project"]
            onDropped: (drop) => group._handleDrop(drop)
        }

        Rectangle {
            anchors.fill: parent
            anchors.rightMargin: 2
            radius: Theme.radiusSm
            color: headerDrop.containsDrag ? Theme.dropHighlight : "transparent"
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            spacing: 6

            MaterialIcon {
                name: group.isAll ? "workspaces" : group.folderIcon
                size: 15
                color: group.isAll ? Theme.text3 : group.folderColor
            }
            Text {
                text: group.folderName.toUpperCase()
                color: Theme.text3
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 0.6
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Text {
                text: group.folderCount
                color: Theme.text3
                font.pixelSize: 10
            }
            MaterialIcon {
                name: group.expanded ? "expand_more" : "chevron_right"
                size: 16
                color: Theme.text3
            }
        }

        TapHandler { onTapped: group.expanded = !group.expanded }
    }

    // ── Body: project rows ────────────────────────────────────────────────────
    Repeater {
        model: group.expanded ? group.listModel : null

        delegate: Item {
            id: row
            required property int index
            required property var model

            readonly property string projId: model.id !== undefined ? model.id : ""
            // Re-evaluates when membershipRev changes.
            readonly property bool isMember:
                group.isAll || (group.membershipRev >= 0
                                && FolderManager.isProjectInFolder(projId, group.folderId))

            width: group.width
            height: isMember ? 30 : 0
            visible: isMember
            clip: true

            Rectangle {
                id: content
                width: row.width - 4
                height: 28
                x: 0; y: 1
                radius: Theme.radiusSm
                color: {
                    if (drag.active) return Theme.surface2
                    if (row.projId === group.selectedProjectId) return Theme.accentSoft
                    return hover.hovered ? Theme.surface2 : "transparent"
                }

                property string projectId: row.projId

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    Rectangle {
                        width: 6; height: 6; radius: 3
                        color: {
                            var s = (row.model.project_status || "").toString().toLowerCase()
                            if (s.indexOf("active") >= 0) return Theme.green
                            if (s.indexOf("hold") >= 0)   return Theme.amber
                            if (s.indexOf("closed") >= 0) return Theme.text3
                            return Theme.accent
                        }
                    }
                    Text {
                        text: (row.model.project_number || "").toString()
                        color: Theme.text2
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        Layout.preferredWidth: 36
                    }
                    Text {
                        text: (row.model.project_name || "").toString()
                        color: Theme.text
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                HoverHandler { id: hover }
                TapHandler {
                    onTapped: group.projectActivated(row.projId)
                }

                // ── Drag source ───────────────────────────────────────────────
                Drag.active: drag.active
                Drag.keys: ["project"]
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2

                DragHandler {
                    id: drag
                    dragThreshold: 6
                    onActiveChanged: {
                        if (!active) {
                            // Snap the row back into its list slot after the drop.
                            content.x = 0
                            content.y = 1
                        }
                    }
                }

                states: State {
                    when: drag.active
                    ParentChange {
                        target: content
                        parent: group.dragLayer ? group.dragLayer : content.parent
                    }
                }
            }
        }
    }
}
