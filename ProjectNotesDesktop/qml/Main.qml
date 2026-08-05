// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import ProjectNotesDesktop

ApplicationWindow {
    id: root
    // Shown explicitly from main.cpp after the Windows taskbar identity
    // (AppUserModelID/RelaunchIconResource) is stamped onto the native window —
    // that must happen before the window is first mapped, or the taskbar button
    // is created without it and falls back to a generic icon.
    visible: false
    width: 1200
    height: 800
    minimumWidth: 900
    minimumHeight: 560
    title: qsTr("Project Notes")
    color: Theme.bg

    property string currentSection: "projects"
    property string selectedProjectId: ""

    // Breadcrumb subtitle for the current detail page (project / note).
    property string crumbSub: ""

    // Update-download progress for the themed downloader dialog (-1 = unknown).
    property int _dlPercent: 0

    readonly property var sectionMeta: ({
        "projects": { icon: "description", title: "Projects", newLabel: "New Project", search: true,  add: true  },
        "items":    { icon: "task_alt",    title: "Items",    newLabel: "",            search: true,  add: false },
        "people":   { icon: "group",       title: "People",   newLabel: "New Person",  search: true,  add: true  },
        "clients":  { icon: "apartment",   title: "Clients",  newLabel: "New Client",  search: true,  add: true  },
        "search":   { icon: "search",      title: "Search",   newLabel: "",            search: false, add: false },
        "settings": { icon: "settings",    title: "Settings", newLabel: "",            search: false, add: false },
        "help":     { icon: "menu_book",   title: "User Guide", newLabel: "",           search: false, add: false }
    })
    readonly property var meta: sectionMeta[currentSection]

    Component.onCompleted: {
        if (DesktopAppController.openOrCreateDatabase())
            FolderManager.reload()
        // Quiet launch-time update check (mirrors the Widgets app): only prompts
        // if a newer release is available. Deferred so the UI is up first.
        Qt.callLater(DesktopAppController.checkForUpdatesSilent)
    }

    Connections {
        target: DesktopAppController
        function onErrorOccurred(title, message) {
            errorDialog.title = title
            errorLabel.text = message
            errorDialog.open()
        }
        function onInfoOccurred(title, message) {
            infoDialog.title = title
            infoLabel.text = message
            infoDialog.open()
        }
        function onUpToDate(version) {
            infoDialog.title = qsTr("Check for Updates")
            infoLabel.text = qsTr("You're up to date.\n\nProject Notes %1 is the latest version.").arg(version)
            infoDialog.open()
        }
        function onUpdateCheckFailed(err) {
            infoDialog.title = qsTr("Check for Updates")
            infoLabel.text = qsTr("Could not check for updates:\n\n%1").arg(err)
            infoDialog.open()
        }
        function onUpdateAvailable(version, notes) {
            updateLabel.text = qsTr("Project Notes %1 is available. You are running %2.\n\n"
                                    + "Install it now? The app will download the update, install it, "
                                    + "and restart automatically.")
                               .arg(version).arg(DesktopAppController.appVersion())
            updateDialog.open()
        }
        function onUpdateDownloadStarted() {
            root._dlPercent = -1
            downloadDialog.open()
        }
        function onUpdateDownloadProgress(percent) {
            root._dlPercent = percent
        }
        function onUpdateInstallFailed(err) {
            downloadDialog.close()
            infoDialog.title = qsTr("Software Update")
            infoLabel.text = qsTr("The update could not be installed:\n\n%1").arg(err)
            infoDialog.open()
        }
        function onQuitForUpdate() {
            // The installer/relaunch helper is running; exit so it can replace us.
            downloadDialog.close()
            Qt.quit()
        }
    }

    // ── Navigation ────────────────────────────────────────────────────────────
    function _saveCurrent() {
        var it = contentStack.currentItem
        if (it && typeof it._saveNow === "function")
            it._saveNow()
    }

    // Section pages are created once and reused across navigation, so
    // revisiting a section doesn't rebuild its item tree (and keeps its scroll
    // position). StackView never destroys items it didn't instantiate, so
    // clear() just detaches them. Detail pages stay Component-pushed — they're
    // parameterized per record and cheap relative to the master lists.
    property var _pageCache: ({})
    function _sectionPage(section, comp) {
        if (!_pageCache[section])
            _pageCache[section] = comp.createObject(root)
        return _pageCache[section]
    }

    function selectSection(section) {
        if (section === root.currentSection && contentStack.depth <= 1)
            return
        _saveCurrent()
        root.currentSection = section
        root.crumbSub = ""
        contentStack.clear()
        switch (section) {
        case "projects": contentStack.push(_sectionPage("projects", projectsComponent)); break
        case "people":   contentStack.push(_sectionPage("people", peopleComponent)); break
        case "clients":  contentStack.push(_sectionPage("clients", clientsComponent)); break
        case "items":    contentStack.push(_sectionPage("items", itemsComponent)); break
        case "search":   contentStack.push(_sectionPage("search", searchComponent)); break
        case "settings": contentStack.push(_sectionPage("settings", settingsComponent)); break
        case "help":     contentStack.push(helpComponent, { topic: root._pendingHelpTopic }); break
        default:         contentStack.push(stubComponent); break
        }
    }

    // Open the embedded User Guide on a topic. Called by the menu + F1 with the
    // context topic for the current section; re-drives the page in place if help
    // is already the active section.
    property string _pendingHelpTopic: "index.md"
    function openHelp(topic) {
        var t = (topic && topic.length) ? topic : "index.md"
        if (root.currentSection === "help" && contentStack.currentItem
                && typeof contentStack.currentItem.openHelp === "function") {
            contentStack.currentItem.openHelp(t)
            return
        }
        root._pendingHelpTopic = t
        root.selectSection("help")
    }

    // Navigate to a search result based on its datatype.
    function openSearchResult(dataType, dataId, fkId) {
        switch (dataType) {
        case "Project":
            openProject(dataId); break
        case "People":
            openPerson(DesktopAppController.peopleRowForId(dataId), dataId); break
        case "Client":
            openClient(DesktopAppController.clientRowForId(dataId), dataId); break
        case "Item Tracker":
            openItem(dataId); break
        case "Tracker Update":
            openItem(fkId !== "" ? fkId : dataId); break
        default:
            // Notes / attendees / locations / team / status → open the parent project.
            if (fkId !== "") openProject(fkId)
            break
        }
    }

    function openProject(projectId) {
        if (projectId === "") return
        _saveCurrent()
        root.selectedProjectId = projectId
        var r = DesktopAppController.projectRowForId(projectId)
        if (r < 0) return
        root.crumbSub = DesktopAppController.projectNumberForId(projectId)
                        + " " + DesktopAppController.projectNameForId(projectId)
        contentStack.push(projectDetailComponent, { projectRow: r, projectId: projectId })
    }

    function openNote(noteRow, noteId, projectId) {
        _saveCurrent()
        root.crumbSub = qsTr("Note")
        contentStack.push(noteDetailComponent,
                          { noteRow: noteRow, noteId: noteId, projectId: projectId })
    }

    function openPerson(row, personId) {
        _saveCurrent()
        root.crumbSub = DesktopAppController.peopleNameForId(personId)
        contentStack.push(personDetailComponent, { personRow: row, personId: personId })
    }

    function openClient(row, clientId) {
        _saveCurrent()
        root.crumbSub = DesktopAppController.clientNameForId(clientId)
        contentStack.push(clientDetailComponent, { clientRow: row, clientId: clientId })
    }

    function openItem(itemId) {
        if (itemId === "") return
        _saveCurrent()
        root.crumbSub = qsTr("Item")
        contentStack.push(itemDetailComponent, { itemId: itemId })
    }

    // Move a tracker item to a different project via the sidebar drop handler
    // (drag/drop has no "pick a meeting" step, so any linked meeting is simply
    // cleared — meetings are project-scoped and can't follow the item). Only
    // opens the review dialog when the move actually needs attention (renumber
    // / losing a linked meeting); a plain move (including any silent team
    // auto-add) just happens. The "Move To…" menu option uses MoveToDialog
    // instead, which lets the user pick the destination meeting explicitly.
    function requestTrackerItemMove(itemId, projectId) {
        var chk = DesktopAppController.checkTrackerItemMove(itemId, projectId)
        if (!chk.valid) return
        if (chk.willRenumber || chk.willClearMeeting)
            moveReviewDialog.openFor(itemId, projectId, chk)
        else
            DesktopAppController.moveTrackerItem(itemId, projectId, "")
    }

    // Create a new record in the current section (shared by the TopBar + menu).
    function addForCurrentSection() {
        if (root.currentSection === "projects") {
            var r = DesktopAppController.addProject()
            if (r >= 0) root.openProject(DesktopAppController.projectIdAtRow(r))
        } else if (root.currentSection === "people") {
            var pr = DesktopAppController.addPerson()
            if (pr >= 0) root.openPerson(pr, DesktopAppController.personIdAtRow(pr))
        } else if (root.currentSection === "clients") {
            var cr = DesktopAppController.addClient()
            if (cr >= 0) root.openClient(cr, DesktopAppController.clientIdAtProxyRow(cr))
        }
    }

    // Dispatch a hamburger-menu action.
    function handleMenuAction(action) {
        switch (action) {
        case "new":         addForCurrentSection(); break
        case "search":
        case "find":        selectSection("search"); break
        case "export":
            var it = contentStack.currentItem
            if (it && it.exportTable !== undefined && it.exportTable !== "" && it.exportId !== "")
                exportRecord(it.exportTable, it.exportId)
            break
        case "import":      importDialog.open(); break
        case "preferences":
        case "about":       selectSection("settings"); break
        case "sync":        DesktopAppController.syncNow(); break
        case "sync_all":    DesktopAppController.syncAll(); break
        case "filter":      filterDialog.openFor(root.currentSection); break
        case "logs":        logViewer.openViewer(); break
        case "help":        root.openHelp(root._helpTopicForSection(root.currentSection)); break
        case "check_updates": DesktopAppController.checkForUpdates(); break
        case "support_logs":  DesktopAppController.sendLogsToSupport(); break
        case "exit":        Qt.quit(); break
        }
    }

    // ── XML export ────────────────────────────────────────────────────────────
    property string _exportTable: ""
    property string _exportId: ""
    function exportRecord(table, id) {
        if (!table || !id) return
        _saveCurrent()
        root._exportTable = table
        root._exportId = id
        exportDialog.open()
    }

    // Pop the current detail page and recompute the breadcrumb for the page we
    // return to. Shared by goBack() and deleteCurrent().
    function _popAndRecrumb() {
        if (contentStack.depth > 1) {
            contentStack.pop()
            var it = contentStack.currentItem
            root.crumbSub = (it && it.projectId !== undefined && it.noteRow === undefined)
                ? DesktopAppController.projectNumberForId(it.projectId) + " "
                  + DesktopAppController.projectNameForId(it.projectId)
                : ""
        }
    }

    function goBack() {
        _saveCurrent()
        _popAndRecrumb()
    }

    // Delete the record shown on the current detail page, then return to its list.
    // The page's _deleteRecord() removes the row from the underlying model(s); on
    // success we pop back WITHOUT saving (the record no longer exists).
    function deleteCurrent() {
        var it = contentStack.currentItem
        if (!it || typeof it._deleteRecord !== "function") return
        if (it._deleteRecord())
            _popAndRecrumb()
    }

    function confirmDelete() {
        var it = contentStack.currentItem
        if (!it || it.canDelete !== true) return
        confirmLabel.text = root.crumbSub
        confirmDeleteDialog.open()
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    // Zoomable logical viewport. Its logical size shrinks as uiScale grows, then
    // the scale transform magnifies it back to fill the window — so the whole UI
    // reflows at the new size (like browser Ctrl +/−) and stays vector-crisp
    // (Qt Quick re-rasterises distance-field glyphs at the composited scale).
    Item {
        id: zoomLayer
        transformOrigin: Item.TopLeft
        scale: Theme.uiScale
        width:  (root.contentItem ? root.contentItem.width  : root.width)  / Theme.uiScale
        height: (root.contentItem ? root.contentItem.height : root.height) / Theme.uiScale

    RowLayout {
        anchors.fill: parent
        spacing: 0

        IconRail {
            Layout.fillHeight: true
            currentSection: root.currentSection
            // Same "is there an open record" signal the TopBar's Export XML
            // button uses — lets the app menu offer the Widgets Plugins-menu-bar
            // parity group (dataexport matches the open record's table) below
            // the always-present global (dataexport-less) plugin entries.
            pluginMenuTable: contentStack.currentItem
                             && contentStack.currentItem.exportTable !== undefined
                             ? contentStack.currentItem.exportTable : ""
            pluginMenuRecordId: contentStack.currentItem
                                && contentStack.currentItem.exportId !== undefined
                                ? contentStack.currentItem.exportId : ""
            onSectionActivated: (s) => root.selectSection(s)
            onMenuAction: (a) => root.handleMenuAction(a)
        }

        ProjectSidebar {
            id: sidebar
            Layout.fillHeight: true
            visible: root.currentSection === "projects"
            dragLayer: dragOverlay
            selectedProjectId: root.selectedProjectId
            onProjectActivated: (pid) => root.openProject(pid)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
            onItemMoveRequested: (itemId, pid) => root.requestTrackerItemMove(itemId, pid)
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            TopBar {
                Layout.fillWidth: true
                crumbIcon: root.meta.icon
                crumbTitle: root.meta.title
                crumbSub: root.crumbSub
                newLabel: root.meta.newLabel
                showNew: root.meta.add && contentStack.depth <= 1
                showSearch: root.meta.search && contentStack.depth <= 1
                showFilter: root.meta.search && contentStack.depth <= 1
                showBack: contentStack.depth > 1
                showExport: contentStack.currentItem
                            && contentStack.currentItem.exportTable !== undefined
                            && contentStack.currentItem.exportTable !== ""
                            && contentStack.currentItem.exportId !== ""
                showDelete: contentStack.currentItem
                            && contentStack.currentItem.canDelete === true
                onBackClicked: root.goBack()
                onDeleteClicked: root.confirmDelete()
                onExportClicked: root.exportRecord(contentStack.currentItem.exportTable,
                                                   contentStack.currentItem.exportId)
                onAddClicked: root.addForCurrentSection()
                onFilterClicked: filterDialog.openFor(root.currentSection)
                onSearchEdited: (t) => {
                    switch (root.currentSection) {
                    case "projects": DesktopAppController.setQuickSearch(DesktopAppController.projectsListModel, t); break
                    case "people":   DesktopAppController.setQuickSearch(DesktopAppController.peopleModel, t); break
                    case "clients":  DesktopAppController.setQuickSearch(DesktopAppController.clientsModel, t); break
                    case "items":    DesktopAppController.setQuickSearch(DesktopAppController.allItemsModel, t); break
                    }
                }
            }

            StackView {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                // Start on the cached projects page (an initialItem component
                // would be StackView-owned and destroyed on the first clear()).
                Component.onCompleted: push(root._sectionPage("projects", projectsComponent))
            }
        }
    }

    // Overlay dragged sidebar rows reparent onto (always on top).
    Item {
        id: dragOverlay
        anchors.fill: parent
        z: 9999
    }
    } // zoomLayer

    // Ctrl + mouse-wheel zoom. Captured in a transparent full-window layer above
    // the content so ScrollViews don't eat it; a plain Item with only a
    // WheelHandler stays transparent to every other event (clicks/hover fall
    // through), and it sits below the window overlay so popups still render on top.
    Item {
        anchors.fill: parent
        z: 10000
        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            onWheel: (ev) => {
                if (ev.angleDelta.y > 0) Theme.zoomIn()
                else if (ev.angleDelta.y < 0) Theme.zoomOut()
            }
        }
    }

    // Keyboard zoom: Ctrl +/= in, Ctrl - out, Ctrl 0 reset (browser-style).
    Shortcut { sequences: ["Ctrl+=", "Ctrl++"]; onActivated: Theme.zoomIn() }
    Shortcut { sequence: "Ctrl+-";              onActivated: Theme.zoomOut() }
    Shortcut { sequence: "Ctrl+0";              onActivated: Theme.zoomReset() }

    // ── Content components ────────────────────────────────────────────────────
    Component {
        id: projectsComponent
        ProjectsListPage {
            dragLayer: dragOverlay
            selectedProjectId: root.selectedProjectId
            onProjectActivated: (pid) => root.openProject(pid)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
        }
    }
    Component {
        id: projectDetailComponent
        ProjectDetailPage {
            dragLayer: dragOverlay
            onNoteActivated: (noteRow, noteId) => root.openNote(noteRow, noteId, projectId)
            onItemActivated: (itemId) => root.openItem(itemId)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onMoveToRequested: (id) => moveToDialog.openFor(id)
            onDeleteRequested: root.confirmDelete()
            onNewRequested: root.addForCurrentSection()
            onFilterRequested: filterDialog.openFor(root.currentSection)
            onSubFilterRequested: (section) => filterDialog.openFor(section)
        }
    }
    Component {
        id: noteDetailComponent
        ProjectNoteDetailPage {
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onMoveToRequested: (id) => moveToDialog.openFor(id)
        }
    }
    Component {
        id: peopleComponent
        PeoplePage {
            onPersonActivated: (row, personId) => root.openPerson(row, personId)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
        }
    }
    Component {
        id: personDetailComponent
        PersonDetailPage {}
    }
    Component {
        id: clientsComponent
        ClientsPage {
            onClientActivated: (row, clientId) => root.openClient(row, clientId)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
        }
    }
    Component {
        id: clientDetailComponent
        ClientDetailPage {}
    }
    Component {
        id: itemsComponent
        ItemsPage {
            onItemActivated: (itemId) => root.openItem(itemId)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
            onMoveToRequested: (id) => moveToDialog.openFor(id)
        }
    }
    Component {
        id: itemDetailComponent
        ItemDetailPage {
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onMoveToRequested: (id) => moveToDialog.openFor(id)
        }
    }
    Component {
        id: searchComponent
        SearchPage {
            onResultActivated: (dataType, dataId, fkId) => root.openSearchResult(dataType, dataId, fkId)
            onExportRequested: (table, id) => root.exportRecord(table, id)
        }
    }
    Component {
        id: settingsComponent
        SettingsPage {}
    }
    Component {
        id: helpComponent
        HelpPage { topic: root._pendingHelpTopic }
    }
    Component {
        id: stubComponent
        StubPage {
            pageTitle: root.meta.title
            pageIcon: root.meta.icon
        }
    }

    Dialog {
        id: errorDialog
        anchors.centerIn: parent
        width: 380
        scale: Theme.uiScale   // match the zoomed workspace (centered origin)
        modal: true
        padding: 0
        closePolicy: Popup.CloseOnEscape
        title: "Error"

        // Fully themed chrome so the dialog matches the app instead of the
        // platform default (which renders a black title bar / background).
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
        header: null
        footer: null

        contentItem: ColumnLayout {
            spacing: 0

            // Header
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 14
                spacing: 8
                MaterialIcon { name: "error"; size: 20; color: Theme.red }
                Text {
                    text: errorDialog.title
                    color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold
                    Layout.fillWidth: true
                }
                MaterialIcon {
                    name: "close"; size: 20; color: Theme.text3
                    // Exclusive grab: see the matching infoDialog close button below.
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: errorDialog.close() }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

            // Message + button
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 18
                spacing: 16
                Text {
                    id: errorLabel
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: Theme.text
                    font.pixelSize: 13
                }
                Item {
                    Layout.fillWidth: true
                    implicitHeight: 30
                    Rectangle {
                        anchors.right: parent.right
                        implicitHeight: 30
                        implicitWidth: okText.implicitWidth + 26
                        radius: Theme.radiusSm
                        color: okHover.hovered ? Theme.accentStrong : Theme.accent
                        Text {
                            id: okText
                            anchors.centerIn: parent
                            text: qsTr("OK")
                            color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold
                        }
                        HoverHandler { id: okHover }
                        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: errorDialog.close() }
                    }
                }
            }
        }
    }

    // Informational dialog (Send Logs result, up-to-date, update-check errors).
    Dialog {
        id: infoDialog
        anchors.centerIn: parent
        width: 400
        scale: Theme.uiScale
        modal: true
        padding: 0
        closePolicy: Popup.CloseOnEscape
        title: qsTr("Information")

        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
        header: null
        footer: null

        contentItem: ColumnLayout {
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 14
                spacing: 8
                MaterialIcon { name: "info"; size: 20; color: Theme.accent }
                Text {
                    text: infoDialog.title
                    color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold
                    Layout.fillWidth: true
                }
                MaterialIcon {
                    name: "close"; size: 20; color: Theme.text3
                    // Exclusive grab: a plain TapHandler only takes a passive grab, so
                    // without this the same tap can also fall through to whatever's
                    // behind the modal dialog (see the KebabButton fix for the same bug).
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: infoDialog.close() }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 18
                spacing: 16
                Text {
                    id: infoLabel
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: Theme.text
                    font.pixelSize: 13
                }
                Item {
                    Layout.fillWidth: true
                    implicitHeight: 30
                    Rectangle {
                        anchors.right: parent.right
                        implicitHeight: 30
                        implicitWidth: infoOkText.implicitWidth + 26
                        radius: Theme.radiusSm
                        color: infoOkHover.hovered ? Theme.accentStrong : Theme.accent
                        Text {
                            id: infoOkText
                            anchors.centerIn: parent
                            text: qsTr("OK")
                            color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold
                        }
                        HoverHandler { id: infoOkHover }
                        // Exclusive grab so this tap doesn't also fall through to
                        // whatever's behind the dialog (e.g. a project row).
                        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: infoDialog.close() }
                    }
                }
            }
        }
    }

    // "Update available" dialog — offers to open the download page.
    Dialog {
        id: updateDialog
        anchors.centerIn: parent
        width: 400
        scale: Theme.uiScale
        modal: true
        padding: 0
        closePolicy: Popup.CloseOnEscape
        title: qsTr("Update Available")

        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
        header: null
        footer: null

        contentItem: ColumnLayout {
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 14
                spacing: 8
                MaterialIcon { name: "system_update_alt"; size: 20; color: Theme.accent }
                Text {
                    text: updateDialog.title
                    color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold
                    Layout.fillWidth: true
                }
                MaterialIcon {
                    name: "close"; size: 20; color: Theme.text3
                    // Exclusive grab: see the matching infoDialog close button above.
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: updateDialog.close() }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 18
                spacing: 16
                Text {
                    id: updateLabel
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: Theme.text
                    font.pixelSize: 13
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    spacing: 10
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        implicitHeight: 30
                        implicitWidth: laterText.implicitWidth + 26
                        radius: Theme.radiusSm
                        color: laterHover.hovered ? Theme.surface2 : Theme.surface
                        border.color: Theme.border
                        Text {
                            id: laterText
                            anchors.centerIn: parent
                            text: qsTr("Later")
                            color: Theme.text; font.pixelSize: 12; font.weight: Font.DemiBold
                        }
                        HoverHandler { id: laterHover }
                        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: updateDialog.close() }
                    }
                    Rectangle {
                        implicitHeight: 30
                        implicitWidth: dlText.implicitWidth + 26
                        radius: Theme.radiusSm
                        color: dlHover.hovered ? Theme.accentStrong : Theme.accent
                        Text {
                            id: dlText
                            anchors.centerIn: parent
                            text: qsTr("Install & Restart")
                            color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold
                        }
                        HoverHandler { id: dlHover }
                        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: { updateDialog.close(); DesktopAppController.installUpdate() } }
                    }
                }
            }
        }
    }

    // Themed update-download progress (replaces UpdateManager's native dialog).
    Dialog {
        id: downloadDialog
        anchors.centerIn: parent
        width: 400
        scale: Theme.uiScale
        modal: true
        padding: 0
        closePolicy: Popup.NoAutoClose   // dismissed only via Cancel / completion
        title: qsTr("Downloading Update")

        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
        header: null
        footer: null

        contentItem: ColumnLayout {
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 14
                spacing: 8
                MaterialIcon { name: "system_update_alt"; size: 20; color: Theme.accent }
                Text {
                    text: downloadDialog.title
                    color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold
                    Layout.fillWidth: true
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 18
                spacing: 14
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: Theme.text
                    font.pixelSize: 13
                    text: root._dlPercent >= 0
                          ? qsTr("Downloading the update… %1%").arg(root._dlPercent)
                          : qsTr("Downloading the update…")
                }
                // Progress track + fill (indeterminate sweep when percent < 0).
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 8
                    radius: 4
                    color: Theme.surface
                    border.color: Theme.border
                    clip: true
                    Rectangle {
                        id: dlFill
                        height: parent.height
                        radius: 4
                        color: Theme.accent
                        width: root._dlPercent >= 0
                               ? parent.width * Math.max(0.02, root._dlPercent / 100)
                               : parent.width * 0.3
                        // Indeterminate: slide the partial bar back and forth.
                        SequentialAnimation on x {
                            running: downloadDialog.visible && root._dlPercent < 0
                            loops: Animation.Infinite
                            NumberAnimation { from: 0; to: dlFill.parent.width * 0.7; duration: 900; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: dlFill.parent.width * 0.7; to: 0; duration: 900; easing.type: Easing.InOutQuad }
                        }
                        // Determinate: pin to the left.
                        Binding { target: dlFill; property: "x"; value: 0; when: root._dlPercent >= 0 }
                    }
                }
                Item {
                    Layout.fillWidth: true
                    implicitHeight: 30
                    Rectangle {
                        anchors.right: parent.right
                        implicitHeight: 30
                        implicitWidth: dlCancelText.implicitWidth + 26
                        radius: Theme.radiusSm
                        color: dlCancelHover.hovered ? Theme.surface2 : Theme.surface
                        border.color: Theme.border
                        Text {
                            id: dlCancelText
                            anchors.centerIn: parent
                            text: qsTr("Cancel")
                            color: Theme.text; font.pixelSize: 12; font.weight: Font.DemiBold
                        }
                        HoverHandler { id: dlCancelHover }
                        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: { downloadDialog.close(); DesktopAppController.cancelUpdateDownload() } }
                    }
                }
            }
        }
    }

    // Delete confirmation for the current detail record (project / item / note).
    Dialog {
        id: confirmDeleteDialog
        anchors.centerIn: parent
        width: 380
        scale: Theme.uiScale   // match the zoomed workspace (centered origin)
        modal: true
        padding: 0
        closePolicy: Popup.CloseOnEscape
        title: qsTr("Delete Record")

        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
        header: null
        footer: null

        contentItem: ColumnLayout {
            spacing: 0

            // Header
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 14
                spacing: 8
                MaterialIcon { name: "delete"; size: 20; color: Theme.red }
                Text {
                    text: confirmDeleteDialog.title
                    color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold
                    Layout.fillWidth: true
                }
                MaterialIcon {
                    name: "close"; size: 20; color: Theme.text3
                    // Exclusive grab: see the matching infoDialog close button above.
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: confirmDeleteDialog.close() }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

            // Message + buttons
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 18
                spacing: 16
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: Theme.text
                    font.pixelSize: 13
                    text: {
                        var l = confirmLabel.text.trim()
                        return l === ""
                            ? qsTr("Delete this record? This cannot be undone.")
                            : qsTr("Delete “%1”? This cannot be undone.").arg(l)
                    }
                }
                // Hidden holder for the record label (set by confirmDelete()).
                Text { id: confirmLabel; visible: false }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    spacing: 10
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        implicitHeight: 30
                        implicitWidth: cancelText.implicitWidth + 26
                        radius: Theme.radiusSm
                        color: cancelHover.hovered ? Theme.surface2 : Theme.surface
                        border.color: Theme.border
                        Text {
                            id: cancelText
                            anchors.centerIn: parent
                            text: qsTr("Cancel")
                            color: Theme.text; font.pixelSize: 12; font.weight: Font.DemiBold
                        }
                        HoverHandler { id: cancelHover }
                        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: confirmDeleteDialog.close() }
                    }
                    Rectangle {
                        implicitHeight: 30
                        implicitWidth: delConfirmText.implicitWidth + 26
                        radius: Theme.radiusSm
                        color: delConfirmHover.hovered ? Theme.red : Theme.redSoft
                        border.color: Theme.red
                        Text {
                            id: delConfirmText
                            anchors.centerIn: parent
                            text: qsTr("Delete")
                            color: delConfirmHover.hovered ? "#ffffff" : Theme.red
                            font.pixelSize: 12; font.weight: Font.DemiBold
                        }
                        HoverHandler { id: delConfirmHover }
                        // Exclusive grab matters most here: without it, this tap could
                        // also fall through and activate whatever record row happens to
                        // sit underneath the dialog, deleting the wrong thing.
                        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: { confirmDeleteDialog.close(); root.deleteCurrent() } }
                    }
                }
            }
        }
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Export XML to file")
        fileMode: FileDialog.SaveFile
        nameFilters: [ qsTr("XML files (*.xml)") ]
        defaultSuffix: "xml"
        onAccepted: DesktopAppController.exportRecordXml(root._exportTable, root._exportId, selectedFile)
    }

    FileDialog {
        id: importDialog
        title: qsTr("Import XML from file")
        fileMode: FileDialog.OpenFile
        nameFilters: [ qsTr("XML files (*.xml)") ]
        onAccepted: DesktopAppController.importXmlFile(selectedFile)
    }

    // Filter editor (opened from the TopBar Filter button and the hamburger menu).
    FilterDialog { id: filterDialog }

    // Tracker item move: MoveReviewDialog confirms the sidebar drag/drop path
    // (shown only when it needs attention — renumber or losing a linked
    // meeting, via root.requestTrackerItemMove()); MoveToDialog is every
    // "Move To…" menu entry's project + meeting picker and performs the move
    // itself once the user presses Move.
    MoveReviewDialog { id: moveReviewDialog }
    MoveToDialog { id: moveToDialog }

    // Log Viewer — a separate, movable, non-modal window opened from the menu.
    LogViewerWindow { id: logViewer }

    // Map the active shell section to the help topic that documents it, so F1 /
    // Help ▸ User Guide opens context-sensitively. Falls back to the home page.
    function _helpTopicForSection(section) {
        switch (section) {
        case "projects": return "InterfaceOverview/ProjectListPage.md"
        case "people":   return "InterfaceOverview/PeopleListPage.md"
        case "clients":  return "InterfaceOverview/ClientListPage.md"
        case "items":    return "InterfaceOverview/ItemTrackerPage.md"
        case "search":   return "InterfaceOverview/SearchPage.md"
        case "settings": return "InterfaceOverview/Preferences.md"
        default:         return "index.md"
        }
    }

    // F1 opens the User Guide on the current screen's topic.
    Shortcut {
        sequences: [ StandardKey.HelpContents ]
        onActivated: root.openHelp(root._helpTopicForSection(root.currentSection))
    }
}
