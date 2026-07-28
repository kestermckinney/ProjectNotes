// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Project sidebar: brand row, project search, and the folder groups (each a
// drop target) followed by the implicit "All Projects" group.
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

    // Shared record/plugin context menu for every project row in the sidebar
    // (all FolderGroups funnel their right-click and kebab clicks here through
    // openProjectMenu). One popup instance instead of one per row.
    property string _ctxId: ""
    RecordContextMenu {
        id: projCtxMenu
        recordType: qsTr("Project")
        model: DesktopAppController.projectsListModel
        recordId: sidebar._ctxId
        onOpenRequested:   sidebar.projectActivated(sidebar._ctxId)
        onNewRequested: {
            var r = DesktopAppController.addProject()
            if (r >= 0) sidebar.projectActivated(DesktopAppController.projectIdAtRow(r))
        }
        onDeleteRequested: DesktopAppController.deleteProject(
                               DesktopAppController.projectRowForId(sidebar._ctxId))
        onExportRequested: sidebar.exportRequested("projects", sidebar._ctxId)
        onFilterRequested: sidebar.filterRequested()
        onRefreshRequested: DesktopAppController.refreshModel(DesktopAppController.projectsListModel)
    }

    // Called by a FolderGroup row (right-click or kebab) to open the shared menu
    // for a given project at scene coordinates.
    function openProjectMenu(projId, label, sx, sy) {
        sidebar._ctxId = projId
        projCtxMenu.recordLabel = label
        projCtxMenu.openAt(sx, sy)
    }

    // Incremented on every folder/membership change so FolderGroup membership
    // bindings re-evaluate (FolderManager.isProjectInFolder is imperative).
    property int membershipRev: 0
    Connections {
        target: FolderManager
        function onFoldersChanged() { sidebar.membershipRev++ }
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

        // Search
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 8
            implicitHeight: 32
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: Theme.border
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 8
                MaterialIcon { name: "search"; size: 16; color: Theme.text3 }
                TextField {
                    id: search
                    Layout.fillWidth: true
                    placeholderText: "Search projects"
                    color: Theme.text
                    placeholderTextColor: Theme.text3
                    background: null
                    font.pixelSize: 13
                    onTextEdited: DesktopAppController.setQuickSearch(
                                      DesktopAppController.projectsListModel, text)
                }
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
                        listModel:         DesktopAppController.projectsListModel
                        selectedProjectId: sidebar.selectedProjectId
                        membershipRev:     sidebar.membershipRev
                        dragLayer:         sidebar.dragLayer
                        onProjectActivated: (pid) => sidebar.projectActivated(pid)
                        onMenuRequested: (pid, label, sx, sy) => sidebar.openProjectMenu(pid, label, sx, sy)
                    }
                }

                FolderGroup {
                    width: parent.width - 16
                    folderName: "All Projects"
                    isAll: true
                    listModel:         DesktopAppController.projectsListModel
                    selectedProjectId: sidebar.selectedProjectId
                    membershipRev:     sidebar.membershipRev
                    dragLayer:         sidebar.dragLayer
                    onProjectActivated: (pid) => sidebar.projectActivated(pid)
                    onMenuRequested: (pid, label, sx, sy) => sidebar.openProjectMenu(pid, label, sx, sy)
                }
            }
        }
    }
}
