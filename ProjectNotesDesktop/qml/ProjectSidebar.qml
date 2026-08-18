// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Project sidebar: brand row and the folder groups (each a drop target)
// followed by the implicit "All Projects" group.
Rectangle {
    id: sidebar
    color: Theme.sidebar

    // Panel width is user-resizable (see the drag handle below) and persisted
    // per-user (local QSettings, not the synced application_settings table) —
    // same pattern as ProjectDetailPage's header-height handle. Starts at the
    // design-system default until a saved width is restored below.
    property int panelWidth: Theme.sidebarWidth
    readonly property int minWidth: 160
    readonly property int maxWidth: 480
    implicitWidth: sidebar.panelWidth

    Component.onCompleted: {
        var saved = DesktopAppController.projectSidebarWidth()
        if (saved > 0)
            sidebar.panelWidth = Math.min(sidebar.maxWidth, Math.max(sidebar.minWidth, saved))
    }

    property string selectedProjectId: ""
    property var    dragLayer: null
    signal projectActivated(string projectId)
    // Routed to Main (same contract as ProjectsListPage) so the sidebar's
    // right-click / kebab menu can export a project or open the filter dialog.
    signal exportRequested(string table, string id)
    signal filterRequested()
    signal sortRequested(real sx, real sy)
    // A tracker item was dropped on a project row; forwarded from FolderGroup.
    signal itemMoveRequested(string itemId, string projectId)

    // Shared record/plugin context menu for every project row in the sidebar
    // (all FolderGroups funnel their right-click and kebab clicks here through
    // openProjectMenu). One popup instance instead of one per row.
    property string _ctxId: ""
    RecordContextMenu {
        id: projCtxMenu
        recordType: qsTr("Project")
        model: DesktopAppController.projectsListModel
        recordId: sidebar._ctxId
        canMoveTo: true
        onOpenRequested:   sidebar.projectActivated(sidebar._ctxId)
        onNewRequested: {
            var r = DesktopAppController.addProject()
            if (r >= 0) sidebar.projectActivated(DesktopAppController.projectIdAtRow(r))
        }
        onDeleteRequested: DesktopAppController.deleteProject(
                               DesktopAppController.projectRowForId(sidebar._ctxId))
        onDuplicateRequested: {
            // ProjectsModel::copyRecord also brings the project's team across.
            var newId = DesktopAppController.duplicateRecord(
                            DesktopAppController.projectsListModel, sidebar._ctxId)
            if (newId !== "") sidebar.projectActivated(newId)
        }
        onMoveToRequested: moveToFolderDialog.openFor(sidebar._ctxId, projCtxMenu.recordLabel)
        onExportRequested: sidebar.exportRequested("projects", sidebar._ctxId)
        onFilterRequested: sidebar.filterRequested()
        onSortRequested: (sx, sy) => sidebar.sortRequested(sx, sy)
        onRefreshRequested: DesktopAppController.refreshModel(DesktopAppController.projectsListModel)
    }

    MoveToFolderDialog { id: moveToFolderDialog }

    // Called by a FolderGroup row (right-click or kebab) to open the shared menu
    // for a given project at scene coordinates.
    function openProjectMenu(projId, label, sx, sy) {
        sidebar._ctxId = projId
        projCtxMenu.recordLabel = label
        projCtxMenu.openAt(sx, sy)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            Layout.bottomMargin: 6
            spacing: 7
            Rectangle {
                width: 20; height: 20; radius: Theme.radiusSm; color: Theme.accent
                MaterialIcon { anchors.centerIn: parent; name: "description"; size: 13; color: "#ffffff" }
            }
            Text {
                text: "Project Notes"
                color: Theme.text
                font.pixelSize: Theme.fontXl
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
        }

        // Folder groups + All Projects
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Column {
                width: sidebar.width
                spacing: 4
                topPadding: 2
                bottomPadding: 10
                leftPadding: 6
                rightPadding: 6

                Repeater {
                    model: FolderManager.folders
                    delegate: FolderGroup {
                        required property var modelData
                        width: parent.width - 12
                        folderId:    modelData.id
                        folderName:  modelData.name
                        folderIcon:  modelData.icon
                        folderColor: modelData.color
                        folderCount: modelData.count
                        isAll:       false
                        expanded:    !modelData.collapsed
                        selectedProjectId: sidebar.selectedProjectId
                        dragLayer:         sidebar.dragLayer
                        onProjectActivated: (pid) => sidebar.projectActivated(pid)
                        onMenuRequested: (pid, label, sx, sy) => sidebar.openProjectMenu(pid, label, sx, sy)
                        onItemMoveRequested: (itemId, pid) => sidebar.itemMoveRequested(itemId, pid)
                        onToggled: (exp) => FolderManager.setFolderCollapsed(modelData.id, !exp)
                    }
                }

                FolderGroup {
                    width: parent.width - 12
                    folderId: "__uncategorized__"
                    folderName: "Not Categorized"
                    isAll: false
                    isUncategorized: true
                    expanded: !FolderManager.uncategorizedCollapsed
                    selectedProjectId: sidebar.selectedProjectId
                    dragLayer:         sidebar.dragLayer
                    onProjectActivated: (pid) => sidebar.projectActivated(pid)
                    onMenuRequested: (pid, label, sx, sy) => sidebar.openProjectMenu(pid, label, sx, sy)
                    onItemMoveRequested: (itemId, pid) => sidebar.itemMoveRequested(itemId, pid)
                    onToggled: (exp) => FolderManager.uncategorizedCollapsed = !exp
                }

                FolderGroup {
                    width: parent.width - 12
                    folderName: "All Projects"
                    isAll: true
                    expanded: !FolderManager.allProjectsCollapsed
                    selectedProjectId: sidebar.selectedProjectId
                    dragLayer:         sidebar.dragLayer
                    onProjectActivated: (pid) => sidebar.projectActivated(pid)
                    onMenuRequested: (pid, label, sx, sy) => sidebar.openProjectMenu(pid, label, sx, sy)
                    onItemMoveRequested: (itemId, pid) => sidebar.itemMoveRequested(itemId, pid)
                    onToggled: (exp) => FolderManager.allProjectsCollapsed = !exp
                }
            }
        }
    }

    // ── Width resize handle ──────────────────────────────────────────────────
    // Drags sidebar.panelWidth; persisted per-user via
    // DesktopAppController.setProjectSidebarWidth on release — same pattern as
    // ProjectDetailPage's header resize handle (see _headerHeight there).
    Rectangle {
        id: widthResizeHandle
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 2
        color: (handleArea.pressed || handleArea.containsMouse) ? Theme.accent : "transparent"

        MouseArea {
            id: handleArea
            anchors.fill: parent
            anchors.margins: -3
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            property real dragStartX: 0
            property real dragStartWidth: 0
            onPressed: (mouse) => {
                dragStartX = mapToItem(sidebar, mouse.x, mouse.y).x
                dragStartWidth = sidebar.panelWidth
            }
            onPositionChanged: (mouse) => {
                if (!pressed) return
                var currentX = mapToItem(sidebar, mouse.x, mouse.y).x
                sidebar.panelWidth = Math.min(sidebar.maxWidth,
                                     Math.max(sidebar.minWidth, dragStartWidth + (currentX - dragStartX)))
            }
            onReleased: DesktopAppController.setProjectSidebarWidth(sidebar.panelWidth)
        }
    }
}
