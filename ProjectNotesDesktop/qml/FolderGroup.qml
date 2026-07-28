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

    // Shared projects proxy model + selection state (owned by ProjectSidebar).
    property var    listModel: null
    property string selectedProjectId: ""

    // Bumped by ProjectSidebar on FolderManager.foldersChanged so membership
    // bindings below re-evaluate (isProjectInFolder is an imperative call).
    property int    membershipRev: 0

    // Overlay layer the dragged row reparents onto while dragging.
    property var    dragLayer: null

    signal projectActivated(string projectId)
    // A project row wants the shared record/plugin menu opened for it, at the
    // given scene coordinates. Handled by ProjectSidebar.openProjectMenu.
    signal menuRequested(string projId, string label, real sceneX, real sceneY)

    // "All Projects" has no FolderManager count — derive it live from the list
    // model. Instantiator.count is reactive (and follows the proxy's quick-search
    // filter, so it reflects the *visible* rows), but it only counts rows for
    // which it actually instantiates an object — so it MUST have a delegate.
    // Without one it instantiates nothing and count stays 0. A bare non-visual
    // QtObject is enough just to count.
    Instantiator {
        id: allCounter
        active: group.isAll
        model: group.isAll ? group.listModel : null
        delegate: QtObject {}
    }

    // Bumped whenever the shared list model's visible rows change (the sidebar
    // quick-search or the column-filter editor), so a named folder recounts the
    // members that survive the active filter.
    property int filterRev: 0
    Connections {
        target: group.listModel
        enabled: !group.isAll
        function onModelReset()    { group.filterRev++ }
        function onRowsInserted()  { group.filterRev++ }
        function onRowsRemoved()   { group.filterRev++ }
        function onLayoutChanged() { group.filterRev++ }
    }

    // "All Projects" counts every visible row via the Instantiator above. A named
    // folder counts only its members that are currently visible in the filtered
    // model; with no filter active that equals its full membership. filterRev and
    // membershipRev are touched so the count re-evaluates on filter/membership
    // changes (folderVisibleCount is an imperative call).
    readonly property int displayCount: {
        if (group.isAll)
            return allCounter.count
        var _dep = group.filterRev + group.membershipRev
        return _dep >= 0
            ? DesktopAppController.folderVisibleCount(group.listModel, group.folderId)
            : 0
    }

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
                text: group.displayCount
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
            readonly property string ctxLabel:
                ((model.project_number || "") + " " + (model.project_name || "")).trim()
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
                    if (dragArea.drag.active) return Theme.surface2
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

                // Passive hover across the whole row (coexists with the drag
                // MouseArea) — drives the kebab's reveal.
                HoverHandler { id: rowHover }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    Rectangle {
                        implicitWidth: 6; implicitHeight: 6; radius: 3
                        Layout.alignment: Qt.AlignVCenter
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
                        // Natural width, capped + elided so a long number never
                        // paints over the project name beside it.
                        Layout.maximumWidth: 58
                        elide: Text.ElideRight
                    }
                    Text {
                        text: (row.model.project_name || "").toString()
                        color: Theme.text
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    // Kebab: revealed on hover. It sits in the slot the drag
                    // MouseArea vacates (rightMargin below), so its own tap is
                    // not swallowed by the drag handler.
                    KebabButton {
                        id: kebab
                        implicitWidth: 22; implicitHeight: 22
                        visible: rowHover.hovered
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
                    // Uncover the kebab's slot while it's visible so its own tap
                    // lands on the button rather than on this drag handler.
                    anchors.rightMargin: kebab.visible ? kebab.width + 10 : 0
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
