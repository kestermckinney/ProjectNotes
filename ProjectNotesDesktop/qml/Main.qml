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

    // Last known non-maximized geometry. x/y/width/height report the maximized
    // rect while visibility is Maximized, so these are tracked separately —
    // updated only while windowed — and are what gets persisted, so
    // un-maximizing later restores the actual pre-maximize size instead of
    // snapping to whatever the maximized rect happened to be.
    property int _normalX: 0
    property int _normalY: 0
    property int _normalWidth: 0
    property int _normalHeight: 0

    // Visibility to restore when leaving fullscreen (toggled from the app
    // menu's zoom control) — so un-fullscreening lands back on Maximized if
    // that's what it was before, rather than always dropping to Windowed.
    property int _preFullScreenVisibility: Window.Windowed
    onXChanged: if (visibility === Window.Windowed) _normalX = x
    onYChanged: if (visibility === Window.Windowed) _normalY = y
    onWidthChanged: if (visibility === Window.Windowed) _normalWidth = width
    onHeightChanged: if (visibility === Window.Windowed) _normalHeight = height

    // Restore the previous window geometry (falls back to the width/height/
    // minimum* above when nothing has been saved yet, e.g. first run). Applied
    // before main.cpp's window->show() — Component.onCompleted runs synchronously
    // during engine.load(), which returns before show() is called — so the window
    // maps directly at its saved size/position instead of flashing the default
    // and then jumping.
    function _restoreGeometry() {
        var g = DesktopAppController.windowGeometry()
        if (!g.valid)
            return
        // x/y come back as -1 when the saved position no longer lands on any
        // connected screen (e.g. a monitor was unplugged) — leave Qt's default
        // placement in that case and only restore the size.
        if (g.x !== -1 && g.y !== -1) {
            root.x = g.x
            root.y = g.y
        }
        root.width = g.width
        root.height = g.height
        if (g.maximized)
            root.visibility = Window.Maximized
    }

    // Persist the current geometry so the window reopens the same size/position
    // next launch.
    onClosing: {
        DesktopAppController.saveWindowGeometry(
            root._normalX, root._normalY, root._normalWidth, root._normalHeight,
            root.visibility === Window.Maximized)
    }

    Component.onCompleted: {
        _restoreGeometry()
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
        // Verdict on the cloud sync settings the user just edited (see
        // _gateOnSyncCheck). A late answer for a check the user already skipped
        // has no navigation parked against it and is theirs to ignore.
        function onSyncSettingsVerified(status, message) {
            if (root._pendingNavigation === null)
                return
            if (message === "") {
                // Nothing to report — carry on to wherever they were headed.
                root._resumeNavigation()
                syncCheckDialog.close()
                return
            }
            syncCheckDialog.message = message
            syncCheckDialog.status = status
        }
    }

    // ── Navigation ────────────────────────────────────────────────────────────
    function _saveCurrent() {
        var it = contentStack.currentItem
        if (!it) return
        if (typeof it._saveNow === "function")
            it._saveNow()
        // A record the "New" button only staged (see DesktopAppController::
        // addProject) still isn't in the database if _saveNow() had nothing to
        // save — no name typed, or a name that clashed. Drop the staged row on
        // the way out so it can't linger in the list as a blank, dead card.
        if (it.isNewRecord === true && typeof it._discardNew === "function")
            it._discardNew()
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

    // The quick-search-able models, keyed by rail section — shared by the top
    // bar's onSearchEdited (write) and selectSection's resync (read) so the
    // two can't drift apart. Quick search only applies to the 4 top-level
    // rail sections (not the 5 project sub-tabs); model resolution itself
    // delegates to DesktopAppController.modelForSection(), the one canonical
    // section→model map also used by Filter and Sort.
    function _quickSearchModelForSection(section) {
        switch (section) {
        case "projects": case "people": case "clients": case "items":
            return DesktopAppController.modelForSection(section)
        default:
            return null
        }
    }

    // Opens SortMenu for whatever section is active, anchored beside the top
    // bar's Sort button — used by the app-menu "Sort…" action and its keyboard
    // shortcut, neither of which has a clicked row/chip of their own to
    // position from (contrast the per-row/per-chip Sort triggers, which pass
    // their own coordinates directly).
    function _openSortMenuForCurrentSection() {
        if (!topBar.sortButtonItem) return
        var p = topBar.sortButtonItem.mapToItem(Overlay.overlay, 0, topBar.sortButtonItem.height)
        sortMenu.openFor(root.currentSection, p.x, p.y + 4)
    }

    // ── Leaving Settings: check the cloud sync fields first ───────────────────
    // The sync credentials and the encryption phrase are write-through — a
    // mistyped password or a phrase that doesn't match the account is simply
    // saved, and only shows up much later as a sync that never completes. So
    // when the user walks away from Settings having edited any of them, verify
    // them against the host and say so while they are still on the page that
    // can fix it. Navigation waiting on that answer is parked here.
    property var _pendingNavigation: null

    // Returns true when `next` has been parked pending the check — the caller
    // must then do nothing; _resumeNavigation() runs it once the user is done
    // with the dialog.
    function _gateOnSyncCheck(next) {
        if (root.currentSection !== "settings" || !DesktopAppController.syncSettingsUnverified)
            return false
        root._pendingNavigation = next
        syncCheckDialog.status = ""
        DesktopAppController.verifySyncSettings()
        // With sync off or unconfigured there is nothing to contact the host
        // about: the verdict came back inside that call and already ran the
        // navigation, so don't flash a dialog for it.
        if (root._pendingNavigation !== null)
            syncCheckDialog.open()
        return true
    }

    function _resumeNavigation() {
        var next = root._pendingNavigation
        root._pendingNavigation = null
        if (next)
            next()
    }

    function selectSection(section) {
        if (section === root.currentSection && contentStack.depth <= 1)
            return
        _saveCurrent()
        if (_gateOnSyncCheck(function() { root._selectSectionNow(section) }))
            return
        _selectSectionNow(section)
    }

    function _selectSectionNow(section) {
        root.currentSection = section
        root.crumbSub = ""
        // The search field only tracks the field the user is looking at (see
        // TopBar's searchText doc comment) — resync it here to whichever quick
        // search is actually active for the section being switched to, rather
        // than leaving behind whatever text the previous section left typed in.
        var m = root._quickSearchModelForSection(section)
        topBar.searchText = m ? DesktopAppController.getQuickSearch(m) : ""
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

    // Open the detail page on a project the "New Project" button just staged. It
    // has no id yet — the row is written the moment the page has a project number
    // and name (ProjectDetailPage._saveNow), which is also when its child tabs
    // become usable, since every one of them is keyed by the project id.
    function openNewProject(projectRow) {
        _saveCurrent()
        root.crumbSub = qsTr("New Project")
        contentStack.push(projectDetailComponent,
                          { projectRow: projectRow, projectId: "", isNewRecord: true })
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
            if (r >= 0) root.openNewProject(r)
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
        case "preferences": selectSection("settings"); break
        case "about":       aboutDialog.open(); break
        case "sync":        DesktopAppController.syncNow(); break
        case "sync_all":    DesktopAppController.syncAll(); break
        case "filter":      filterDialog.openFor(root.currentSection); break
        case "sort":        root._openSortMenuForCurrentSection(); break
        case "logs":        logViewer.openViewer(); break
        case "toggle_fullscreen":
            if (root.visibility === Window.FullScreen) {
                root.visibility = root._preFullScreenVisibility
            } else {
                root._preFullScreenVisibility = root.visibility
                root.visibility = Window.FullScreen
            }
            break
        case "help":        root.openHelp(root._helpTopicForSection(root.currentSection)); break
        case "check_updates": DesktopAppController.checkForUpdates(); break
        case "support_logs":  DesktopAppController.sendLogsToSupport(); break
        // root.close() (not Qt.quit()) so this goes through the window's own
        // close path — onClosing is what persists window geometry, and only
        // fires for an actual close event, not a direct quit. quitOnLastWindowClosed
        // (Qt's default) then ends the app once the window is gone, same as
        // clicking its native close button.
        case "exit":        root.close(); break
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
            onSortRequested: (sx, sy) => sortMenu.openFor(root.currentSection, sx, sy)
            onItemMoveRequested: (itemId, pid) => root.requestTrackerItemMove(itemId, pid)
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            TopBar {
                id: topBar
                Layout.fillWidth: true
                crumbIcon: root.meta.icon
                crumbTitle: root.meta.title
                crumbSub: root.crumbSub
                newLabel: root.meta.newLabel
                showNew: root.meta.add && contentStack.depth <= 1
                showSearch: root.meta.search && contentStack.depth <= 1
                showFilter: root.meta.search && contentStack.depth <= 1
                // Reads filterRev first so this re-evaluates whenever any
                // filter (quick or manual) changes anywhere — see
                // DesktopAppController.filterRev/hasActiveColumnFilters().
                filterActive: {
                    DesktopAppController.filterRev
                    var m = DesktopAppController.modelForSection(root.currentSection)
                    return m ? DesktopAppController.hasActiveColumnFilters(m) : false
                }
                showSort: root.meta.search && contentStack.depth <= 1
                // Reads sortRev first for the same reason filterActive reads
                // filterRev — see DesktopAppController.sortRev/activeSort().
                sortActive: {
                    DesktopAppController.sortRev
                    var m = DesktopAppController.modelForSection(root.currentSection)
                    return m ? (DesktopAppController.activeSort(m).field || "") !== "" : false
                }
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
                onSortClicked: root._openSortMenuForCurrentSection()
                onSearchEdited: (t) => {
                    var m = root._quickSearchModelForSection(root.currentSection)
                    if (m) DesktopAppController.setQuickSearch(m, t)
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

    // ── App menu shortcuts ───────────────────────────────────────────────────
    // Portable sequences come from AppShortcuts.map — the same source AppMenu.qml
    // reads to display the key beside each row, so the two can't drift. "search"
    // and "find" share one key (both land on the same selectSection("search")
    // call below); only one Shortcut is wired for it — a second Shortcut on the
    // identical sequence would make both ambiguous and fire neither.
    Shortcut { sequence: AppShortcuts.map["new"];         onActivated: root.addForCurrentSection() }
    Shortcut { sequence: AppShortcuts.map["search"];      onActivated: root.selectSection("search") }
    Shortcut { sequence: AppShortcuts.map["preferences"]; onActivated: root.selectSection("settings") }
    // root.close(), not Qt.quit() — see the "exit" case in the app-menu handler above.
    Shortcut { sequence: AppShortcuts.map["exit"];        onActivated: root.close() }
    Shortcut {
        sequence: AppShortcuts.map["export"]
        onActivated: {
            var it = contentStack.currentItem
            if (it && it.exportTable !== undefined && it.exportTable !== "" && it.exportId !== "")
                root.exportRecord(it.exportTable, it.exportId)
        }
    }
    Shortcut { sequence: AppShortcuts.map["import"]; onActivated: importDialog.open() }
    Shortcut { sequence: AppShortcuts.map["filter"]; onActivated: filterDialog.openFor(root.currentSection) }
    Shortcut { sequence: AppShortcuts.map["sort"];   onActivated: root._openSortMenuForCurrentSection() }
    // Ctrl+L (⌘L on macOS) drops the cursor into the top bar's quick search
    // field, browser address-bar style. Harmless on the sections that don't show
    // the field — focusSearch() is a no-op there.
    Shortcut { sequence: AppShortcuts.map["focus_quick_search"]; onActivated: topBar.focusSearch() }
    // Always-on toggle (F11 win/linux, Control+Command+F mac — see AppShortcuts.fullscreen).
    Shortcut { sequence: AppShortcuts.map["toggle_fullscreen"]; onActivated: root.handleMenuAction("toggle_fullscreen") }
    // Esc only ever exits full screen (never enters it) — matches browser/OS
    // convention. Only enabled while actually full screen so it doesn't swallow
    // Esc from whatever a focused popup/field would otherwise use it for (those
    // still get first crack at the key; this only fires once nothing else claims
    // it — see Popup.CloseOnEscape and FindReplaceBar's own Keys.onEscapePressed).
    Shortcut {
        sequence: "Esc"
        enabled: root.visibility === Window.FullScreen
        onActivated: root.handleMenuAction("toggle_fullscreen")
    }

    // ── Content components ────────────────────────────────────────────────────
    Component {
        id: projectsComponent
        ProjectsListPage {
            dragLayer: dragOverlay
            selectedProjectId: root.selectedProjectId
            onProjectActivated: (pid) => root.openProject(pid)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
            onSortRequested: (sx, sy) => sortMenu.openFor(root.currentSection, sx, sy)
        }
    }
    Component {
        id: projectDetailComponent
        ProjectDetailPage {
            dragLayer: dragOverlay
            onNoteActivated: (noteRow, noteId) => root.openNote(noteRow, noteId, projectId)
            onItemActivated: (itemId) => root.openItem(itemId)
            onProjectActivated: (pid) => root.openProject(pid)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onMoveToRequested: (id) => moveToDialog.openFor(id)
            onDeleteRequested: root.confirmDelete()
            onNewRequested: root.addForCurrentSection()
            // A staged new project became a real record — it has a breadcrumb and
            // a sidebar selection now, like any project opened from the list.
            onRecordPersisted: (projectId) => {
                root.selectedProjectId = projectId
                root.crumbSub = DesktopAppController.projectNumberForId(projectId)
                                + " " + DesktopAppController.projectNameForId(projectId)
            }
            onFilterRequested: filterDialog.openFor(root.currentSection)
            onSubFilterRequested: (section) => filterDialog.openFor(section)
            onSortRequested: (sx, sy) => sortMenu.openFor(root.currentSection, sx, sy)
            onSubSortRequested: (section, sx, sy) => sortMenu.openFor(section, sx, sy)
            onGoToPersonRequested: (personId) => {
                var row = DesktopAppController.peopleRowForId(personId)
                if (row >= 0) {
                    root.selectSection("people")
                    root.openPerson(row, personId)
                }
            }
            onGoToClientRequested: (clientId) => {
                var row = DesktopAppController.clientRowForId(clientId)
                if (row >= 0) {
                    root.selectSection("clients")
                    root.openClient(row, clientId)
                }
            }
        }
    }
    Component {
        id: noteDetailComponent
        ProjectNoteDetailPage {
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onMoveToRequested: (id) => moveToDialog.openFor(id)
            onDeleteRequested: root.confirmDelete()
            onNoteActivated: (noteRow, noteId) => root.openNote(noteRow, noteId, projectId)
        }
    }
    Component {
        id: peopleComponent
        PeoplePage {
            onPersonActivated: (row, personId) => root.openPerson(row, personId)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
            onSortRequested: (sx, sy) => sortMenu.openFor(root.currentSection, sx, sy)
            onGoToClientRequested: (clientId) => {
                var row = DesktopAppController.clientRowForId(clientId)
                if (row >= 0) {
                    root.selectSection("clients")
                    root.openClient(row, clientId)
                }
            }
        }
    }
    Component {
        id: personDetailComponent
        PersonDetailPage {
            onPersonActivated: (row, personId) => root.openPerson(row, personId)
            onGoToClientRequested: (clientId) => {
                var row = DesktopAppController.clientRowForId(clientId)
                if (row >= 0) {
                    root.selectSection("clients")
                    root.openClient(row, clientId)
                }
            }
        }
    }
    Component {
        id: clientsComponent
        ClientsPage {
            onClientActivated: (row, clientId) => root.openClient(row, clientId)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
            onSortRequested: (sx, sy) => sortMenu.openFor(root.currentSection, sx, sy)
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
            onSortRequested: (sx, sy) => sortMenu.openFor(root.currentSection, sx, sy)
            onMoveToRequested: (id) => moveToDialog.openFor(id)
        }
    }
    Component {
        id: itemDetailComponent
        ItemDetailPage {
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onMoveToRequested: (id) => moveToDialog.openFor(id)
            onDeleteRequested: root.confirmDelete()
            onItemActivated: (itemId) => root.openItem(itemId)
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

    // About is a compact, modal application dialog (matching ScheduleVault)
    // rather than another category in the Settings page.
    Dialog {
        id: aboutDialog
        objectName: "aboutDialog"
        anchors.centerIn: parent
        width: 500
        scale: Theme.uiScale
        modal: true
        padding: 0
        closePolicy: Popup.CloseOnEscape
        title: qsTr("About Project Notes")

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
                    text: aboutDialog.title
                    color: Theme.text
                    font.pixelSize: Theme.font2xl
                    font.weight: Font.Bold
                    Layout.fillWidth: true
                }
                MaterialIcon {
                    name: "close"
                    size: 20
                    color: Theme.text3
                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds
                        onTapped: aboutDialog.close()
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 18
                spacing: 8

                Image {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 88
                    Layout.preferredHeight: 88
                    source: "qrc:/qt/qml/ProjectNotesDesktop/icons/projectnotes.ico"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Project Notes")
                    color: Theme.text
                    font.pixelSize: Theme.fontBody + 10
                    font.weight: Font.Bold
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Version %1").arg(Qt.application.version)
                    color: Theme.text2
                    font.pixelSize: Theme.fontBody
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Build timestamp: %1").arg(DesktopAppController.buildTimestamp())
                    color: Theme.text3
                    font.pixelSize: Theme.fontSm
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Qt %1").arg(DesktopAppController.qtRuntimeVersion())
                    color: Theme.text3
                    font.pixelSize: Theme.fontSm
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    visible: DesktopAppController.developerProfile().length > 0
                    text: qsTr("Profile: %1").arg(DesktopAppController.developerProfile())
                    color: Theme.text3
                    font.pixelSize: Theme.fontSm
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "© 2022–2026 Paul McKinney · GPL-3.0-only"
                    color: Theme.text3
                    font.pixelSize: Theme.fontSm
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 4
                    spacing: 6
                    Repeater {
                        model: [
                            { label: qsTr("Documentation"), url: "https://projectnotes.readthedocs.io/" },
                            { label: qsTr("Release Notes"), url: "https://github.com/kestermckinney/ProjectNotes/releases" },
                            { label: qsTr("Source Code"), url: "https://github.com/kestermckinney/ProjectNotes" }
                        ]
                        delegate: Button {
                            required property var modelData
                            implicitHeight: 28
                            leftPadding: 12
                            rightPadding: 12
                            topPadding: 0
                            bottomPadding: 0
                            contentItem: Text {
                                text: modelData.label
                                color: Theme.text
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: Qt.openUrlExternally(modelData.url)
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    implicitHeight: aboutPromoColumn.implicitHeight + 20
                    color: Theme.surface2
                    radius: Theme.radius
                    border.color: Theme.border

                    ColumnLayout {
                        id: aboutPromoColumn
                        anchors { top: parent.top; left: parent.left; right: parent.right; margins: 10 }
                        spacing: 5
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Take Project Notes With You")
                            font.pixelSize: Theme.fontLg
                            font.weight: Font.Bold
                            color: Theme.text
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Download the Project Notes mobile app for iOS to check status, review notes, and stay on top of tracker items on the go.")
                            font.pixelSize: Theme.fontSm
                            color: Theme.text3
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 2
                    text: "<a href=\"https://www.projectnotespro.com\">www.projectnotespro.com</a>"
                    textFormat: Text.RichText
                    font.pixelSize: Theme.fontBody
                    color: Theme.accent
                    linkColor: Theme.accent
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                }

                Button {
                    Layout.alignment: Qt.AlignRight
                    Layout.topMargin: 4
                    text: qsTr("Close")
                    onClicked: aboutDialog.close()
                }
            }
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
                    color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold
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
                    font.pixelSize: Theme.fontLg
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
                            color: "#ffffff"; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
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
                    color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold
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
                    font.pixelSize: Theme.fontLg
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
                            color: "#ffffff"; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
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

    // Cloud sync check, shown while the user is on their way out of Settings
    // after editing the sync credentials or the encryption phrase (see
    // _gateOnSyncCheck). An empty status means the check is still running;
    // anything else is a problem worth stopping for.
    Dialog {
        id: syncCheckDialog
        property string status: ""
        property string message: ""
        readonly property bool checking: status === ""
        // "Back to Settings" is the only outcome that keeps the user here, so
        // credential/phrase problems are the only ones offering a second button.
        readonly property bool fixable: status === "credentials" || status === "encryption"

        anchors.centerIn: parent
        width: 420
        scale: Theme.uiScale   // match the zoomed workspace (centered origin)
        modal: true
        padding: 0
        closePolicy: checking ? Popup.NoAutoClose : Popup.CloseOnEscape
        title: checking ? qsTr("Checking Cloud Sync") : qsTr("Cloud Sync")

        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
        header: null
        footer: null

        // Escape and the close button mean "stay here and fix it", so drop the
        // navigation parked against this dialog. The buttons that do continue
        // run it before closing.
        onClosed: root._pendingNavigation = null

        contentItem: ColumnLayout {
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 14
                spacing: 8
                MaterialIcon {
                    name: syncCheckDialog.checking ? "sync"
                          : (syncCheckDialog.status === "offline" ? "cloud_off" : "error")
                    size: 20
                    color: syncCheckDialog.fixable ? Theme.red : Theme.accent
                }
                Text {
                    text: syncCheckDialog.title
                    color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold
                    Layout.fillWidth: true
                }
                MaterialIcon {
                    name: "close"; size: 20; color: Theme.text3
                    visible: !syncCheckDialog.checking
                    // Exclusive grab: see the matching infoDialog close button above.
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: syncCheckDialog.close() }
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
                    font.pixelSize: Theme.fontLg
                    text: syncCheckDialog.checking
                          ? qsTr("Checking your sync email, password, and encryption phrase…")
                          : syncCheckDialog.message
                }

                // Indeterminate sweep while the sync host is contacted — same
                // treatment as the update downloader's unknown-size case.
                Rectangle {
                    Layout.fillWidth: true
                    visible: syncCheckDialog.checking
                    implicitHeight: 6
                    radius: 3
                    color: Theme.surface
                    border.color: Theme.border
                    clip: true
                    Rectangle {
                        id: syncCheckFill
                        height: parent.height
                        width: parent.width * 0.3
                        radius: 3
                        color: Theme.accent
                        SequentialAnimation on x {
                            running: syncCheckDialog.visible && syncCheckDialog.checking
                            loops: Animation.Infinite
                            NumberAnimation { from: 0; to: syncCheckFill.parent.width * 0.7; duration: 900; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: syncCheckFill.parent.width * 0.7; to: 0; duration: 900; easing.type: Easing.InOutQuad }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Item { Layout.fillWidth: true }

                    // Leave with the settings as typed — they stay saved either way.
                    Rectangle {
                        visible: syncCheckDialog.fixable
                        implicitHeight: 30
                        implicitWidth: syncLeaveText.implicitWidth + 26
                        radius: Theme.radiusSm
                        color: syncLeaveHover.hovered ? Theme.surface2 : Theme.surface
                        border.color: Theme.border
                        Text {
                            id: syncLeaveText
                            anchors.centerIn: parent
                            text: qsTr("Leave Anyway")
                            color: Theme.text; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
                        }
                        HoverHandler { id: syncLeaveHover }
                        TapHandler {
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            onTapped: { root._resumeNavigation(); syncCheckDialog.close() }
                        }
                    }

                    Rectangle {
                        implicitHeight: 30
                        implicitWidth: syncPrimaryText.implicitWidth + 26
                        radius: Theme.radiusSm
                        color: syncPrimaryHover.hovered ? Theme.accentStrong : Theme.accent
                        Text {
                            id: syncPrimaryText
                            anchors.centerIn: parent
                            text: syncCheckDialog.checking ? qsTr("Skip")
                                  : (syncCheckDialog.fixable ? qsTr("Back to Settings") : qsTr("OK"))
                            color: "#ffffff"; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
                        }
                        HoverHandler { id: syncPrimaryHover }
                        TapHandler {
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            onTapped: {
                                // Skip and OK carry on to wherever the user was
                                // headed; Back to Settings keeps them here.
                                if (!syncCheckDialog.fixable)
                                    root._resumeNavigation()
                                syncCheckDialog.close()
                            }
                        }
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
                    color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold
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
                    font.pixelSize: Theme.fontLg
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
                            color: Theme.text; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
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
                            color: "#ffffff"; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
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
                    color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold
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
                    font.pixelSize: Theme.fontLg
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
                            color: Theme.text; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
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
                    color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold
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
                    font.pixelSize: Theme.fontLg
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
                            color: Theme.text; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
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
                            font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
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
    SortMenu { id: sortMenu }

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
