// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import ProjectNotesDesktop

// Project detail. Matching the mockup: ALL project header information lives above
// the tabs — the editable core fields plus the calculated financial (EVM) tiles —
// and each tab carries a live count badge. The tabs themselves hold only the child
// lists (Status Report items, Tracker, Team, Locations, Notes).
Item {
    id: page

    property int    projectRow: -1
    property string projectId:  ""
    property bool   isNewRecord: false
    property bool   _changed: false

    // Draggable header height (see headerResizeHandle below). Defaults to the
    // original proportional height until the user drags the handle or a saved
    // per-user preference is loaded in Component.onCompleted.
    property real   _headerHeight: page.height * 0.62
    readonly property real _minHeaderHeight: 72

    // Overlay layer a dragged tracker item card reparents onto while dragging
    // (threaded down from Main.qml's dragOverlay, same as the sidebar's).
    property var    dragLayer: null

    // Consumed by the TopBar's Export XML action.
    readonly property string exportTable: "projects"
    readonly property string exportId: projectId

    // Consumed by the TopBar's Delete action (Main.deleteCurrent()). A staged new
    // project has nothing to delete — leaving the page discards it.
    readonly property bool canDelete: page.projectId !== ""

    // Delete this project. Removes the row from the shared projects model, so the
    // projects list refreshes itself once Main pops back to it.
    function _deleteRecord() {
        var r = DesktopAppController.projectRowForId(page.projectId)
        if (r < 0) return false
        return DesktopAppController.deleteProject(r)
    }

    // Client / contact id<->name mapping tables (built once).
    property var _clients: []
    property var _people: []

    // Financial fields. Budget/Actual/BCWP/BCWS/BAC are stored and editable;
    // EAC (and the other EVM ratios) are calculated read-only in the model.
    property string _budget: ""
    property string _actual: ""
    property string _bcwp: ""
    property string _bcws: ""
    property string _bac: ""
    property string _eac: ""
    property string _cv: ""
    property string _sv: ""
    property string _cpi: ""
    property string _pctComplete: ""

    signal noteActivated(int noteRow, string noteId)
    signal itemActivated(string itemId)
    // Duplicate on the page's own menu opens the copy — routed through Main so
    // it gets a breadcrumb, a sidebar selection and a history entry.
    signal projectActivated(string projectId)
    // Routed to Main.exportRecord when a sub-table row's menu exports XML.
    signal exportRequested(string table, string id)
    // Routed to Main → MoveToDialog when a tracker item's "Move To…" is chosen.
    signal moveToRequested(string itemId)
    // Page-level record menu (kebab + right-click on the page background),
    // mirroring the sidebar's per-row Project menu. Routed to Main so it can
    // reuse the same confirm-delete / add-project / filter flows as elsewhere.
    signal deleteRequested()
    signal newRequested()
    signal filterRequested()
    signal sortRequested(real sx, real sy)
    // Column filter for one of this page's sub-tab lists (Status Report,
    // Tracker, Team, Locations, Notes) — routed to Main's shared FilterDialog,
    // keyed by the tab's filterSection name (see FilterDialog.openFor()).
    signal subFilterRequested(string section)
    // Sort for one of this page's sub-tab lists — same idea as
    // subFilterRequested above, but also carries the chip's own scene
    // position since SortMenu.openFor() positions itself by coordinates
    // rather than an Item reference (see SortMenu.qml's doc comment).
    signal subSortRequested(string section, real sx, real sy)
    // Navigation signals from sub-table row menus
    signal goToPersonRequested(string personId)
    signal goToClientRequested(string clientId)
    // A new project that was staged by the New Project button has just been
    // written and now has an id (see _adoptSavedRecord).
    signal recordPersisted(string projectId)

    // Whether a sub-tab's own model currently has an active column filter —
    // drives its SectionBar's Filter chip highlight. Reads filterRev first so
    // it re-evaluates whenever any filter (quick or manual) changes anywhere.
    function _sectionFilterActive(section) {
        DesktopAppController.filterRev
        var m = DesktopAppController.modelForSection(section)
        return m ? DesktopAppController.hasActiveColumnFilters(m) : false
    }
    // Whether a sub-tab's own model currently has an active sort — drives its
    // SectionBar's Sort chip highlight. Reads sortRev first, same reason.
    function _sectionSortActive(section) {
        DesktopAppController.sortRev
        var m = DesktopAppController.modelForSection(section)
        return m ? (DesktopAppController.activeSort(m).field || "") !== "" : false
    }

    function _clientNames() { return _clients.map(function(c){ return c.name }) }
    function _peopleNames() { return _people.map(function(p){ return p.name }) }
    function _idForName(list, name) {
        for (var i = 0; i < list.length; i++) if (list[i].name === name) return list[i].id
        return ""
    }
    function _nameForId(list, id) {
        for (var i = 0; i < list.length; i++) if (list[i].id === id) return list[i].name
        return ""
    }
    // Reload the team-member list backing the Primary Contact combo (keeping the
    // current contact) after the team roster changes.
    function _refreshTeamPeople() {
        page._people = DesktopAppController.teamMemberList(page.projectId, [page._contactId])
    }
    function _money(v) { var s = (v || "").toString().trim(); return s === "" ? "—" : s }
    // The data layer already formats percent columns with a trailing "%", so we
    // only blank-guard here (don't append another).
    function _pct(v)   { var s = (v || "").toString().trim(); return s === "" ? "—" : s }
    function _num(v)   { var s = (v || "").toString().trim(); return s === "" ? "—" : s }

    // Tracker-list display helpers (mirrors ItemsPage styling).
    function _statusColor(s) {
        s = (s || "").toLowerCase()
        if (s.indexOf("resolved") >= 0 || s.indexOf("closed") >= 0) return Theme.green
        if (s.indexOf("assigned") >= 0) return Theme.amber
        if (s.indexOf("new") >= 0) return Theme.red
        return Theme.text3
    }
    function _priorityColor(p) {
        p = (p || "").toLowerCase()
        if (p.indexOf("high") >= 0) return Theme.red
        if (p.indexOf("medium") >= 0) return Theme.amber
        return Theme.text3
    }

    Component.onCompleted: {
        _clients = DesktopAppController.clientList()
        _reload()
        // For a staged project this filters every child list to an id no row has,
        // i.e. leaves them empty — which is what a project that doesn't exist yet
        // should show (rather than the previously-viewed project's rows).
        DesktopAppController.setProjectFilter(page.projectId)
        DesktopAppController.refreshProjectNotes()
        // These sub-tab models are shared, project-scoped singletons reused by
        // every ProjectDetailPage instance — clear any quick search left over
        // from a previously-viewed project so it doesn't silently carry over.
        DesktopAppController.setQuickSearch(DesktopAppController.statusReportItemsModel, "")
        DesktopAppController.setQuickSearch(DesktopAppController.projectTrackerItemsModel, "")
        DesktopAppController.setQuickSearch(DesktopAppController.projectTeamMembersModel, "")
        DesktopAppController.setQuickSearch(DesktopAppController.projectLocationsModel, "")
        DesktopAppController.setQuickSearch(DesktopAppController.projectNotesModel, "")
        if (page.projectId !== "")
            tabBar.currentIndex = DesktopAppController.lastProjectDetailTab(page.projectId)
        var savedHeaderHeight = DesktopAppController.projectDetailHeaderHeight()
        if (savedHeaderHeight > 0) page._headerHeight = savedHeaderHeight
    }

    property string _clientId: ""
    property string _contactId: ""

    function _reload() {
        var d = DesktopAppController.getProjectData(page.projectRow)
        numberField.text  = (d.project_number || "").toString()
        nameField.text    = (d.project_name || "").toString()
        statusCombo.value = (d.project_status || "").toString()
        statusDate.text   = (d.last_status_date || "").toString()
        invoiceDate.text  = (d.last_invoice_date || "").toString()
        invoicingCombo.value = (d.invoicing_period || "").toString()
        reportCombo.value = (d.status_report_period || "").toString()
        page._clientId  = (d.client_id || "").toString()
        page._contactId = (d.primary_contact || "").toString()
        // Primary Contact lists only this project's team members (plus the current
        // contact, so an existing value keeps displaying) — as in the Widgets app.
        page._people = DesktopAppController.teamMemberList(page.projectId,
                            [page._contactId])
        clientCombo.value  = _nameForId(page._clients, page._clientId)
        contactCombo.value = _nameForId(page._people, page._contactId)
        page._budget = (d.budget || "").toString()
        page._actual = (d.actual || "").toString()
        page._bcwp   = (d.bcwp || "").toString()
        page._bcws   = (d.bcws || "").toString()
        page._bac    = (d.bac || "").toString()
        page._eac    = (d.eac || "").toString()
        page._cv          = (d.cv || "").toString()
        page._sv          = (d.sv || "").toString()
        page._cpi         = (d.cpi || "").toString()
        page._pctComplete = (d.pct_complete || "").toString()
        budgetField.text = page._budget
        actualField.text = page._actual
        bcwpField.text   = page._bcwp
        bcwsField.text   = page._bcws
        bacField.text    = page._bac
        page._changed = false
        // A staged project starts with no number. Offer the next free one so the
        // record can be created by typing nothing but a name — project_number and
        // project_name are both required, and neither is written until then.
        if (page.projectId === "" && numberField.text.trim() === "")
            numberField.text = DesktopAppController.nextProjectNumber()
    }

    function _saveNow() {
        // A project the New Project button staged isn't in the database yet: the
        // save below is what creates it, and it can only do that once both
        // required fields are filled in. Until then there is nothing to save.
        var isStaged = page.projectId === ""
        if (isStaged) {
            if (numberField.text.trim() === "" || nameField.text.trim() === "")
                return true
            // A staged row lives in the model cache, so a refresh from anywhere
            // else (cloud sync, a plugin) can drop it. If this row now belongs to
            // a stored project, it isn't ours to write over.
            if (DesktopAppController.projectIdAtRow(page.projectRow) !== "")
                return false
        } else if (!page._changed) {
            return true
        }

        var ok = DesktopAppController.saveProject(
            page.projectRow, numberField.text, nameField.text, statusCombo.value,
            page._contactId, page._clientId, statusDate.text, invoiceDate.text,
            invoicingCombo.value, reportCombo.value,
            budgetField.text, actualField.text, bcwpField.text, bcwsField.text, bacField.text)
        // A rejected create (a number or name already in use) leaves the typed
        // values on screen to be corrected — reloading would blank them, since
        // there is still no stored record to read them back from.
        if (!ok && isStaged) return false
        if (ok) {
            page._changed = false
            if (isStaged) _adoptSavedRecord()
        }
        // reload to pick up recalculated EVM on success, or to revert the fields
        // to the last valid values when a rule rejected the edit.
        _reload()
        return ok
    }

    // The staged row has just been written: take on its new id, re-resolve the row
    // (the insert can move it within the sorted list), point the child lists at it
    // and let Main relabel the breadcrumb.
    function _adoptSavedRecord() {
        var id = DesktopAppController.lastCreatedProjectId()
        if (id === "") return
        page.projectId = id
        page.isNewRecord = false
        var r = DesktopAppController.projectRowForId(id)
        if (r >= 0) page.projectRow = r
        DesktopAppController.setProjectFilter(id)
        page._refreshTeamPeople()   // the default project manager was just added
        page.recordPersisted(id)
    }

    // Called by Main when leaving the page with the new project still unwritten.
    function _discardNew() {
        if (page.projectId !== "") return
        DesktopAppController.discardNewProject(page.projectRow)
    }

    // Refresh everything the page shows for this project: the core fields plus
    // every child list. Backs the page-level menu's Refresh action.
    function _refreshAll() {
        // Nothing stored to refresh yet, and re-running the projects query would
        // drop the staged row this page is editing.
        if (page.projectId === "") return
        DesktopAppController.refreshModel(DesktopAppController.projectsListModel)
        page._reload()
        DesktopAppController.refreshStatusItems()
        DesktopAppController.refreshModel(DesktopAppController.projectTrackerItemsModel)
        DesktopAppController.refreshTeamMembers()
        page._refreshTeamPeople()
        DesktopAppController.refreshProjectLocations()
        DesktopAppController.refreshProjectNotes()
    }

    // Open the page's own record menu (kebab or right-click) at scene coords.
    function _openSelfMenu(sx, sy) {
        selfMenu.recordLabel = ((numberField.text || "") + " " + (nameField.text || "")).trim()
        selfMenu.openAt(sx, sy)
    }

    // Right-click anywhere on the page background opens the project menu
    // (parity with right-clicking a project row in the sidebar). Declared
    // beneath the page content so field/button clicks still reach their own
    // handlers first; only right-clicks on empty background fall through here.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: (mouse) => page._openSelfMenu(mouse.x, mouse.y)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // â”€â”€ Header (all project information, above the tabs) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        ScrollView {
            id: headerScroll
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(page._headerHeight,
                                             Math.max(page._minHeaderHeight, page.height - 200))
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: headerScroll.availableWidth
                spacing: 10

                // Title row: number · name · status (all editable inline)
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 13
                    spacing: 9
                    FormField {
                        id: numberField
                        label: qsTr("Project Number")
                        Layout.preferredWidth: 110
                        Layout.fillWidth: false
                        onEdited: page._changed = true
                        // These two fields are what a new project needs before it
                        // can be created, so committing either one is what turns a
                        // staged project into a record (see page._saveNow). Left
                        // empty, the number falls back to the next free one rather
                        // than leaving the project uncreatable.
                        onEditingFinished: {
                            if (page.projectId === "" && numberField.text.trim() === "")
                                numberField.text = DesktopAppController.nextProjectNumber()
                            page._saveNow()
                        }
                    }
                    FormField {
                        id: nameField
                        label: qsTr("Project Name")
                        Layout.fillWidth: true
                        spellCheck: true
                        spellDialog: spellDialog
                        onEdited: page._changed = true
                        onEditingFinished: page._saveNow()
                    }
                    ComboField {
                        id: statusCombo
                        label: qsTr("Status")
                        Layout.preferredWidth: 150
                        Layout.fillWidth: false
                        options: DesktopAppController.projectStatusOptions()
                        onActivated: page._changed = true
                    }
                    // Page-level record menu — parity with the sidebar's per-row
                    // Project kebab (this project's own detail page previously had
                    // no equivalent quick-actions entry point).
                    KebabButton {
                        Layout.alignment: Qt.AlignBottom
                        Layout.bottomMargin: 5
                        onClicked: (sx, sy) => page._openSelfMenu(sx, sy)
                    }
                }

                // A project that hasn't been created yet: say so, and say what
                // creates it. The tabs below stay disabled until then — status
                // items, tracker items, team, locations and notes all hang off the
                // project id, which doesn't exist before the row is written.
                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    visible: page.projectId === ""
                    wrapMode: Text.WordWrap
                    color: Theme.text3
                    font.pixelSize: Theme.fontBody
                    text: qsTr("New project — give it a name to create it. The tabs below become available once it is saved.")
                }

                // Remaining editable fields
                GridLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    columns: page.width > 940 ? 3 : 2
                    columnSpacing: 10
                    rowSpacing: 9
                    ComboField {
                        id: clientCombo
                        label: qsTr("Client")
                        options: page._clientNames()
                        includeNone: true
                        searchable: true
                        onActivated: (v) => { page._clientId = page._idForName(page._clients, v); page._changed = true }
                    }
                    ComboField {
                        id: contactCombo
                        label: qsTr("Primary Contact")
                        options: page._peopleNames()
                        includeNone: true
                        searchable: true
                        onActivated: (v) => { page._contactId = page._idForName(page._people, v); page._changed = true }
                    }
                    DateField { id: statusDate;  label: qsTr("Status Date");  onEdited: page._changed = true }
                    DateField { id: invoiceDate; label: qsTr("Invoice Date"); onEdited: page._changed = true }
                    ComboField {
                        id: invoicingCombo
                        label: qsTr("Invoicing Period")
                        options: DesktopAppController.invoicingPeriodOptions()
                        onActivated: page._changed = true
                    }
                    ComboField {
                        id: reportCombo
                        label: qsTr("Status Report Period")
                        options: DesktopAppController.statusReportPeriodOptions()
                        onActivated: page._changed = true
                    }
                }

                // Financial (EVM) fields — gated by the "show internal / budget
                // items" view option, like the Widgets app. Budget/Actual/BCWP/
                // BCWS/BAC are editable inputs; EAC is calculated (read-only).
                // Editable budget inputs.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    spacing: 8
                    visible: DesktopAppController.showInternalItems
                    // Commit on editingFinished (focus leaves the field) so the
                    // calculated EVM tiles below refresh as each value is entered,
                    // rather than only when navigating away.
                    FormField { id: budgetField; label: qsTr("Budget");        onEdited: page._changed = true; onEditingFinished: page._saveNow() }
                    FormField { id: actualField; label: qsTr("Actual (ACWP)"); onEdited: page._changed = true; onEditingFinished: page._saveNow() }
                    FormField { id: bcwpField;   label: qsTr("BCWP");          onEdited: page._changed = true; onEditingFinished: page._saveNow() }
                    FormField { id: bcwsField;   label: qsTr("BCWS");          onEdited: page._changed = true; onEditingFinished: page._saveNow() }
                    FormField { id: bacField;    label: qsTr("BAC");           onEdited: page._changed = true; onEditingFinished: page._saveNow() }
                }

                // Calculated (read-only) tiles shown below the entered values.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    spacing: 8
                    visible: DesktopAppController.showInternalItems
                    MetricTile { label: qsTr("EAC");       value: page._money(page._eac); valueColor: Theme.amber }
                    MetricTile { label: qsTr("CV");        value: page._pct(page._cv) }
                    MetricTile { label: qsTr("SV");        value: page._pct(page._sv) }
                    MetricTile { label: qsTr("CPI");       value: page._num(page._cpi) }
                    MetricTile { label: qsTr("% Complete"); value: page._pct(page._pctComplete) }
                }

                Item { Layout.preferredHeight: 2 }
            }
        }

        // â”€â”€ Header resize handle â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        // Drags page._headerHeight; the height is persisted per-user (not synced)
        // via DesktopAppController.setProjectDetailHeaderHeight on release.
        Rectangle {
            id: headerResizeHandle
            Layout.fillWidth: true
            Layout.preferredHeight: 6
            color: (handleArea.pressed || handleArea.containsMouse) ? Theme.accent : "transparent"

            MouseArea {
                id: handleArea
                anchors.fill: parent
                anchors.margins: -3
                hoverEnabled: true
                cursorShape: Qt.SizeVerCursor
                property real dragStartY: 0
                property real dragStartHeight: 0
                onPressed: (mouse) => {
                    dragStartY = mapToItem(page, mouse.x, mouse.y).y
                    dragStartHeight = page._headerHeight
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    var currentY = mapToItem(page, mouse.x, mouse.y).y
                    var minH = page._minHeaderHeight
                    var maxH = Math.max(minH, page.height - 200)
                    page._headerHeight = Math.min(maxH, Math.max(minH, dragStartHeight + (currentY - dragStartY)))
                }
                onReleased: DesktopAppController.setProjectDetailHeaderHeight(page._headerHeight)
            }
        }

        // â”€â”€ Tabs (with live count badges) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            // Every tab below is a list of child records keyed by the project id,
            // so none of them mean anything until the project exists.
            enabled: page.projectId !== ""
            opacity: enabled ? 1 : 0.4
            background: Rectangle {
                color: "transparent"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
            }
            onCurrentIndexChanged: {
                if (page.projectId !== "")
                    DesktopAppController.setLastProjectDetailTab(page.projectId, currentIndex)
            }
            TabItem { iconName: "flag";        label: qsTr("Status Report"); count: statusRep.count }
            TabItem { iconName: "task_alt";    label: qsTr("Tracker");       count: trackerRep.count }
            TabItem { iconName: "groups";      label: qsTr("Team");          count: teamRep.count }
            TabItem { iconName: "folder";      label: qsTr("Locations");     count: locRep.count }
            TabItem { iconName: "description"; label: qsTr("Notes");         count: notesRep.count }
        }

        // â”€â”€ Tab content â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex
            enabled: page.projectId !== ""   // see the TabBar above
            opacity: enabled ? 1 : 0.4

            // â”€â”€ 0: STATUS REPORT ITEMS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            // Virtualized: only visible rows exist; the tab badge reads
            // statusRep.count, which is the model's row count either way.
            // No reuseItems on these tabs — delegates hold edit-field state.
            Item {
                ListView {
                    id: statusRep
                    anchors.fill: parent
                    anchors.margins: 13
                    clip: true
                    spacing: 7
                    model: DesktopAppController.statusReportItemsModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Item { width: 1; height: 6 }
                    header: Item {
                        width: ListView.view ? ListView.view.width : 0
                        height: statusBar.implicitHeight + 8
                        SectionBar {
                            id: statusBar
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                            title: qsTr("Status Report Items")
                            icon: "flag"
                            addLabel: qsTr("Add Status Item")
                            searchModel: DesktopAppController.statusReportItemsModel
                            filterSection: "statusreport"
                            filterActive: page._sectionFilterActive("statusreport")
                            sortActive: page._sectionSortActive("statusreport")
                            // addStatusItem appends a pending (unsaved) row; it is INSERTed
                            // only when a field is edited. Do NOT refresh here — refresh()
                            // re-queries the DB and would wipe the new row before it is seen.
                            onAdd: DesktopAppController.addStatusItem(page.projectId)
                            onFilter: page.subFilterRequested(filterSection)
                            onSort: (sx, sy) => page.subSortRequested(filterSection, sx, sy)
                        }
                    }
                    delegate: Card {
                            id: stCard
                            required property int index
                            required property var model
                            width: ListView.view ? ListView.view.width : 0
                            implicitHeight: 44
                            function _menu(sx, sy) {
                                rowMenu.openFor(DesktopAppController.statusReportItemsModel,
                                    (stCard.model.id || "").toString(), qsTr("Status Item"),
                                    (stCard.model.task_description || "").toString(), sx, sy)
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 6
                                spacing: 6
                                ComboField {
                                    Layout.preferredWidth: 115
                                    Layout.fillWidth: false
                                    options: DesktopAppController.statusItemCategoryOptions()
                                    value: (stCard.model.task_category || "").toString()
                                    onActivated: (v) => DesktopAppController.saveStatusItem(
                                        stCard.index, v, (stCard.model.task_description || "").toString())
                                }
                                Rectangle {
                                    Layout.fillWidth: true; implicitHeight: 26
                                    radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.border
                                    TextField {
                                        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                                        verticalAlignment: Text.AlignVCenter
                                        text: (stCard.model.task_description || "").toString()
                                        placeholderText: qsTr("Description")
                                        placeholderTextColor: Theme.text3
                                        color: Theme.text; background: null; font.pixelSize: Theme.fontBody
                                        onEditingFinished: DesktopAppController.saveStatusItem(
                                            stCard.index, (stCard.model.task_category || "").toString(), text)
                                        SpellCheckField { dialog: spellDialog }
                                    }
                                }
                                KebabButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    onClicked: (sx, sy) => stCard._menu(sx, sy)
                                }
                                RowDelete { onDel: { DesktopAppController.deleteStatusItem(stCard.index); DesktopAppController.refreshStatusItems() } }
                            }
                            TapHandler {
                                acceptedButtons: Qt.RightButton
                                onTapped: (ev) => stCard._menu(ev.scenePosition.x, ev.scenePosition.y)
                            }
                    }
                }
            }

            // â”€â”€ 1: TRACKER â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            Item {
                ListView {
                    id: trackerRep
                    anchors.fill: parent
                    anchors.margins: 13
                    clip: true
                    spacing: 7
                    model: DesktopAppController.projectTrackerItemsModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Item { width: 1; height: 6 }
                    header: Item {
                        width: ListView.view ? ListView.view.width : 0
                        height: trackerBar.implicitHeight + 8
                        SectionBar {
                            id: trackerBar
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                            title: qsTr("Tracker Items")
                            icon: "task_alt"
                            addLabel: qsTr("Add Item")
                            searchModel: DesktopAppController.projectTrackerItemsModel
                            filterSection: "trackeritems"
                            filterActive: page._sectionFilterActive("trackeritems")
                            sortActive: page._sectionSortActive("trackeritems")
                            onAdd: {
                                page._saveNow()
                                DesktopAppController.addTrackerItem(page.projectId)
                                var d = DesktopAppController.getTrackerItemDetailData(0)
                                if (d.id !== undefined) page.itemActivated(d.id.toString())
                            }
                            onFilter: page.subFilterRequested(filterSection)
                            onSort: (sx, sy) => page.subSortRequested(filterSection, sx, sy)
                        }
                    }
                    // An outer Item is the actual delegate and stays put
                    // (reserving the row's slot); the inner Card is what visually
                    // reparents onto page.dragLayer while being dragged (mirrors
                    // FolderGroup's row/content split).
                    delegate: Item {
                            id: trackerSlot
                            required property int index
                            required property var model
                            readonly property string iid: model.id !== undefined ? model.id : ""
                            width: ListView.view ? ListView.view.width : 0
                            implicitHeight: trackerCard.implicitHeight
                            function _menu(sx, sy) {
                                rowMenu.openFor(DesktopAppController.projectTrackerItemsModel,
                                    trackerSlot.iid, qsTr("Tracker Item"),
                                    (trackerSlot.model.item_name || "").toString(), sx, sy,
                                    /*allowMoveTo*/ true)
                            }

                            Card {
                                id: trackerCard
                                width: trackerSlot.width
                                x: 0; y: 0
                                implicitHeight: tiCol.implicitHeight + 16
                                height: implicitHeight
                                color: dragArea.drag.active ? Theme.surface2
                                     : (tiHover.hovered ? Theme.raise : Theme.surface)

                                property string itemId: trackerSlot.iid

                                ColumnLayout {
                                    id: tiCol
                                    anchors.left: parent.left; anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.leftMargin: 10; anchors.rightMargin: 10
                                    spacing: 3
                                    // Title row: number · name · status
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        Text {
                                            text: (trackerSlot.model.item_number || "").toString()
                                            color: Theme.text3; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                            Layout.preferredWidth: 52
                                            Layout.maximumWidth: 52
                                        }
                                        Text {
                                            text: (trackerSlot.model.item_name || qsTr("(unnamed)")).toString()
                                            color: Theme.text; font.pixelSize: Theme.fontBody; Layout.fillWidth: true; elide: Text.ElideRight
                                        }
                                        Rectangle {
                                            readonly property color c: page._statusColor((trackerSlot.model.status || "").toString())
                                            visible: (trackerSlot.model.status || "").toString() !== ""
                                            radius: 5
                                            color: Qt.rgba(c.r, c.g, c.b, 0.14)
                                            implicitHeight: 16; implicitWidth: tiStatus.implicitWidth + 12
                                            Layout.alignment: Qt.AlignVCenter
                                            Text {
                                                id: tiStatus; anchors.centerIn: parent
                                                text: (trackerSlot.model.status || "").toString()
                                                color: parent.c; font.pixelSize: Theme.font2xs; font.weight: Font.DemiBold
                                            }
                                        }
                                        KebabButton {
                                            implicitWidth: 22; implicitHeight: 22
                                            Layout.alignment: Qt.AlignVCenter
                                            onClicked: (sx, sy) => trackerSlot._menu(sx, sy)
                                        }
                                        MaterialIcon { name: "chevron_right"; size: 16; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                                    }
                                    // Metadata row: assigned · priority · due
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.leftMargin: 46
                                        spacing: 13
                                        MetaPair {
                                            label: qsTr("Assigned")
                                            value: page._nameForId(page._people, (trackerSlot.model.assigned_to || "").toString())
                                        }
                                        MetaPair {
                                            label: qsTr("Priority")
                                            value: (trackerSlot.model.priority || "").toString()
                                            valueColor: page._priorityColor((trackerSlot.model.priority || "").toString())
                                        }
                                        MetaPair {
                                            label: qsTr("Due")
                                            value: (trackerSlot.model.date_due || "").toString()
                                        }
                                        Item { Layout.fillWidth: true }
                                    }
                                }
                                HoverHandler { id: tiHover }

                                // â”€â”€ Drag source: drag onto a project row in the
                                // sidebar to move this item there.
                                Drag.active: dragArea.drag.active
                                Drag.source: trackerCard
                                Drag.keys: ["trackerItem"]
                                Drag.hotSpot.x: width / 2
                                Drag.hotSpot.y: height / 2

                                // z: -1 so this background MouseArea sits behind
                                // tiCol's own children — a click/press lands on the
                                // kebab (or any future interactive child) first, and
                                // only falls through to this MouseArea (activate /
                                // context-menu / drag) where tiCol has nothing to
                                // claim it.
                                MouseArea {
                                    id: dragArea
                                    z: -1
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    preventStealing: true
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    drag.target: trackerCard
                                    drag.threshold: 6
                                    onClicked: (mouse) => {
                                        if (mouse.button === Qt.RightButton) {
                                            var p = dragArea.mapToItem(null, mouse.x, mouse.y)
                                            trackerSlot._menu(p.x, p.y)
                                        } else {
                                            page._saveNow()
                                            page.itemActivated(trackerSlot.iid)
                                        }
                                    }
                                    onReleased: if (drag.active) trackerCard.Drag.drop()
                                }

                                states: State {
                                    when: dragArea.drag.active
                                    ParentChange {
                                        target: trackerCard
                                        parent: page.dragLayer ? page.dragLayer : trackerCard.parent
                                    }
                                }
                            }
                    }
                }
            }

            // â”€â”€ 2: TEAM â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            Item {
                // Drop a vCard (a .vcf file, or a contact dragged straight from
                // Outlook / Contacts) anywhere on the team list to add it: the
                // contact's company is associated with a matching client (or a
                // new one is created), the person is added/looked up, and they
                // are added to this project's team. See
                // DesktopAppController::addTeamMembersFromVCardDrop().
                DropArea {
                    anchors.fill: parent
                    onDropped: (drop) => {
                        var urls = []
                        if (drop.hasUrls)
                            for (var i = 0; i < drop.urls.length; i++)
                                urls.push(drop.urls[i].toString())
                        var text = ""
                        for (var i = 0; i < drop.formats.length; i++) {
                            if (drop.formats[i].toLowerCase().indexOf("vcard") !== -1) {
                                text = drop.getDataAsString(drop.formats[i])
                                break
                            }
                        }
                        if (text === "" && drop.hasText && drop.text.indexOf("BEGIN:VCARD") !== -1)
                            text = drop.text
                        if (urls.length === 0 && text === "") { drop.accepted = false; return }
                        DesktopAppController.addTeamMembersFromVCardDrop(page.projectId, urls, text)
                        page._refreshTeamPeople()
                        drop.accept()
                    }
                }

                ListView {
                    id: teamRep
                    anchors.fill: parent
                    anchors.margins: 13
                    clip: true
                    spacing: 7
                    model: DesktopAppController.projectTeamMembersModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Item { width: 1; height: 6 }
                    header: Item {
                        width: ListView.view ? ListView.view.width : 0
                        height: teamBar.implicitHeight + 8
                        SectionBar {
                            id: teamBar
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                            title: qsTr("Team")
                            icon: "groups"
                            addLabel: qsTr("Add Member")
                            searchModel: DesktopAppController.projectTeamMembersModel
                            filterSection: "team"
                            filterActive: page._sectionFilterActive("team")
                            sortActive: page._sectionSortActive("team")
                            onAdd: { page._saveNow(); teamPicker.open() }
                            onFilter: page.subFilterRequested(filterSection)
                            onSort: (sx, sy) => page.subSortRequested(filterSection, sx, sy)
                        }
                    }
                    delegate: Card {
                            id: teamCard
                            required property int index
                            required property var model
                            width: ListView.view ? ListView.view.width : 0
                            implicitHeight: 44
                            function _menu(sx, sy) {
                                rowMenu.openFor(DesktopAppController.projectTeamMembersModel,
                                    (teamCard.model.id || "").toString(), qsTr("Team Member"),
                                    (teamCard.model.name || "").toString(), sx, sy,
                                    /*allowMoveTo*/ false,
                                    (teamCard.model.people_id || "").toString())
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 6
                                spacing: 8
                                MaterialIcon { name: "person"; size: 15; color: Theme.text3 }
                                Text {
                                    text: (teamCard.model.name || qsTr("(no name)")).toString()
                                    color: Theme.text; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
                                    Layout.preferredWidth: 130; elide: Text.ElideRight
                                }
                                Rectangle {
                                    Layout.fillWidth: true; implicitHeight: 26
                                    radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.border
                                    TextField {
                                        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                                        verticalAlignment: Text.AlignVCenter
                                        text: (teamCard.model.role || "").toString()
                                        placeholderText: qsTr("Role")
                                        placeholderTextColor: Theme.text3
                                        color: Theme.text; background: null; font.pixelSize: Theme.fontBody
                                        onEditingFinished: DesktopAppController.saveTeamMember(
                                            teamCard.index, (teamCard.model.people_id || "").toString(), text,
                                            (teamCard.model.receive_status_report || "0") !== "0")
                                        SpellCheckField { dialog: spellDialog }
                                    }
                                }
                                CheckBox {
                                    id: statusCheck
                                    Layout.alignment: Qt.AlignVCenter
                                    checked: (teamCard.model.receive_status_report || "0") !== "0"
                                    onToggled: DesktopAppController.saveTeamMember(
                                        teamCard.index, (teamCard.model.people_id || "").toString(),
                                        (teamCard.model.role || "").toString(), checked)
                                    indicator: Rectangle {
                                        implicitWidth: 16; implicitHeight: 16; radius: 4
                                        x: statusCheck.leftPadding; y: parent.height/2 - height/2
                                        color: statusCheck.checked ? Theme.accent : Theme.surface
                                        border.color: statusCheck.checked ? Theme.accent : Theme.border
                                        MaterialIcon { anchors.centerIn: parent; visible: statusCheck.checked; name: "check"; size: 12; color: "#ffffff" }
                                    }
                                    contentItem: Text {
                                        text: qsTr("Status Report"); color: Theme.text3; font.pixelSize: Theme.fontXs
                                        leftPadding: statusCheck.indicator.width + 5; verticalAlignment: Text.AlignVCenter
                                    }
                                }
                                KebabButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    onClicked: (sx, sy) => teamCard._menu(sx, sy)
                                }
                                RowDelete { onDel: { DesktopAppController.deleteTeamMember(teamCard.index); DesktopAppController.refreshTeamMembers(); page._refreshTeamPeople() } }
                            }
                            TapHandler {
                                acceptedButtons: Qt.RightButton
                                onTapped: (ev) => teamCard._menu(ev.scenePosition.x, ev.scenePosition.y)
                            }
                    }
                }
            }

            // â”€â”€ 3: LOCATIONS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            // Wrapper hosts a DropArea so files / web links dragged anywhere onto
            // the tab are added as locations (the ScrollView itself can't be a
            // drop target without swallowing its own flick content).
            Item {
                id: locationsTab

                // Adds one location per dropped file or web link. Local files
                // arrive as file:// URLs (converted to a path in the controller);
                // web links arrive as their URL, or as plain text from some apps.
                DropArea {
                    id: locationDrop
                    anchors.fill: parent
                    onDropped: (drop) => {
                        if (drop.hasUrls) {
                            for (var i = 0; i < drop.urls.length; i++)
                                DesktopAppController.addProjectLocationFromUrl(page.projectId, drop.urls[i])
                            drop.accept()
                        } else if (drop.hasText && drop.text.length > 0) {
                            DesktopAppController.addProjectLocationFromUrl(page.projectId, drop.text)
                            drop.accept()
                        }
                    }
                }

                // Shared file picker: targetRow < 0 → add a new location,
                // otherwise re-point that row's path.
                FileDialog {
                    id: locationFileDialog
                    property int targetRow: -1
                    title: qsTr("Select a file")
                    fileMode: FileDialog.OpenFile
                    onAccepted: {
                        if (targetRow < 0)
                            DesktopAppController.addProjectLocationFromUrl(page.projectId, selectedFile)
                        else
                            DesktopAppController.setProjectLocationPath(targetRow, selectedFile)
                    }
                }

                ListView {
                    id: locRep
                    anchors.fill: parent
                    anchors.margins: 13
                    clip: true
                    spacing: 7
                    model: DesktopAppController.projectLocationsModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Item { width: 1; height: 6 }
                    header: Column {
                        width: ListView.view ? ListView.view.width : 0
                        spacing: 8
                        bottomPadding: 8
                        RowLayout {
                            width: parent.width
                            spacing: 6
                            SectionBar {
                                Layout.fillWidth: true
                                title: qsTr("Locations")
                                icon: "folder"
                                addLabel: qsTr("Add Location")
                                searchModel: DesktopAppController.projectLocationsModel
                                filterSection: "locations"
                                filterActive: page._sectionFilterActive("locations")
                                sortActive: page._sectionSortActive("locations")
                                // Pending row is INSERTed on first edit; refresh() would wipe it.
                                onAdd: DesktopAppController.addProjectLocation(page.projectId)
                                onFilter: page.subFilterRequested(filterSection)
                                onSort: (sx, sy) => page.subSortRequested(filterSection, sx, sy)
                            }
                            // Browse for a file and add it as a new location.
                            Rectangle {
                                implicitHeight: 26; implicitWidth: browseRow.implicitWidth + 14
                                radius: Theme.radiusSm
                                color: browseHover.hovered ? Theme.surface2 : "transparent"
                                border.color: Theme.border
                                Layout.alignment: Qt.AlignVCenter
                                RowLayout {
                                    id: browseRow; anchors.centerIn: parent; spacing: 4
                                    MaterialIcon { name: "folder_open"; size: 13; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
                                    Text {
                                        text: qsTr("Browse File"); color: Theme.text2
                                        font.pixelSize: Theme.fontSm; font.weight: Font.DemiBold
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                                HoverHandler { id: browseHover }
                                TapHandler { onTapped: { locationFileDialog.targetRow = -1; locationFileDialog.open() } }
                            }
                        }
                        // Drop hint — only while a drag is hovering over the tab.
                        Rectangle {
                            width: parent.width
                            height: 26
                            visible: locationDrop.containsDrag
                            radius: Theme.radiusSm
                            color: Theme.accentSoft
                            border.color: Theme.accent
                            Text {
                                anchors.centerIn: parent
                                text: qsTr("Drop files or web links to add them")
                                color: Theme.accent; font.pixelSize: Theme.fontSm; font.weight: Font.DemiBold
                            }
                        }
                    }
                    delegate: Card {
                                id: locCard
                                required property int index
                                required property var model
                                property bool expanded: false
                                width: ListView.view ? ListView.view.width : 0
                                implicitHeight: locCol.implicitHeight + 16
                                function _menu(sx, sy) {
                                    rowMenu.openFor(DesktopAppController.projectLocationsModel,
                                        (locCard.model.id || "").toString(), qsTr("Location"),
                                        (locCard.model.location_description || locCard.model.full_path || "").toString(),
                                        sx, sy)
                                }
                                // Icon that reflects the location's file type.
                                readonly property var _typeIcons: ({
                                    "File Folder": "folder",
                                    "Web Link": "link",
                                    "Microsoft Project": "view_timeline",
                                    "Word Document": "description",
                                    "Excel Document": "table_chart",
                                    "PowerPoint Document": "slideshow",
                                    "PDF File": "picture_as_pdf",
                                    "Generic File (System Identified)": "text_snippet"
                                })
                                readonly property string _typeIcon:
                                    _typeIcons[(locCard.model.location_type || "").toString()] || "text_snippet"
                                ColumnLayout {
                                    id: locCol
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 6

                                    // Summary row (click to expand/collapse the editor)
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6
                                        MaterialIcon { name: locCard._typeIcon; size: 14; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                                        Text {
                                            text: (locCard.model.location_description || locCard.model.full_path
                                                   || qsTr("(unnamed location)")).toString()
                                            color: Theme.text
                                            font.pixelSize: Theme.fontBody
                                            elide: Text.ElideRight
                                            horizontalAlignment: Text.AlignLeft
                                            Layout.fillWidth: true
                                        }
                                        // Browse for / re-point this row's file.
                                        RowIconBtn {
                                            Layout.alignment: Qt.AlignVCenter
                                            icon: "folder_open"
                                            onAct: { locationFileDialog.targetRow = locCard.index; locationFileDialog.open() }
                                        }
                                        // Open with the OS default handler.
                                        RowIconBtn {
                                            Layout.alignment: Qt.AlignVCenter
                                            icon: "open_in_new"; tint: Theme.accent
                                            enabled: (locCard.model.full_path || "").toString().length > 0
                                            onAct: DesktopAppController.openProjectLocation(locCard.index)
                                        }
                                        // Edit / collapse toggle
                                        Rectangle {
                                            implicitWidth: 24; implicitHeight: 24; radius: Theme.radiusSm
                                            color: locEHover.hovered ? Theme.surface2 : "transparent"
                                            Layout.alignment: Qt.AlignVCenter
                                            MaterialIcon {
                                                anchors.centerIn: parent
                                                name: locCard.expanded ? "expand_less" : "edit"
                                                size: 13; color: Theme.text2
                                            }
                                            HoverHandler { id: locEHover }
                                            TapHandler { onTapped: locCard.expanded = !locCard.expanded }
                                        }
                                        KebabButton {
                                            Layout.alignment: Qt.AlignVCenter
                                            onClicked: (sx, sy) => locCard._menu(sx, sy)
                                        }
                                        RowDelete {
                                            Layout.alignment: Qt.AlignVCenter
                                            onDel: { DesktopAppController.deleteProjectLocation(locCard.index); DesktopAppController.refreshProjectLocations() }
                                        }
                                    }

                                    // Inline editor — type, name/description, path.
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.leftMargin: 20
                                        visible: locCard.expanded
                                        spacing: 5
                                        ComboField {
                                            Layout.fillWidth: true
                                            label: qsTr("Type")
                                            options: DesktopAppController.fileTypeOptions()
                                            value: (locCard.model.location_type || "").toString()
                                            onActivated: (v) => DesktopAppController.saveProjectLocation(
                                                locCard.index, v, (locCard.model.location_description || "").toString(),
                                                (locCard.model.full_path || "").toString())
                                        }
                                        LocField {
                                            Layout.fillWidth: true
                                            icon: "label"
                                            fieldText: (locCard.model.location_description || "").toString()
                                            placeholder: qsTr("Name / description")
                                            onCommit: (t) => DesktopAppController.saveProjectLocation(
                                                locCard.index, (locCard.model.location_type || "").toString(),
                                                t, (locCard.model.full_path || "").toString())
                                        }
                                        LocField {
                                            Layout.fillWidth: true
                                            icon: "link"
                                            fieldText: (locCard.model.full_path || "").toString()
                                            placeholder: qsTr("File path or web link")
                                            onCommit: (t) => DesktopAppController.saveProjectLocation(
                                                locCard.index, (locCard.model.location_type || "").toString(),
                                                (locCard.model.location_description || "").toString(), t)
                                        }
                                    }
                                }
                                TapHandler {
                                    acceptedButtons: Qt.RightButton
                                    onTapped: (ev) => locCard._menu(ev.scenePosition.x, ev.scenePosition.y)
                                }
                    }
                }
            }

            // â”€â”€ 4: NOTES â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            Item {
                ListView {
                    id: notesRep
                    anchors.fill: parent
                    anchors.margins: 13
                    clip: true
                    spacing: 7
                    model: DesktopAppController.projectNotesModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Item { width: 1; height: 6 }
                    header: Item {
                        width: ListView.view ? ListView.view.width : 0
                        height: notesBar.implicitHeight + 8
                        SectionBar {
                            id: notesBar
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                            title: qsTr("Notes")
                            icon: "edit_note"
                            addLabel: qsTr("Add Note")
                            searchModel: DesktopAppController.projectNotesModel
                            filterSection: "notes"
                            filterActive: page._sectionFilterActive("notes")
                            sortActive: page._sectionSortActive("notes")
                            onAdd: {
                                page._saveNow()
                                var r = DesktopAppController.addProjectNote(page.projectId)
                                if (r < 0) return
                                page.noteActivated(r, DesktopAppController.projectNoteIdAtRow(r))
                            }
                            onFilter: page.subFilterRequested(filterSection)
                            onSort: (sx, sy) => page.subSortRequested(filterSection, sx, sy)
                        }
                    }
                    delegate: Card {
                            id: noteCard
                            required property int index
                            required property var model
                            width: ListView.view ? ListView.view.width : 0
                            implicitHeight: 48
                            color: nHover.hovered ? Theme.raise : Theme.surface
                            function _menu(sx, sy) {
                                rowMenu.openFor(DesktopAppController.projectNotesModel,
                                    (noteCard.model.id || "").toString(), qsTr("Note"),
                                    (noteCard.model.note_title || "").toString(), sx, sy,
                                    /*allowMoveTo*/ false)
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 11; anchors.rightMargin: 11
                                spacing: 9
                                MaterialIcon { name: "description"; size: 16; color: Theme.text3 }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Text {
                                        text: (noteCard.model.note_title || qsTr("(Untitled note)")).toString()
                                        color: Theme.text; font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold
                                        elide: Text.ElideRight; Layout.fillWidth: true
                                    }
                                    Text {
                                        text: (noteCard.model.note_date || "").toString()
                                        color: Theme.text3; font.pixelSize: Theme.fontSm
                                    }
                                }
                                KebabButton {
                                    implicitWidth: 22; implicitHeight: 22
                                    onClicked: (sx, sy) => noteCard._menu(sx, sy)
                                }
                                MaterialIcon { name: "chevron_right"; size: 17; color: Theme.text3 }
                            }
                            HoverHandler { id: nHover }
                            TapHandler {
                                onTapped: {
                                    page._saveNow()
                                    page.noteActivated(noteCard.index, (noteCard.model.id || "").toString())
                                }
                            }
                            TapHandler {
                                acceptedButtons: Qt.RightButton
                                onTapped: (ev) => noteCard._menu(ev.scenePosition.x, ev.scenePosition.y)
                            }
                    }
                }
            }
        }
    }

    // â”€â”€ Team member people picker â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    PeoplePickerDialog {
        id: teamPicker
        headingText: qsTr("Add Team Member")
        model: DesktopAppController.peopleList()
        onPicked: (person) => {
            var r = DesktopAppController.addTeamMember(page.projectId)
            if (r >= 0) {
                DesktopAppController.saveTeamMember(r, person.id, "", false)
                DesktopAppController.refreshTeamMembers()
                page._refreshTeamPeople()
            }
        }
    }

    // â”€â”€ Inline reusable pieces â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    // "LABEL value" metadata pair; hides itself when the value is empty.
    component MetaPair: RowLayout {
        id: mp
        property string label: ""
        property string value: ""
        property color valueColor: Theme.text2
        visible: value !== ""
        spacing: 3
        Text { text: mp.label; color: Theme.text3; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold }
        Text { text: mp.value; color: mp.valueColor; font.pixelSize: Theme.fontXs }
    }

    // Read-only calculated-financial tile.
    component MetricTile: Rectangle {
        property string label: ""
        property string value: "—"
        property color valueColor: Theme.text
        Layout.fillWidth: true
        implicitHeight: 44
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 11; anchors.rightMargin: 11
            anchors.topMargin: 6;   anchors.bottomMargin: 6
            spacing: 1
            Text {
                text: label; color: Theme.text3; font.pixelSize: Theme.fontXs
                elide: Text.ElideRight; Layout.fillWidth: true
            }
            Text {
                text: value; color: valueColor
                font.pixelSize: Theme.fontXl; font.weight: Font.DemiBold
                elide: Text.ElideRight; Layout.fillWidth: true
            }
        }
    }

    component TabItem: TabButton {
        id: tb
        property string iconName: ""
        property string label: ""
        property int count: 0
        implicitHeight: 36
        implicitWidth: tabRow.implicitWidth + 22
        clip: true
        background: Rectangle {
            color: tb.hovered && !tb.checked ? Theme.surface2 : "transparent"
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 2
                color: tb.checked ? Theme.accent : "transparent"
            }
        }
        contentItem: RowLayout {
            id: tabRow
            spacing: 5
            clip: true
            MaterialIcon {
                name: tb.iconName; size: 14
                color: tb.checked ? Theme.accent : Theme.text2
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: tb.label
                color: tb.checked ? Theme.accent : Theme.text2
                font.pixelSize: Theme.fontBody
                font.weight: tb.checked ? Font.DemiBold : Font.Normal
                verticalAlignment: Text.AlignVCenter
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                elide: Text.ElideRight
            }
            // Count badge next to the tab label.
            Rectangle {
                radius: 8
                color: Theme.surface2
                implicitHeight: 14
                implicitWidth: Math.max(16, cnt.implicitWidth + 10)
                Layout.alignment: Qt.AlignVCenter
                Text {
                    id: cnt
                    anchors.centerIn: parent
                    text: tb.count.toString()
                    color: Theme.text3
                    font.pixelSize: Theme.font2xs; font.weight: Font.DemiBold
                }
            }
        }
    }

    component SectionBar: RowLayout {
        id: bar
        property string title: ""
        property string icon: "folder"
        property string addLabel: qsTr("Add")
        // Opt-in quick search: when set, a search field appears that filters
        // this model live (mirrors TopBar's global search box).
        property var searchModel: null
        // Opt-in filter button: when non-empty, a Filter button appears and
        // emits filter() — the page wires this to the shared FilterDialog,
        // keyed by this section name (see FilterDialog.openFor()).
        property string filterSection: ""
        // True when filterSection's model has an active column filter —
        // highlights the Filter chip, mirroring TopBar's Filter button.
        property bool filterActive: false
        // True when filterSection's model has an active sort.
        property bool sortActive: false
        signal add()
        signal filter()
        // Carries the chip's own scene position (Overlay.overlay space) since
        // SortMenu is a shared instance living in Main.qml, outside this
        // pushed page's scope — an Item id can't cross that boundary, a
        // couple of numbers can (see SortMenu.qml's openFor doc comment).
        signal sort(real sx, real sy)

        // Empties the quick search field and drops the filter behind it. clear()
        // is programmatic, so the field's onTextEdited never fires for it — the
        // model has to be told here.
        function clearSearch() {
            if (subSearchField.text === "")
                return
            subSearchField.clear()
            if (bar.searchModel)
                DesktopAppController.setQuickSearch(bar.searchModel, "")
        }

        Layout.fillWidth: true
        Layout.topMargin: 6
        spacing: 6
        MaterialIcon { name: bar.icon; size: 16; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
        Text {
            text: bar.title; color: Theme.text; font.pixelSize: Theme.fontXl; font.weight: Font.Bold
            Layout.fillWidth: true; elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        Rectangle {
            visible: bar.searchModel !== null
            Layout.preferredWidth: 160
            implicitHeight: 26
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: subSearchField.activeFocus ? Theme.accent : Theme.border
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 7
                anchors.rightMargin: 5
                spacing: 5
                MaterialIcon { name: "search"; size: 13; color: Theme.text3 }
                TextField {
                    id: subSearchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search")
                    color: Theme.text
                    placeholderTextColor: Theme.text3
                    background: null
                    topPadding: 0; bottomPadding: 0
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontSm
                    onTextEdited: DesktopAppController.setQuickSearch(bar.searchModel, text)
                    // Esc clears the field while it has focus; with nothing to
                    // clear the key falls through to whoever else wants it.
                    Keys.onEscapePressed: (ev) => {
                        if (subSearchField.text === "") {
                            ev.accepted = false
                            return
                        }
                        bar.clearSearch()
                    }
                }
                // Clear button, same circle-X affordance as TopBar's search box —
                // only visible once there's something to clear.
                MaterialIcon {
                    name: "cancel"
                    size: 13
                    color: subClearHover.hovered ? Theme.text2 : Theme.text3
                    visible: subSearchField.text !== ""
                    Layout.alignment: Qt.AlignVCenter
                    HoverHandler { id: subClearHover }
                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds
                        onTapped: bar.clearSearch()
                    }
                    ToolTip.visible: subClearHover.hovered
                    ToolTip.text: qsTr("Clear Search")
                    ToolTip.delay: 400
                }
            }
        }
        Rectangle {
            visible: bar.filterSection !== ""
            implicitHeight: 26; implicitWidth: fRow.implicitWidth + 13
            radius: Theme.radiusSm
            color: bar.filterActive ? Theme.accentSoft : (fHover.hovered ? Theme.surface2 : Theme.surface)
            border.color: bar.filterActive ? Theme.accent : Theme.border
            Layout.alignment: Qt.AlignVCenter
            RowLayout {
                id: fRow; anchors.centerIn: parent; spacing: 4
                MaterialIcon { name: "filter_list"; size: 13; color: bar.filterActive ? Theme.accent : Theme.text2; Layout.alignment: Qt.AlignVCenter }
                Text {
                    text: qsTr("Filter"); color: bar.filterActive ? Theme.accent : Theme.text; font.pixelSize: Theme.fontSm
                    verticalAlignment: Text.AlignVCenter
                }
            }
            HoverHandler { id: fHover }
            TapHandler { onTapped: bar.filter() }
        }
        Rectangle {
            id: sortChip
            visible: bar.filterSection !== ""
            implicitHeight: 26; implicitWidth: sRow.implicitWidth + 13
            radius: Theme.radiusSm
            color: bar.sortActive ? Theme.accentSoft : (sHover.hovered ? Theme.surface2 : Theme.surface)
            border.color: bar.sortActive ? Theme.accent : Theme.border
            Layout.alignment: Qt.AlignVCenter
            RowLayout {
                id: sRow; anchors.centerIn: parent; spacing: 4
                MaterialIcon { name: "swap_vert"; size: 13; color: bar.sortActive ? Theme.accent : Theme.text2; Layout.alignment: Qt.AlignVCenter }
                Text {
                    text: qsTr("Sort"); color: bar.sortActive ? Theme.accent : Theme.text; font.pixelSize: Theme.fontSm
                    verticalAlignment: Text.AlignVCenter
                }
            }
            HoverHandler { id: sHover }
            TapHandler {
                onTapped: {
                    var p = sortChip.mapToItem(Overlay.overlay, 0, sortChip.height)
                    bar.sort(p.x, p.y + 4)
                }
            }
        }
        Rectangle {
            implicitHeight: 26; implicitWidth: aRow.implicitWidth + 14
            radius: Theme.radiusSm; color: aHover.hovered ? Theme.accentStrong : Theme.accent
            Layout.alignment: Qt.AlignVCenter
            RowLayout {
                id: aRow; anchors.centerIn: parent; spacing: 4
                MaterialIcon { name: "add"; size: 13; color: "#ffffff"; Layout.alignment: Qt.AlignVCenter }
                Text {
                    text: bar.addLabel; color: "#ffffff"; font.pixelSize: Theme.fontSm; font.weight: Font.DemiBold
                    verticalAlignment: Text.AlignVCenter
                }
            }
            HoverHandler { id: aHover }
            TapHandler { onTapped: bar.add() }
        }
    }

    // One shared record/plugin menu for every child list on this page (tracker,
    // notes, team, locations, status). Rows call rowMenu.openFor(model, id, …).
    RecordRowMenu {
        id: rowMenu
        onExportRecord: (table, id) => page.exportRequested(table, id)
        // Notes and tracker items get their own page, so a copy of one is worth
        // opening; locations and status items live in a list on this page, where
        // the copy simply shows up in place.
        onDuplicateRecord: (table, id) => {
            if (table === "project_notes") {
                var r = DesktopAppController.copyProjectNote(id)
                if (r >= 0) { page._saveNow(); page.noteActivated(r, DesktopAppController.projectNoteIdAtRow(r)) }
            } else if (table === "item_tracker") {
                var newId = DesktopAppController.copyTrackerItem(id)
                if (newId !== "") { page._saveNow(); page.itemActivated(newId) }
            } else {
                DesktopAppController.duplicateRecordInTable(table, id)
            }
        }
        onMoveToRecord: (id) => page.moveToRequested(id)
        onGoToPersonRequested: (personId) => page.goToPersonRequested(personId)
        onGoToClientRequested: (clientId) => page.goToClientRequested(clientId)
    }

    // The project's own record/plugin menu — opened by the title row's kebab
    // and by right-clicking the page background. Parity with the sidebar's
    // per-row Project menu (RecordContextMenu there too).
    RecordContextMenu {
        id: selfMenu
        recordType: qsTr("Project")
        model: DesktopAppController.projectsListModel
        recordId: page.projectId
        canOpen: false
        canMoveTo: true
        onNewRequested: page.newRequested()
        onDeleteRequested: page.deleteRequested()
        onDuplicateRequested: {
            // Copy what's on screen, not what was last written.
            page._saveNow()
            // ProjectsModel::copyRecord also brings the project's team across.
            var newId = DesktopAppController.duplicateRecord(
                            DesktopAppController.projectsListModel, page.projectId)
            if (newId !== "") page.projectActivated(newId)
        }
        onMoveToRequested: moveToFolderDialog.openFor(page.projectId, selfMenu.recordLabel)
        onExportRequested: page.exportRequested(page.exportTable, page.exportId)
        onFilterRequested: page.filterRequested()
        onSortRequested: (sx, sy) => page.sortRequested(sx, sy)
        onRefreshRequested: page._refreshAll()
    }

    // "Move To Folder" for the project itself, opened from selfMenu above.
    MoveToFolderDialog { id: moveToFolderDialog }

    // Shared full-field spell-check dialog (opened by fields / right-click).
    SpellCheckDialog { id: spellDialog }

    component RowDelete: Item {
        id: rd
        signal del()
        implicitWidth: 26; implicitHeight: 26
        Rectangle {
            anchors.centerIn: parent; width: 22; height: 22; radius: Theme.radiusSm
            color: rdHover.hovered ? Theme.redSoft : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: "close"; size: 13; color: Theme.red }
        }
        HoverHandler { id: rdHover }
        TapHandler { onTapped: rd.del() }
    }

    // One labelled inline text field inside a location card (name or path).
    component LocField: Rectangle {
        id: lf
        property string icon: "label"
        property string fieldText: ""
        property string placeholder: ""
        signal commit(string text)
        implicitHeight: 28
        radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.border
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 7; anchors.rightMargin: 7
            spacing: 5
            MaterialIcon { name: lf.icon; size: 13; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
            TextField {
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
                text: lf.fieldText
                placeholderText: lf.placeholder
                placeholderTextColor: Theme.text3
                color: Theme.text; background: null; font.pixelSize: Theme.fontBody
                onEditingFinished: lf.commit(text)
            }
        }
    }

    // Small square icon button for a row action (browse, open, …).
    component RowIconBtn: Item {
        id: rib
        property string icon: "open_in_new"
        property color tint: Theme.text2
        signal act()
        implicitWidth: 26; implicitHeight: 26
        opacity: enabled ? 1 : 0.35
        Rectangle {
            anchors.centerIn: parent; width: 22; height: 22; radius: Theme.radiusSm
            color: ribHover.hovered && rib.enabled ? Theme.surface2 : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: rib.icon; size: 14; color: rib.tint }
        }
        HoverHandler { id: ribHover; enabled: rib.enabled }
        TapHandler { enabled: rib.enabled; onTapped: rib.act() }
    }
}
