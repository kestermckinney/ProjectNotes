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

    // Overlay layer a dragged tracker item card reparents onto while dragging
    // (threaded down from Main.qml's dragOverlay, same as the sidebar's).
    property var    dragLayer: null

    // Consumed by the TopBar's Export XML action.
    readonly property string exportTable: "projects"
    readonly property string exportId: projectId

    // Consumed by the TopBar's Delete action (Main.deleteCurrent()).
    readonly property bool canDelete: true

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
    // Column filter for one of this page's sub-tab lists (Status Report,
    // Tracker, Team, Locations, Notes) — routed to Main's shared FilterDialog,
    // keyed by the tab's filterSection name (see FilterDialog.openFor()).
    signal subFilterRequested(string section)

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
        tabBar.currentIndex = DesktopAppController.lastProjectDetailTab(page.projectId)
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
    }

    function _saveNow() {
        if (!page._changed) return true
        var ok = DesktopAppController.saveProject(
            page.projectRow, numberField.text, nameField.text, statusCombo.value,
            page._contactId, page._clientId, statusDate.text, invoiceDate.text,
            invoicingCombo.value, reportCombo.value,
            budgetField.text, actualField.text, bcwpField.text, bcwsField.text, bacField.text)
        // reload to pick up recalculated EVM on success, or to revert the fields
        // to the last valid values when a rule rejected the edit.
        if (ok) page._changed = false
        _reload()
        return ok
    }

    // Refresh everything the page shows for this project: the core fields plus
    // every child list. Backs the page-level menu's Refresh action.
    function _refreshAll() {
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

        // ── Header (all project information, above the tabs) ──────────────────
        ScrollView {
            id: headerScroll
            Layout.fillWidth: true
            Layout.maximumHeight: page.height * 0.62
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: headerScroll.availableWidth
                spacing: 14

                // Title row: number · name · status (all editable inline)
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.topMargin: 18
                    spacing: 12
                    FormField {
                        id: numberField
                        label: qsTr("Project Number")
                        Layout.preferredWidth: 120
                        Layout.fillWidth: false
                        onEdited: page._changed = true
                    }
                    FormField {
                        id: nameField
                        label: qsTr("Project Name")
                        Layout.fillWidth: true
                        spellCheck: true
                        spellDialog: spellDialog
                        onEdited: page._changed = true
                    }
                    ComboField {
                        id: statusCombo
                        label: qsTr("Status")
                        Layout.preferredWidth: 170
                        Layout.fillWidth: false
                        options: DesktopAppController.projectStatusOptions()
                        onActivated: page._changed = true
                    }
                    // Page-level record menu — parity with the sidebar's per-row
                    // Project kebab (this project's own detail page previously had
                    // no equivalent quick-actions entry point).
                    KebabButton {
                        Layout.alignment: Qt.AlignBottom
                        Layout.bottomMargin: 6
                        onClicked: (sx, sy) => page._openSelfMenu(sx, sy)
                    }
                }

                // Remaining editable fields
                GridLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    columns: page.width > 940 ? 3 : 2
                    columnSpacing: 14
                    rowSpacing: 12
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
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    spacing: 10
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
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    spacing: 10
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

        // ── Tabs (with live count badges) ─────────────────────────────────────
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            background: Rectangle {
                color: "transparent"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
            }
            onCurrentIndexChanged: DesktopAppController.setLastProjectDetailTab(page.projectId, currentIndex)
            TabItem { iconName: "flag";        label: qsTr("Status Report"); count: statusRep.count }
            TabItem { iconName: "task_alt";    label: qsTr("Tracker");       count: trackerRep.count }
            TabItem { iconName: "groups";      label: qsTr("Team");          count: teamRep.count }
            TabItem { iconName: "folder";      label: qsTr("Locations");     count: locRep.count }
            TabItem { iconName: "description"; label: qsTr("Notes");         count: notesRep.count }
        }

        // ── Tab content ───────────────────────────────────────────────────────
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // ── 0: STATUS REPORT ITEMS ─────────────────────────────────────────
            // Virtualized: only visible rows exist; the tab badge reads
            // statusRep.count, which is the model's row count either way.
            // No reuseItems on these tabs — delegates hold edit-field state.
            Item {
                ListView {
                    id: statusRep
                    anchors.fill: parent
                    anchors.margins: 18
                    clip: true
                    spacing: 10
                    model: DesktopAppController.statusReportItemsModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Item { width: 1; height: 8 }
                    header: Item {
                        width: ListView.view ? ListView.view.width : 0
                        height: statusBar.implicitHeight + 10
                        SectionBar {
                            id: statusBar
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                            title: qsTr("Status Report Items")
                            icon: "flag"
                            addLabel: qsTr("Add Status Item")
                            searchModel: DesktopAppController.statusReportItemsModel
                            filterSection: "statusreport"
                            // addStatusItem appends a pending (unsaved) row; it is INSERTed
                            // only when a field is edited. Do NOT refresh here — refresh()
                            // re-queries the DB and would wipe the new row before it is seen.
                            onAdd: DesktopAppController.addStatusItem(page.projectId)
                            onFilter: page.subFilterRequested(filterSection)
                        }
                    }
                    delegate: Card {
                            id: stCard
                            required property int index
                            required property var model
                            width: ListView.view ? ListView.view.width : 0
                            implicitHeight: 50
                            function _menu(sx, sy) {
                                rowMenu.openFor(DesktopAppController.statusReportItemsModel,
                                    (stCard.model.id || "").toString(), qsTr("Status Item"),
                                    (stCard.model.task_description || "").toString(), sx, sy)
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12; anchors.rightMargin: 8
                                spacing: 8
                                ComboField {
                                    Layout.preferredWidth: 130
                                    Layout.fillWidth: false
                                    options: DesktopAppController.statusItemCategoryOptions()
                                    value: (stCard.model.task_category || "").toString()
                                    onActivated: (v) => DesktopAppController.saveStatusItem(
                                        stCard.index, v, (stCard.model.task_description || "").toString())
                                }
                                Rectangle {
                                    Layout.fillWidth: true; implicitHeight: 30
                                    radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.border
                                    TextField {
                                        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                                        verticalAlignment: Text.AlignVCenter
                                        text: (stCard.model.task_description || "").toString()
                                        placeholderText: qsTr("Description")
                                        placeholderTextColor: Theme.text3
                                        color: Theme.text; background: null; font.pixelSize: 13
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

            // ── 1: TRACKER ─────────────────────────────────────────────────────
            Item {
                ListView {
                    id: trackerRep
                    anchors.fill: parent
                    anchors.margins: 18
                    clip: true
                    spacing: 10
                    model: DesktopAppController.projectTrackerItemsModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Item { width: 1; height: 8 }
                    header: Item {
                        width: ListView.view ? ListView.view.width : 0
                        height: trackerBar.implicitHeight + 10
                        SectionBar {
                            id: trackerBar
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                            title: qsTr("Tracker Items")
                            icon: "task_alt"
                            addLabel: qsTr("Add Item")
                            searchModel: DesktopAppController.projectTrackerItemsModel
                            filterSection: "trackeritems"
                            onAdd: {
                                page._saveNow()
                                DesktopAppController.addTrackerItem(page.projectId)
                                var d = DesktopAppController.getTrackerItemDetailData(0)
                                if (d.id !== undefined) page.itemActivated(d.id.toString())
                            }
                            onFilter: page.subFilterRequested(filterSection)
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
                                    (trackerSlot.model.item_name || "").toString(), sx, sy, true, true)
                            }

                            Card {
                                id: trackerCard
                                width: trackerSlot.width
                                x: 0; y: 0
                                implicitHeight: tiCol.implicitHeight + 20
                                height: implicitHeight
                                color: dragArea.drag.active ? Theme.surface2
                                     : (tiHover.hovered ? Theme.raise : Theme.surface)

                                property string itemId: trackerSlot.iid

                                ColumnLayout {
                                    id: tiCol
                                    anchors.left: parent.left; anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.leftMargin: 12; anchors.rightMargin: 12
                                    spacing: 4
                                    // Title row: number · name · status
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 10
                                        Text {
                                            text: (trackerSlot.model.item_number || "").toString()
                                            color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                            Layout.preferredWidth: 60
                                            Layout.maximumWidth: 60
                                        }
                                        Text {
                                            text: (trackerSlot.model.item_name || qsTr("(unnamed)")).toString()
                                            color: Theme.text; font.pixelSize: 13; Layout.fillWidth: true; elide: Text.ElideRight
                                        }
                                        Rectangle {
                                            readonly property color c: page._statusColor((trackerSlot.model.status || "").toString())
                                            visible: (trackerSlot.model.status || "").toString() !== ""
                                            radius: 5
                                            color: Qt.rgba(c.r, c.g, c.b, 0.14)
                                            implicitHeight: 18; implicitWidth: tiStatus.implicitWidth + 14
                                            Layout.alignment: Qt.AlignVCenter
                                            Text {
                                                id: tiStatus; anchors.centerIn: parent
                                                text: (trackerSlot.model.status || "").toString()
                                                color: parent.c; font.pixelSize: 10; font.weight: Font.DemiBold
                                            }
                                        }
                                        KebabButton {
                                            implicitWidth: 24; implicitHeight: 24
                                            Layout.alignment: Qt.AlignVCenter
                                            onClicked: (sx, sy) => trackerSlot._menu(sx, sy)
                                        }
                                        MaterialIcon { name: "chevron_right"; size: 18; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                                    }
                                    // Metadata row: assigned · priority · due
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.leftMargin: 54
                                        spacing: 16
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

                                // ── Drag source: drag onto a project row in the
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

            // ── 2: TEAM ────────────────────────────────────────────────────────
            Item {
                ListView {
                    id: teamRep
                    anchors.fill: parent
                    anchors.margins: 18
                    clip: true
                    spacing: 10
                    model: DesktopAppController.projectTeamMembersModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Item { width: 1; height: 8 }
                    header: Item {
                        width: ListView.view ? ListView.view.width : 0
                        height: teamBar.implicitHeight + 10
                        SectionBar {
                            id: teamBar
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                            title: qsTr("Team")
                            icon: "groups"
                            addLabel: qsTr("Add Member")
                            searchModel: DesktopAppController.projectTeamMembersModel
                            filterSection: "team"
                            onAdd: { page._saveNow(); teamPicker.open() }
                            onFilter: page.subFilterRequested(filterSection)
                        }
                    }
                    delegate: Card {
                            id: teamCard
                            required property int index
                            required property var model
                            width: ListView.view ? ListView.view.width : 0
                            implicitHeight: 50
                            function _menu(sx, sy) {
                                rowMenu.openFor(DesktopAppController.projectTeamMembersModel,
                                    (teamCard.model.id || "").toString(), qsTr("Team Member"),
                                    (teamCard.model.name || "").toString(), sx, sy)
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12; anchors.rightMargin: 8
                                spacing: 10
                                MaterialIcon { name: "person"; size: 18; color: Theme.text3 }
                                Text {
                                    text: (teamCard.model.name || qsTr("(no name)")).toString()
                                    color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold
                                    Layout.preferredWidth: 150; elide: Text.ElideRight
                                }
                                Rectangle {
                                    Layout.fillWidth: true; implicitHeight: 30
                                    radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.border
                                    TextField {
                                        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                                        verticalAlignment: Text.AlignVCenter
                                        text: (teamCard.model.role || "").toString()
                                        placeholderText: qsTr("Role")
                                        placeholderTextColor: Theme.text3
                                        color: Theme.text; background: null; font.pixelSize: 13
                                        onEditingFinished: DesktopAppController.saveTeamMember(
                                            teamCard.index, (teamCard.model.people_id || "").toString(), text,
                                            (teamCard.model.receive_status_report || "0") !== "0")
                                        SpellCheckField { dialog: spellDialog }
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

            // ── 3: LOCATIONS ───────────────────────────────────────────────────
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
                    anchors.margins: 18
                    clip: true
                    spacing: 10
                    model: DesktopAppController.projectLocationsModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Item { width: 1; height: 8 }
                    header: Column {
                        width: ListView.view ? ListView.view.width : 0
                        spacing: 10
                        bottomPadding: 10
                        RowLayout {
                            width: parent.width
                            spacing: 8
                            SectionBar {
                                Layout.fillWidth: true
                                title: qsTr("Locations")
                                icon: "folder"
                                addLabel: qsTr("Add Location")
                                searchModel: DesktopAppController.projectLocationsModel
                                filterSection: "locations"
                                // Pending row is INSERTed on first edit; refresh() would wipe it.
                                onAdd: DesktopAppController.addProjectLocation(page.projectId)
                                onFilter: page.subFilterRequested(filterSection)
                            }
                            // Browse for a file and add it as a new location.
                            Rectangle {
                                implicitHeight: 28; implicitWidth: browseRow.implicitWidth + 18
                                radius: Theme.radiusSm
                                color: browseHover.hovered ? Theme.surface2 : "transparent"
                                border.color: Theme.border
                                Layout.alignment: Qt.AlignVCenter
                                RowLayout {
                                    id: browseRow; anchors.centerIn: parent; spacing: 5
                                    MaterialIcon { name: "folder_open"; size: 15; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
                                    Text {
                                        text: qsTr("Browse File"); color: Theme.text2
                                        font.pixelSize: 12; font.weight: Font.DemiBold
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
                            height: 30
                            visible: locationDrop.containsDrag
                            radius: Theme.radiusSm
                            color: Theme.accentSoft
                            border.color: Theme.accent
                            Text {
                                anchors.centerIn: parent
                                text: qsTr("Drop files or web links to add them")
                                color: Theme.accent; font.pixelSize: 12; font.weight: Font.DemiBold
                            }
                        }
                    }
                    delegate: Card {
                                id: locCard
                                required property int index
                                required property var model
                                property bool expanded: false
                                width: ListView.view ? ListView.view.width : 0
                                implicitHeight: locCol.implicitHeight + 20
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
                                    anchors.margins: 10
                                    spacing: 8

                                    // Summary row (click to expand/collapse the editor)
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        MaterialIcon { name: locCard._typeIcon; size: 16; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                                        Text {
                                            text: (locCard.model.location_description || locCard.model.full_path
                                                   || qsTr("(unnamed location)")).toString()
                                            color: Theme.text
                                            font.pixelSize: 13
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
                                            implicitWidth: 26; implicitHeight: 26; radius: 6
                                            color: locEHover.hovered ? Theme.surface2 : "transparent"
                                            Layout.alignment: Qt.AlignVCenter
                                            MaterialIcon {
                                                anchors.centerIn: parent
                                                name: locCard.expanded ? "expand_less" : "edit"
                                                size: 15; color: Theme.text2
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
                                        Layout.leftMargin: 24
                                        visible: locCard.expanded
                                        spacing: 6
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

            // ── 4: NOTES ───────────────────────────────────────────────────────
            Item {
                ListView {
                    id: notesRep
                    anchors.fill: parent
                    anchors.margins: 18
                    clip: true
                    spacing: 10
                    model: DesktopAppController.projectNotesModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Item { width: 1; height: 8 }
                    header: Item {
                        width: ListView.view ? ListView.view.width : 0
                        height: notesBar.implicitHeight + 10
                        SectionBar {
                            id: notesBar
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                            title: qsTr("Notes")
                            icon: "edit_note"
                            addLabel: qsTr("Add Note")
                            searchModel: DesktopAppController.projectNotesModel
                            filterSection: "notes"
                            onAdd: {
                                page._saveNow()
                                var r = DesktopAppController.addProjectNote(page.projectId)
                                if (r < 0) return
                                page.noteActivated(r, DesktopAppController.projectNoteIdAtRow(r))
                            }
                            onFilter: page.subFilterRequested(filterSection)
                        }
                    }
                    delegate: Card {
                            id: noteCard
                            required property int index
                            required property var model
                            width: ListView.view ? ListView.view.width : 0
                            implicitHeight: 58
                            color: nHover.hovered ? Theme.raise : Theme.surface
                            function _menu(sx, sy) {
                                rowMenu.openFor(DesktopAppController.projectNotesModel,
                                    (noteCard.model.id || "").toString(), qsTr("Note"),
                                    (noteCard.model.note_title || "").toString(), sx, sy)
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14; anchors.rightMargin: 14
                                spacing: 12
                                MaterialIcon { name: "description"; size: 18; color: Theme.text3 }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: (noteCard.model.note_title || qsTr("(Untitled note)")).toString()
                                        color: Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold
                                        elide: Text.ElideRight; Layout.fillWidth: true
                                    }
                                    Text {
                                        text: (noteCard.model.note_date || "").toString()
                                        color: Theme.text3; font.pixelSize: 12
                                    }
                                }
                                KebabButton {
                                    implicitWidth: 24; implicitHeight: 24
                                    onClicked: (sx, sy) => noteCard._menu(sx, sy)
                                }
                                MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3 }
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

    // ── Team member people picker ─────────────────────────────────────────────
    Dialog {
        id: teamPicker
        anchors.centerIn: parent
        width: 360; height: 420; modal: true; padding: 0
        scale: Theme.uiScale   // match the zoomed workspace (centered origin)
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        // Type-to-search text (lower-cased match target). Empty = show everyone.
        property string _filter: ""

        // Reset and focus the search box each time the picker opens.
        onOpened: { _filter = ""; teamSearch.text = ""; teamSearch.forceActiveFocus() }

        contentItem: ColumnLayout {
            spacing: 0
            RowLayout {
                Layout.fillWidth: true; Layout.margins: 14
                Text { text: qsTr("Add Team Member"); color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold; Layout.fillWidth: true }
                MaterialIcon { name: "close"; size: 20; color: Theme.text3; TapHandler { onTapped: teamPicker.close() } }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

            // Search field — filters the list below as you type.
            Rectangle {
                Layout.fillWidth: true
                Layout.margins: 12
                implicitHeight: 34
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: teamSearch.activeFocus ? Theme.accent : Theme.border
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10; anchors.rightMargin: 10
                    spacing: 6
                    MaterialIcon { name: "search"; size: 16; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                    TextField {
                        id: teamSearch
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search people…")
                        placeholderTextColor: Theme.text3
                        color: Theme.text
                        font.pixelSize: 13
                        background: null
                        verticalAlignment: Text.AlignVCenter
                        selectByMouse: true
                        onTextChanged: teamPicker._filter = text
                    }
                }
            }

            ListView {
                id: teamPeople
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                model: DesktopAppController.peopleList()
                delegate: ItemDelegate {
                    id: teamDelegate
                    required property int index
                    required property var modelData
                    // Collapse rows that don't contain the search text.
                    readonly property bool _match: teamPicker._filter === ""
                        || String(modelData.name).toLowerCase().indexOf(teamPicker._filter.toLowerCase()) >= 0
                    visible: _match
                    width: teamPeople.width; height: _match ? 40 : 0
                    contentItem: Text { text: modelData.name; color: Theme.text; font.pixelSize: 13; leftPadding: 14; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: teamDelegate.hovered ? Theme.surface2 : "transparent" }
                    onClicked: {
                        var r = DesktopAppController.addTeamMember(page.projectId)
                        if (r >= 0) {
                            DesktopAppController.saveTeamMember(r, modelData.id, "", false)
                            DesktopAppController.refreshTeamMembers()
                            page._refreshTeamPeople()
                        }
                        teamPicker.close()
                    }
                }
            }
        }
    }

    // ── Inline reusable pieces ────────────────────────────────────────────────

    // "LABEL value" metadata pair; hides itself when the value is empty.
    component MetaPair: RowLayout {
        id: mp
        property string label: ""
        property string value: ""
        property color valueColor: Theme.text2
        visible: value !== ""
        spacing: 4
        Text { text: mp.label; color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold }
        Text { text: mp.value; color: mp.valueColor; font.pixelSize: 11 }
    }

    // Read-only calculated-financial tile.
    component MetricTile: Rectangle {
        property string label: ""
        property string value: "—"
        property color valueColor: Theme.text
        Layout.fillWidth: true
        implicitHeight: 52
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 14; anchors.rightMargin: 14
            anchors.topMargin: 8;   anchors.bottomMargin: 8
            spacing: 2
            Text {
                text: label; color: Theme.text3; font.pixelSize: 11
                elide: Text.ElideRight; Layout.fillWidth: true
            }
            Text {
                text: value; color: valueColor
                font.pixelSize: 16; font.weight: Font.DemiBold
                elide: Text.ElideRight; Layout.fillWidth: true
            }
        }
    }

    component TabItem: TabButton {
        id: tb
        property string iconName: ""
        property string label: ""
        property int count: 0
        implicitHeight: 42
        implicitWidth: tabRow.implicitWidth + 28
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
            spacing: 6
            MaterialIcon {
                name: tb.iconName; size: 16
                color: tb.checked ? Theme.accent : Theme.text2
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: tb.label
                color: tb.checked ? Theme.accent : Theme.text2
                font.pixelSize: 13
                font.weight: tb.checked ? Font.DemiBold : Font.Normal
                verticalAlignment: Text.AlignVCenter
                Layout.alignment: Qt.AlignVCenter
            }
            // Count badge next to the tab label.
            Rectangle {
                radius: 9
                color: Theme.surface2
                implicitHeight: 16
                implicitWidth: Math.max(18, cnt.implicitWidth + 12)
                Layout.alignment: Qt.AlignVCenter
                Text {
                    id: cnt
                    anchors.centerIn: parent
                    text: tb.count.toString()
                    color: Theme.text3
                    font.pixelSize: 10; font.weight: Font.DemiBold
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
        signal add()
        signal filter()
        Layout.fillWidth: true
        Layout.topMargin: 8
        spacing: 8
        MaterialIcon { name: bar.icon; size: 18; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
        Text {
            text: bar.title; color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold
            Layout.fillWidth: true; elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        Rectangle {
            visible: bar.searchModel !== null
            Layout.preferredWidth: 180
            implicitHeight: 28
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: Theme.border
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 6
                spacing: 6
                MaterialIcon { name: "search"; size: 15; color: Theme.text3 }
                TextField {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search")
                    color: Theme.text
                    placeholderTextColor: Theme.text3
                    background: null
                    topPadding: 0; bottomPadding: 0
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 12
                    onTextEdited: DesktopAppController.setQuickSearch(bar.searchModel, text)
                }
            }
        }
        Rectangle {
            visible: bar.filterSection !== ""
            implicitHeight: 28; implicitWidth: fRow.implicitWidth + 16
            radius: Theme.radiusSm
            color: fHover.hovered ? Theme.surface2 : Theme.surface
            border.color: Theme.border
            Layout.alignment: Qt.AlignVCenter
            RowLayout {
                id: fRow; anchors.centerIn: parent; spacing: 5
                MaterialIcon { name: "filter_list"; size: 15; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
                Text {
                    text: qsTr("Filter"); color: Theme.text; font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                }
            }
            HoverHandler { id: fHover }
            TapHandler { onTapped: bar.filter() }
        }
        Rectangle {
            implicitHeight: 28; implicitWidth: aRow.implicitWidth + 18
            radius: Theme.radiusSm; color: aHover.hovered ? Theme.accentStrong : Theme.accent
            Layout.alignment: Qt.AlignVCenter
            RowLayout {
                id: aRow; anchors.centerIn: parent; spacing: 5
                MaterialIcon { name: "add"; size: 15; color: "#ffffff"; Layout.alignment: Qt.AlignVCenter }
                Text {
                    text: bar.addLabel; color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold
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
        onDuplicateRecord: (table, id) => {
            var newId = DesktopAppController.copyTrackerItem(id)
            if (newId !== "") { page._saveNow(); page.itemActivated(newId) }
        }
        onMoveToRecord: (id) => page.moveToRequested(id)
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
        canDuplicate: false
        canMoveTo: true
        onNewRequested: page.newRequested()
        onDeleteRequested: page.deleteRequested()
        onMoveToRequested: moveToFolderDialog.openFor(page.projectId, selfMenu.recordLabel)
        onExportRequested: page.exportRequested(page.exportTable, page.exportId)
        onFilterRequested: page.filterRequested()
        onRefreshRequested: page._refreshAll()
    }

    // "Move To Folder" for the project itself, opened from selfMenu above.
    MoveToFolderDialog { id: moveToFolderDialog }

    // Shared full-field spell-check dialog (opened by fields / right-click).
    SpellCheckDialog { id: spellDialog }

    component RowDelete: Item {
        id: rd
        signal del()
        implicitWidth: 30; implicitHeight: 30
        Rectangle {
            anchors.centerIn: parent; width: 26; height: 26; radius: 6
            color: rdHover.hovered ? Theme.redSoft : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: "close"; size: 15; color: Theme.red }
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
        implicitHeight: 30
        radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.border
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8; anchors.rightMargin: 8
            spacing: 6
            MaterialIcon { name: lf.icon; size: 15; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
            TextField {
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
                text: lf.fieldText
                placeholderText: lf.placeholder
                placeholderTextColor: Theme.text3
                color: Theme.text; background: null; font.pixelSize: 13
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
        implicitWidth: 30; implicitHeight: 30
        opacity: enabled ? 1 : 0.35
        Rectangle {
            anchors.centerIn: parent; width: 26; height: 26; radius: 6
            color: ribHover.hovered && rib.enabled ? Theme.surface2 : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: rib.icon; size: 16; color: rib.tint }
        }
        HoverHandler { id: ribHover; enabled: rib.enabled }
        TapHandler { enabled: rib.enabled; onTapped: rib.act() }
    }
}
