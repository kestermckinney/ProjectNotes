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
    objectName: "projectSidebar"
    activeFocusOnTab: true
    color: Theme.sidebar
    property string keyboardKey: ""

    function navigationRows() {
        var rows = []
        for (var i = 0; i < folderColumn.children.length; ++i) {
            var child = folderColumn.children[i]
            if (typeof child.navigationRows === "function") rows = rows.concat(child.navigationRows())
        }
        return rows
    }

    function revealRow(row) {
        // Folder/model changes can rebuild rows before the next scene polish.
        // Lay out both levels before computing the row's viewport position.
        for (var i = 0; i < folderColumn.children.length; ++i)
            if (typeof folderColumn.children[i].navigationRows === "function")
                folderColumn.children[i].forceLayout()
        folderColumn.forceLayout()
        var flick = folderScroll.contentItem
        var y = row.mapToItem(flick, 0, 0).y
        var next = flick.contentY
        if (y < 0) next += y
        else if (y + row.height > flick.height) next += y + row.height - flick.height
        var top = flick.originY || 0
        flick.contentY = Math.max(top, Math.min(Math.max(top, top + flick.contentHeight - flick.height), next))
    }

    function focusRow(row) {
        keyboardKey = row.navigationKey
        forceActiveFocus(Qt.OtherFocusReason)
        revealRow(row)
    }

    function moveRow(direction) {
        var rows = navigationRows()
        if (rows.length === 0) return
        var index = -1
        for (var i = 0; i < rows.length; ++i)
            if (rows[i].navigationKey === keyboardKey) { index = i; break }
        var next = index < 0 ? (direction > 0 ? 0 : rows.length - 1)
                            : Math.max(0, Math.min(rows.length - 1, index + direction))
        focusRow(rows[next])
    }

    function activateRow() {
        var rows = navigationRows()
        if (rows.length === 0) return
        var row = rows[0]
        for (var i = 0; i < rows.length; ++i)
            if (rows[i].navigationKey === keyboardKey) { row = rows[i]; break }
        keyboardKey = row.navigationKey
        row.openRecord()
    }

    // Left/Right on a folder header collapse/expand it.
    function expandRow(expandedNow) {
        var rows = navigationRows()
        for (var i = 0; i < rows.length; ++i)
            if (rows[i].navigationKey === keyboardKey && typeof rows[i].setExpanded === "function") {
                rows[i].setExpanded(expandedNow)
                return true
            }
        return false
    }

    // A folder header was activated: filter the projects list to that folder
    // ("" = Not Categorized), or show every folder for All Projects.
    function filterToFolder(folderId, isAll) {
        if (isAll)
            DesktopAppController.clearProjectFolderFilter()
        else
            DesktopAppController.filterProjectsByFolder(folderId)
        sidebar.folderFilterApplied()
    }

    Keys.priority: Keys.AfterItem
    Keys.onPressed: (event) => {
        if (event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)) return
        switch (event.key) {
        case Qt.Key_Up: moveRow(-1); break
        case Qt.Key_Down: moveRow(1); break
        case Qt.Key_Left:
        case Qt.Key_Right:
            if (!expandRow(event.key === Qt.Key_Right)) return
            break
        case Qt.Key_Return:
        case Qt.Key_Enter:
            if (!event.isAutoRepeat) activateRow()
            break
        default: return
        }
        event.accepted = true
    }

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
    // A folder header changed the projects list's folder filter; Main shows
    // the list if a project is open.
    signal folderFilterApplied()

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
        reportItems: [
            { label: qsTr("Meeting Notes…"), value: "meeting-notes-report" },
            { label: qsTr("Status…"), value: "status-report" },
            { label: qsTr("Tracker Items…"), value: "tracker-items-report" }
        ]
        canScheduleMeeting: true
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
        onReportRequested: (workflow) => {
            meetingNotesReportReview.workflow = workflow
            meetingNotesReportReview.open()
        }
        onScheduleMeetingRequested: {
            scheduleMeeting.projectId = sidebar._ctxId
            scheduleMeeting.open()
        }
    }

    MoveToFolderDialog { id: moveToFolderDialog }
    FolderEditDialog { id: folderEditDialog }

    // Right-click menu on a folder header. _folder is the FolderManager.folders
    // entry, or null for the Not Categorized / All Projects groups.
    Popup {
        id: folderMenu
        objectName: "folderHeaderMenu"
        property var _folder: null
        function openAt(folder, sx, sy) {
            _folder = folder
            var sw = width * Theme.uiScale
            var sh = height * Theme.uiScale
            x = Math.max(6, Math.min(sx, (parent ? parent.width  : sx + sw) - sw - 6))
            y = Math.max(6, Math.min(sy, (parent ? parent.height : sy + sh) - sh - 6))
            open()
        }
        parent: Overlay.overlay
        modal: true
        dim: false
        padding: 3
        width: Math.max(170, implicitContentWidth + leftPadding + rightPadding)
        scale: Theme.uiScale
        transformOrigin: Item.TopLeft
        background: Rectangle { radius: Theme.radius; color: Theme.surface; border.color: Theme.border }
        ClickShield { host: folderMenu }
        contentItem: ColumnLayout {
            spacing: 0
            MenuRow {
                visible: folderMenu._folder !== null
                icon: "edit"; label: qsTr("Edit Folder…")
                onActivated: { folderMenu.close(); folderEditDialog.openFor(folderMenu._folder) }
            }
            MenuRow {
                icon: "create_new_folder"; label: qsTr("New Folder…")
                onActivated: { folderMenu.close(); folderEditDialog.openNew() }
            }
        }
    }

    MeetingNotesReportReviewDialog {
        id: meetingNotesReportReview
        projectId: sidebar._ctxId
        controller: DesktopAppController.communicationsController
        recipientModel: DesktopAppController.recipientSelectionModel
        templateModel: DesktopAppController.templateEditorModel
    }

    ScheduleMeetingDialog { id: scheduleMeeting }

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
            id: folderScroll
            objectName: "projectFolderScroll"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Column {
                id: folderColumn
                width: sidebar.width
                spacing: 4
                topPadding: 2
                bottomPadding: 10
                leftPadding: 6
                rightPadding: 6

                Repeater {
                    model: FolderManager.folders
                    delegate: FolderGroup {
                        navigationOwner: sidebar
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
                        onFilterRequested: sidebar.filterToFolder(modelData.id, false)
                        onFolderMenuRequested: (sx, sy) => folderMenu.openAt(modelData, sx, sy)
                    }
                }

                FolderGroup {
                    navigationOwner: sidebar
                    width: parent.width - 12
                    folderId: "__uncategorized__"
                    folderName: qsTr("Not Categorized")
                    isAll: false
                    isUncategorized: true
                    expanded: !FolderManager.uncategorizedCollapsed
                    selectedProjectId: sidebar.selectedProjectId
                    dragLayer:         sidebar.dragLayer
                    onProjectActivated: (pid) => sidebar.projectActivated(pid)
                    onMenuRequested: (pid, label, sx, sy) => sidebar.openProjectMenu(pid, label, sx, sy)
                    onItemMoveRequested: (itemId, pid) => sidebar.itemMoveRequested(itemId, pid)
                    onToggled: (exp) => FolderManager.uncategorizedCollapsed = !exp
                    onFilterRequested: sidebar.filterToFolder("", false)
                    onFolderMenuRequested: (sx, sy) => folderMenu.openAt(null, sx, sy)
                }

                FolderGroup {
                    navigationOwner: sidebar
                    width: parent.width - 12
                    folderName: qsTr("All Projects")
                    isAll: true
                    expanded: !FolderManager.allProjectsCollapsed
                    selectedProjectId: sidebar.selectedProjectId
                    dragLayer:         sidebar.dragLayer
                    onProjectActivated: (pid) => sidebar.projectActivated(pid)
                    onMenuRequested: (pid, label, sx, sy) => sidebar.openProjectMenu(pid, label, sx, sy)
                    onItemMoveRequested: (itemId, pid) => sidebar.itemMoveRequested(itemId, pid)
                    onToggled: (exp) => FolderManager.allProjectsCollapsed = !exp
                    onFilterRequested: sidebar.filterToFolder("", true)
                    onFolderMenuRequested: (sx, sy) => folderMenu.openAt(null, sx, sy)
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
