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
    implicitWidth: Theme.sidebarWidth

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
            Layout.margins: 14
            Layout.bottomMargin: 8
            spacing: 8
            Rectangle {
                width: 22; height: 22; radius: 6; color: Theme.accent
                MaterialIcon { anchors.centerIn: parent; name: "description"; size: 14; color: "#ffffff" }
            }
            Text {
                text: "Project Notes"
                color: Theme.text
                font.pixelSize: 15
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
                spacing: 6
                topPadding: 2
                bottomPadding: 12
                leftPadding: 8
                rightPadding: 8

                Repeater {
                    model: FolderManager.folders
                    delegate: FolderGroup {
                        required property var modelData
                        width: parent.width - 16
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
                    width: parent.width - 16
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
                    width: parent.width - 16
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
}
