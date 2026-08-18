// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQml
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
    // The implicit "Not Categorized" group (projects that are members of no
    // folder). Like isAll, dropping a project here clears all its folder
    // memberships rather than adding it to a (nonexistent) folder.
    property bool   isUncategorized: false

    // Selection state (owned by ProjectSidebar).
    property string selectedProjectId: ""

    // Overlay layer the dragged row reparents onto while dragging.
    property var    dragLayer: null

    signal projectActivated(string projectId)
    // A project row wants the shared record/plugin menu opened for it, at the
    // given scene coordinates. Handled by ProjectSidebar.openProjectMenu.
    signal menuRequested(string projId, string label, real sceneX, real sceneY)
    // A tracker item card (Drag.keys ["trackerItem"]) was dropped on a project
    // row. Handled by ProjectSidebar → Main.requestTrackerItemMove.
    signal itemMoveRequested(string itemId, string projectId)
    // The header was tapped and `expanded` flipped to the given value. The
    // instantiating site (ProjectSidebar) persists this via FolderManager.
    signal toggled(bool expandedNow)

    // Visible members of this folder ("" = every visible project), served from
    // the controller's one-pass snapshot. Referencing sidebarRev makes the
    // binding re-fetch whenever the projects proxy or folder memberships change
    // (folderProjects is an imperative call). Only member rows are instantiated
    // — the old approach repeated the full projects model in every group and
    // hid non-members, making the sidebar folders × projects delegates big.
    readonly property var members: {
        var _rev = DesktopAppController.sidebarRev
        return DesktopAppController.folderProjects(group.isAll ? "" : group.folderId)
    }

    readonly property int displayCount: members.length

    // Handle a project dropped onto this group.
    function _handleDrop(drop) {
        var pid = drop.source ? drop.source.projectId : ""
        if (!pid)
            return
        if (group.isAll || group.isUncategorized)
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
        height: 22

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
            anchors.leftMargin: 5
            anchors.rightMargin: 5
            spacing: 5

            MaterialIcon {
                name: group.isAll ? "workspaces" : (group.isUncategorized ? "folder_off" : group.folderIcon)
                size: 13
                color: (group.isAll || group.isUncategorized) ? Theme.text3 : group.folderColor
            }
            Text {
                text: group.folderName.toUpperCase()
                color: Theme.text3
                font.pixelSize: Theme.listFont
                font.weight: Font.Bold
                font.letterSpacing: 0.6
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Text {
                text: group.displayCount
                color: Theme.text3
                font.pixelSize: Theme.listFont
            }
            MaterialIcon {
                name: group.expanded ? "expand_more" : "chevron_right"
                size: 14
                color: Theme.text3
            }
        }

        TapHandler {
            onTapped: {
                group.expanded = !group.expanded
                group.toggled(group.expanded)
            }
        }
    }

    // ── Body: project rows (members only — see `members` above) ──────────────
    Repeater {
        model: group.expanded ? group.members : []

        delegate: Item {
            id: row
            required property int index
            required property var modelData

            readonly property string projId: (modelData.id || "").toString()
            readonly property string ctxLabel:
                ((modelData.project_number || "") + " " + (modelData.project_name || "")).trim()

            width: group.width
            height: 26
            clip: true

            Rectangle {
                id: content
                width: row.width - 4
                height: 24
                x: 0; y: 1
                radius: Theme.radiusSm
                color: {
                    if (dragArea.drag.active) return Theme.surface2
                    if (itemDrop.containsDrag) return Theme.dropHighlight
                    if (row.projId === group.selectedProjectId) return Theme.accentSoft
                    return dragArea.containsMouse ? Theme.surface2 : "transparent"
                }

                property string projectId: row.projId

                // ── Drag source (driven by the MouseArea below) ───────────────
                Drag.active: dragArea.drag.active
                Drag.source: content
                Drag.keys: ["project"]
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2

                // ── Drop target for a dragged tracker item card ────────────────
                // Dropping a tracker item (ProjectDetailPage's trackerCard) here
                // requests moving it to this row's project.
                DropArea {
                    id: itemDrop
                    anchors.fill: parent
                    keys: ["trackerItem"]
                    onDropped: (drop) => {
                        if (drop.source && drop.source.itemId)
                            group.itemMoveRequested(drop.source.itemId, row.projId)
                        drop.accept()
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 6

                    Rectangle {
                        implicitWidth: 5; implicitHeight: 5; radius: 2.5
                        Layout.alignment: Qt.AlignVCenter
                        color: {
                            var s = (row.modelData.project_status || "").toString().toLowerCase()
                            if (s.indexOf("active") >= 0) return Theme.green
                            if (s.indexOf("hold") >= 0)   return Theme.amber
                            if (s.indexOf("closed") >= 0) return Theme.text3
                            return Theme.accent
                        }
                    }
                    Text {
                        text: (row.modelData.project_number || "").toString()
                        color: Theme.text2
                        font.pixelSize: Theme.listFont
                        // Natural width, capped + elided so a long number never
                        // paints over the project name beside it.
                        Layout.maximumWidth: 52
                        elide: Text.ElideRight
                    }
                    Text {
                        text: (row.modelData.project_name || "").toString()
                        color: Theme.text
                        font.pixelSize: Theme.listFont
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    // Kebab sits in the slot the drag MouseArea vacates (rightMargin
                    // below), so its own tap is not swallowed by the drag handler.
                    KebabButton {
                        id: kebab
                        implicitWidth: 20; implicitHeight: 20
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: (sx, sy) => group.menuRequested(row.projId, row.ctxLabel, sx, sy)
                    }
                }

                // Tap to open, drag to move the project into a folder. A
                // MouseArea (not a DragHandler) is used on purpose:
                //  • preventStealing stops the enclosing ScrollView/Flickable
                //    from hijacking the vertical drag for scrolling — that steal
                //    silently prevented the old DragHandler from ever activating.
                //  • the drop is delivered explicitly on release, so it lands on
                //    the folder header under the cursor instead of racing the
                //    ParentChange revert that snaps the row back to its slot.
                // On release the ParentChange state exits and restores content's
                // saved parent + x/y (0,1), so no manual position reset is needed.
                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    // Uncover the kebab's slot so its own tap lands on the button
                    // rather than on this drag handler.
                    anchors.rightMargin: kebab.width + 10
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    preventStealing: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    drag.target: content
                    drag.threshold: 6
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            var p = dragArea.mapToItem(null, mouse.x, mouse.y)
                            group.menuRequested(row.projId, row.ctxLabel, p.x, p.y)
                        } else {
                            group.projectActivated(row.projId)
                        }
                    }
                    onReleased: if (drag.active) content.Drag.drop()
                }

                states: State {
                    when: dragArea.drag.active
                    ParentChange {
                        target: content
                        parent: group.dragLayer ? group.dragLayer : content.parent
                    }
                }
            }
        }
    }
}
