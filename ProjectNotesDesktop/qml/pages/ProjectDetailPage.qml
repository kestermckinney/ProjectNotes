// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
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

    // Consumed by the TopBar's Export XML action.
    readonly property string exportTable: "projects"
    readonly property string exportId: projectId

    // Client / contact id<->name mapping tables (built once).
    property var _clients: []
    property var _people: []

    // Calculated financial fields (display strings straight from the model).
    property string _budget: ""
    property string _actual: ""
    property string _bcwp: ""
    property string _bac: ""
    property string _eac: ""

    signal noteActivated(int noteRow, string noteId)
    signal itemActivated(string itemId)

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
    function _money(v) { var s = (v || "").toString().trim(); return s === "" ? "—" : s }

    Component.onCompleted: {
        _clients = DesktopAppController.clientList()
        _people  = DesktopAppController.peopleList()
        _reload()
        DesktopAppController.setProjectFilter(page.projectId)
        DesktopAppController.refreshProjectNotes()
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
        clientCombo.value  = _nameForId(page._clients, page._clientId)
        contactCombo.value = _nameForId(page._people, page._contactId)
        page._budget = (d.budget || "").toString()
        page._actual = (d.actual || "").toString()
        page._bcwp   = (d.bcwp || "").toString()
        page._bac    = (d.bac || "").toString()
        page._eac    = (d.eac || "").toString()
        page._changed = false
    }

    function _saveNow() {
        if (!page._changed) return true
        var ok = DesktopAppController.saveProject(
            page.projectRow, numberField.text, nameField.text, statusCombo.value,
            page._contactId, page._clientId, statusDate.text, invoiceDate.text,
            invoicingCombo.value, reportCombo.value)
        if (ok) { page._changed = false; _reload() }   // reload to pick up recalculated EVM
        return ok
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
                        onActivated: (v) => { page._clientId = page._idForName(page._clients, v); page._changed = true }
                    }
                    ComboField {
                        id: contactCombo
                        label: qsTr("Primary Contact")
                        options: page._peopleNames()
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

                // Calculated financial (EVM) tiles — gated by the "show internal /
                // budget items" view option, like the Widgets app.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    spacing: 10
                    visible: DesktopAppController.showInternalItems
                    MetricTile { label: qsTr("Budget");        value: page._money(page._budget) }
                    MetricTile { label: qsTr("Actual (ACWP)"); value: page._money(page._actual) }
                    MetricTile { label: qsTr("BCWP"); value: page._money(page._bcwp); valueColor: Theme.green }
                    MetricTile { label: qsTr("BAC / EAC"); value: page._money(page._eac !== "" ? page._eac : page._bac); valueColor: Theme.amber }
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
            ScrollView {
                id: statusScroll
                clip: true
                padding: 18
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ColumnLayout {
                    width: statusScroll.availableWidth
                    spacing: 10
                    SectionBar {
                        title: qsTr("Status Report Items")
                        icon: "flag"
                        addLabel: qsTr("Add Status Item")
                        onAdd: { DesktopAppController.addStatusItem(page.projectId); DesktopAppController.refreshStatusItems() }
                    }
                    Repeater {
                        id: statusRep
                        model: DesktopAppController.statusReportItemsModel
                        delegate: Card {
                            required property int index
                            required property var model
                            Layout.fillWidth: true
                            implicitHeight: 50
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12; anchors.rightMargin: 8
                                spacing: 8
                                ComboField {
                                    Layout.preferredWidth: 130
                                    Layout.fillWidth: false
                                    options: DesktopAppController.statusItemCategoryOptions()
                                    value: (model.task_category || "").toString()
                                    onActivated: (v) => DesktopAppController.saveStatusItem(
                                        index, v, (model.task_description || "").toString())
                                }
                                Rectangle {
                                    Layout.fillWidth: true; implicitHeight: 30
                                    radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.border
                                    TextField {
                                        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                                        verticalAlignment: Text.AlignVCenter
                                        text: (model.task_description || "").toString()
                                        placeholderText: qsTr("Description")
                                        placeholderTextColor: Theme.text3
                                        color: Theme.text; background: null; font.pixelSize: 13
                                        onEditingFinished: DesktopAppController.saveStatusItem(
                                            index, (model.task_category || "").toString(), text)
                                    }
                                }
                                RowDelete { onDel: { DesktopAppController.deleteStatusItem(index); DesktopAppController.refreshStatusItems() } }
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 8 }
                }
            }

            // ── 1: TRACKER ─────────────────────────────────────────────────────
            ScrollView {
                id: trackerScroll
                clip: true
                padding: 18
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ColumnLayout {
                    width: trackerScroll.availableWidth
                    spacing: 10
                    SectionBar {
                        title: qsTr("Tracker Items")
                        icon: "task_alt"
                        addLabel: qsTr("Add Item")
                        onAdd: {
                            page._saveNow()
                            DesktopAppController.addTrackerItem(page.projectId)
                            var d = DesktopAppController.getTrackerItemDetailData(0)
                            if (d.id !== undefined) page.itemActivated(d.id.toString())
                        }
                    }
                    Repeater {
                        id: trackerRep
                        model: DesktopAppController.projectTrackerItemsModel
                        delegate: Card {
                            required property int index
                            required property var model
                            readonly property string iid: model.id !== undefined ? model.id : ""
                            Layout.fillWidth: true
                            implicitHeight: 48
                            color: tiHover.hovered ? Theme.raise : Theme.surface
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12; anchors.rightMargin: 12
                                spacing: 10
                                Text {
                                    text: (model.item_number || "").toString()
                                    color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold
                                    Layout.preferredWidth: 44
                                }
                                Text {
                                    text: (model.item_name || qsTr("(unnamed)")).toString()
                                    color: Theme.text; font.pixelSize: 13; Layout.fillWidth: true; elide: Text.ElideRight
                                }
                                Text {
                                    text: (model.status || "").toString()
                                    color: Theme.text3; font.pixelSize: 11
                                    elide: Text.ElideRight
                                    Layout.maximumWidth: 110
                                }
                                MaterialIcon { name: "chevron_right"; size: 18; color: Theme.text3 }
                            }
                            HoverHandler { id: tiHover }
                            TapHandler { onTapped: { page._saveNow(); page.itemActivated(iid) } }
                        }
                    }
                    Item { Layout.preferredHeight: 8 }
                }
            }

            // ── 2: TEAM ────────────────────────────────────────────────────────
            ScrollView {
                id: teamScroll
                clip: true
                padding: 18
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ColumnLayout {
                    width: teamScroll.availableWidth
                    spacing: 10
                    SectionBar {
                        title: qsTr("Team")
                        icon: "groups"
                        addLabel: qsTr("Add Member")
                        onAdd: { page._saveNow(); teamPicker.open() }
                    }
                    Repeater {
                        id: teamRep
                        model: DesktopAppController.projectTeamMembersModel
                        delegate: Card {
                            required property int index
                            required property var model
                            Layout.fillWidth: true
                            implicitHeight: 50
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12; anchors.rightMargin: 8
                                spacing: 10
                                MaterialIcon { name: "person"; size: 18; color: Theme.text3 }
                                Text {
                                    text: (model.name || qsTr("(no name)")).toString()
                                    color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold
                                    Layout.preferredWidth: 150; elide: Text.ElideRight
                                }
                                Rectangle {
                                    Layout.fillWidth: true; implicitHeight: 30
                                    radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.border
                                    TextField {
                                        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                                        verticalAlignment: Text.AlignVCenter
                                        text: (model.role || "").toString()
                                        placeholderText: qsTr("Role")
                                        placeholderTextColor: Theme.text3
                                        color: Theme.text; background: null; font.pixelSize: 13
                                        onEditingFinished: DesktopAppController.saveTeamMember(
                                            index, (model.people_id || "").toString(), text,
                                            (model.receive_status_report || "0") !== "0")
                                    }
                                }
                                RowDelete { onDel: { DesktopAppController.deleteTeamMember(index); DesktopAppController.refreshTeamMembers() } }
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 8 }
                }
            }

            // ── 3: LOCATIONS ───────────────────────────────────────────────────
            ScrollView {
                id: locationsScroll
                clip: true
                padding: 18
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ColumnLayout {
                    width: locationsScroll.availableWidth
                    spacing: 10
                    SectionBar {
                        title: qsTr("Locations")
                        icon: "folder"
                        addLabel: qsTr("Add Location")
                        onAdd: { DesktopAppController.addProjectLocation(page.projectId); DesktopAppController.refreshProjectLocations() }
                    }
                    Repeater {
                        id: locRep
                        model: DesktopAppController.projectLocationsModel
                        delegate: Card {
                            required property int index
                            required property var model
                            Layout.fillWidth: true
                            implicitHeight: 50
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12; anchors.rightMargin: 8
                                spacing: 8
                                ComboField {
                                    Layout.preferredWidth: 120
                                    Layout.fillWidth: false
                                    options: DesktopAppController.fileTypeOptions()
                                    value: (model.location_type || "").toString()
                                    onActivated: (v) => DesktopAppController.saveProjectLocation(
                                        index, v, (model.location_description || "").toString(),
                                        (model.full_path || "").toString())
                                }
                                Rectangle {
                                    Layout.fillWidth: true; implicitHeight: 30
                                    radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.border
                                    TextField {
                                        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                                        verticalAlignment: Text.AlignVCenter
                                        text: (model.full_path || "").toString()
                                        placeholderText: qsTr("Path or description")
                                        placeholderTextColor: Theme.text3
                                        color: Theme.text; background: null; font.pixelSize: 13
                                        onEditingFinished: DesktopAppController.saveProjectLocation(
                                            index, (model.location_type || "").toString(),
                                            (model.location_description || "").toString(), text)
                                    }
                                }
                                RowDelete { onDel: { DesktopAppController.deleteProjectLocation(index); DesktopAppController.refreshProjectLocations() } }
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 8 }
                }
            }

            // ── 4: NOTES ───────────────────────────────────────────────────────
            ScrollView {
                id: notesScroll
                clip: true
                padding: 18
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ColumnLayout {
                    width: notesScroll.availableWidth
                    spacing: 10
                    SectionBar {
                        title: qsTr("Notes")
                        icon: "edit_note"
                        addLabel: qsTr("Add Note")
                        onAdd: {
                            page._saveNow()
                            var r = DesktopAppController.addProjectNote(page.projectId)
                            if (r < 0) return
                            page.noteActivated(r, DesktopAppController.projectNoteIdAtRow(r))
                        }
                    }
                    Repeater {
                        id: notesRep
                        model: DesktopAppController.projectNotesModel
                        delegate: Card {
                            id: noteCard
                            required property int index
                            required property var model
                            Layout.fillWidth: true
                            implicitHeight: 58
                            color: nHover.hovered ? Theme.raise : Theme.surface
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
                                MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3 }
                            }
                            HoverHandler { id: nHover }
                            TapHandler {
                                onTapped: {
                                    page._saveNow()
                                    page.noteActivated(noteCard.index, (noteCard.model.id || "").toString())
                                }
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 8 }
                }
            }
        }
    }

    // ── Team member people picker ─────────────────────────────────────────────
    Dialog {
        id: teamPicker
        anchors.centerIn: parent
        width: 360; height: 420; modal: true; padding: 0
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
        contentItem: ColumnLayout {
            spacing: 0
            RowLayout {
                Layout.fillWidth: true; Layout.margins: 14
                Text { text: qsTr("Add Team Member"); color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold; Layout.fillWidth: true }
                MaterialIcon { name: "close"; size: 20; color: Theme.text3; TapHandler { onTapped: teamPicker.close() } }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
            ListView {
                id: teamPeople
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                model: DesktopAppController.peopleList()
                delegate: ItemDelegate {
                    required property int index
                    required property var modelData
                    width: teamPeople.width; height: 40
                    contentItem: Text { text: modelData.name; color: Theme.text; font.pixelSize: 13; leftPadding: 14; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: hovered ? Theme.surface2 : "transparent" }
                    onClicked: {
                        var r = DesktopAppController.addTeamMember(page.projectId)
                        if (r >= 0) {
                            DesktopAppController.saveTeamMember(r, modelData.id, "", false)
                            DesktopAppController.refreshTeamMembers()
                        }
                        teamPicker.close()
                    }
                }
            }
        }
    }

    // ── Inline reusable pieces ────────────────────────────────────────────────

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
        signal add()
        Layout.fillWidth: true
        Layout.topMargin: 8
        MaterialIcon { name: bar.icon; size: 18; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
        Text {
            text: bar.title; color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold
            Layout.fillWidth: true; elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
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
}
