// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Tracker item detail (risk / issue / action item): full field editing plus a
// comments/updates list. The item is opened via openTrackerItem(itemId), which
// filters the detail model so the record is always at row 0.
Item {
    id: page
    property string itemId: ""
    property string projectId: ""
    property bool   _changed: false
    property var    _people: []
    readonly property string exportTable: "item_tracker"
    readonly property string exportId: itemId

    // Consumed by the TopBar's Delete action (Main.deleteCurrent()).
    readonly property bool canDelete: true

    // Routed to Main's confirm-delete flow (root.confirmDelete()) — fired by
    // the page-level kebab/right-click menu below. The TopBar's own Delete
    // button instead calls _deleteRecord() directly via canDelete/
    // Main.deleteCurrent() — same split as ProjectDetailPage/ProjectNoteDetailPage.
    signal deleteRequested()

    // Delete this tracker item. The detail model always holds the item at row 0.
    // The project Tracker tab and the master Items list query item_tracker through
    // separate models, so refresh them explicitly once the record is gone.
    function _deleteRecord() {
        if (!DesktopAppController.deleteTrackerItemDetail(0)) return false
        DesktopAppController.refreshModel(DesktopAppController.projectTrackerItemsModel)
        DesktopAppController.refreshAllItems()
        return true
    }

    function _peopleNames() { return _people.map(function(p){ return p.name }) }
    function _idForName(n){ for (var i=0;i<_people.length;i++) if (_people[i].name===n) return _people[i].id; return "" }
    function _nameForId(id){ for (var i=0;i<_people.length;i++) if (_people[i].id===id) return _people[i].name; return "" }

    property string _identifiedBy: ""
    property string _assignedTo: ""

    Component.onCompleted: {
        if (page.itemId !== "")
            DesktopAppController.openTrackerItem(page.itemId)
        _reload()
    }

    function _reload() {
        var d = DesktopAppController.getTrackerItemDetailData(0)
        page.itemId  = (d.id || page.itemId).toString()
        page.projectId = (d.project_id || "").toString()
        numberField.text = (d.item_number || "").toString()
        typeCombo.value  = (d.item_type || "").toString()
        nameField.text   = (d.item_name || "").toString()
        descArea.text   = (d.description || "").toString()
        priorityCombo.value = (d.priority || "").toString()
        statusCombo.value   = (d.status || "").toString()
        idDate.text  = (d.date_identified || "").toString()
        dueDate.text = (d.date_due || "").toString()
        internalCheck.checked = (d.internal_item || "0") !== "0"
        page._identifiedBy = (d.identified_by || "").toString()
        page._assignedTo   = (d.assigned_to || "").toString()
        // Assigned To / Identified By list only this project's team members (plus
        // whoever is already assigned, so an existing value keeps displaying).
        page._people = DesktopAppController.teamMemberList(page.projectId,
                            [page._identifiedBy, page._assignedTo])
        identifiedCombo.value = _nameForId(page._identifiedBy)
        assignedCombo.value   = _nameForId(page._assignedTo)
        page._changed = false
    }

    function _saveNow() {
        if (!page._changed) return true
        var ok = DesktopAppController.saveTrackerItemDetail(0, page.itemId,
            numberField.text, typeCombo.value, nameField.text, descArea.text,
            page._identifiedBy, page._assignedTo, priorityCombo.value, statusCombo.value,
            idDate.text, dueDate.text, internalCheck.checked)
        // Reload either way: on success to reflect data-layer side effects
        // (assigning a person auto-advances a "New" item to "Assigned", resolving
        // one sets the resolved date — see TrackerItemsModel::setData); on failure
        // to snap the fields back to the last valid values (a rejected edit).
        _reload()
        return ok
    }

    // Open the item's own record/plugin menu (kebab or right-click) at scene
    // coords — parity with ProjectDetailPage/ProjectNoteDetailPage's
    // _openSelfMenu().
    function _openSelfMenu(sx, sy) {
        selfMenu.recordLabel = ((numberField.text || "") + " " + (nameField.text || "")).trim()
        selfMenu.openAt(sx, sy)
    }

    // Right-click anywhere on the page background opens the item menu.
    // Declared beneath the page content so field/button clicks still reach
    // their own handlers first — see ProjectDetailPage's identical pattern.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: (mouse) => page._openSelfMenu(mouse.x, mouse.y)
    }

    ScrollView {
        id: pageScroll
        anchors.fill: parent
        anchors.margins: 13
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: pageScroll.availableWidth
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Rectangle {
                    implicitHeight: 28; implicitWidth: moveRow.implicitWidth + 18
                    radius: Theme.radiusSm
                    color: moveHover.hovered ? Theme.surface2 : Theme.surface
                    border.color: Theme.border
                    RowLayout {
                        id: moveRow; anchors.centerIn: parent; spacing: 5
                        MaterialIcon { name: "drive_file_move"; size: 15; color: Theme.text2 }
                        Text { text: qsTr("Move To…"); color: Theme.text2; font.pixelSize: 12; font.weight: Font.DemiBold }
                    }
                    HoverHandler { id: moveHover }
                    TapHandler { onTapped: page.moveToRequested(page.itemId) }
                }
                // Page-level record menu — exposes plugins whose dataexport is
                // "item_tracker", plus Delete/Export/Refresh. Parity with
                // ProjectDetailPage's kebab.
                KebabButton {
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: (sx, sy) => page._openSelfMenu(sx, sy)
                }
            }

            GridLayout {
                Layout.fillWidth: true; columns: 2; columnSpacing: 10; rowSpacing: 9
                FormField { label: qsTr("Item Number"); id: numberField; onEdited: page._changed = true }
                ComboField {
                    label: qsTr("Type"); id: typeCombo
                    options: DesktopAppController.itemTypeOptions()
                    onActivated: page._changed = true
                }
                FormField {
                    label: qsTr("Item Name"); id: nameField
                    Layout.columnSpan: 2
                    spellCheck: true
                    spellDialog: spellDialog
                    onEdited: page._changed = true
                }
            }

            Text { text: qsTr("Description"); color: Theme.text3; font.pixelSize: 10; font.weight: Font.DemiBold }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(76, descArea.contentHeight + 16)
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: descArea.activeFocus ? Theme.accent : Theme.border
                TextArea {
                    id: descArea
                    anchors.fill: parent
                    anchors.margins: 7
                    color: Theme.text
                    wrapMode: TextEdit.WordWrap
                    selectByMouse: true
                    background: null
                    font.pixelSize: 12
                    onTextChanged: page._changed = true
                    SpellCheckField { dialog: spellDialog }
                }
            }

            GridLayout {
                Layout.fillWidth: true; columns: 2; columnSpacing: 10; rowSpacing: 9
                ComboField {
                    label: qsTr("Identified By"); id: identifiedCombo
                    options: page._peopleNames()
                    includeNone: true
                    searchable: true
                    onActivated: (v) => { page._identifiedBy = page._idForName(v); page._changed = true }
                }
                ComboField {
                    label: qsTr("Assigned To"); id: assignedCombo
                    options: page._peopleNames()
                    includeNone: true
                    searchable: true
                    onActivated: (v) => { page._assignedTo = page._idForName(v); page._changed = true }
                }
                ComboField {
                    label: qsTr("Priority"); id: priorityCombo
                    options: DesktopAppController.itemPriorityOptions()
                    onActivated: page._changed = true
                }
                ComboField {
                    label: qsTr("Status"); id: statusCombo
                    options: DesktopAppController.itemStatusOptions()
                    onActivated: page._changed = true
                }
                DateField { label: qsTr("Date Identified"); id: idDate; onEdited: page._changed = true }
                DateField { label: qsTr("Date Due"); id: dueDate; onEdited: page._changed = true }
            }

            CheckBox {
                id: internalCheck
                onToggled: page._changed = true
                indicator: Rectangle {
                    implicitWidth: 16; implicitHeight: 16; radius: 4
                    x: internalCheck.leftPadding; y: parent.height/2 - height/2
                    color: internalCheck.checked ? Theme.accent : Theme.surface
                    border.color: internalCheck.checked ? Theme.accent : Theme.border
                    MaterialIcon { anchors.centerIn: parent; visible: internalCheck.checked; name: "check"; size: 12; color: "#ffffff" }
                }
                contentItem: Text {
                    text: qsTr("Internal item"); color: Theme.text; font.pixelSize: 12
                    leftPadding: internalCheck.indicator.width + 7; verticalAlignment: Text.AlignVCenter
                }
            }

            // Comments / updates
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 5
                MaterialIcon { name: "forum"; size: 16; color: Theme.text2 }
                Text {
                    text: qsTr("Comments"); color: Theme.text
                    font.pixelSize: 14; font.weight: Font.Bold; Layout.fillWidth: true
                }
                Rectangle {
                    implicitHeight: 26; implicitWidth: cRow.implicitWidth + 14
                    radius: Theme.radiusSm
                    color: cHover.hovered ? Theme.accentStrong : Theme.accent
                    RowLayout {
                        id: cRow; anchors.centerIn: parent; spacing: 4
                        MaterialIcon { name: "add"; size: 13; color: "#ffffff" }
                        Text { text: qsTr("Add Comment"); color: "#ffffff"; font.pixelSize: 11; font.weight: Font.DemiBold }
                    }
                    HoverHandler { id: cHover }
                    TapHandler {
                        onTapped: {
                            page._saveNow()
                            // The tap doesn't steal keyboard focus on its own, so a
                            // comment note still mid-edit would never fire its
                            // onEditingFinished and its text would be lost when the
                            // refresh below reloads the list from the database.
                            // Force focus off it first so it commits via _saveComment().
                            page.forceActiveFocus()
                            DesktopAppController.addComment(page.itemId)
                            DesktopAppController.refreshTrackerComments()
                        }
                    }
                }
            }

            Repeater {
                model: DesktopAppController.trackerItemCommentsModel
                delegate: Card {
                    id: cCard
                    required property int index
                    required property var model
                    Layout.fillWidth: true
                    implicitHeight: cCol.implicitHeight + 16

                    // Persist the row from whatever the three editors currently hold.
                    // On failure (a rule violation) reload the list so the editors
                    // snap back to the last valid values from the model.
                    function _saveComment() {
                        var ok = DesktopAppController.saveComment(cCard.index,
                            dateFld.text, noteArea.text, page._idForName(byCombo.value))
                        if (!ok)
                            DesktopAppController.refreshTrackerComments()
                    }
                    function _menu(sx, sy) {
                        rowMenu.openFor(DesktopAppController.trackerItemCommentsModel,
                            (cCard.model.id || "").toString(), qsTr("Comment"),
                            (cCard.model.update_note || "").toString(), sx, sy)
                    }

                    ColumnLayout {
                        id: cCol
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 5
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            DateField {
                                id: dateFld
                                label: qsTr("Date Updated")
                                text: (cCard.model.lastupdated_date || "").toString()
                                Layout.preferredWidth: 130
                                onEdited: cCard._saveComment()
                            }
                            ComboField {
                                id: byCombo
                                label: qsTr("Updated By")
                                Layout.fillWidth: true
                                options: page._peopleNames()
                                includeNone: true
                                searchable: true
                                value: page._nameForId((cCard.model.updated_by || "").toString())
                                onActivated: cCard._saveComment()
                            }
                            KebabButton {
                                implicitWidth: 20; implicitHeight: 20
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: 5
                                onClicked: (sx, sy) => cCard._menu(sx, sy)
                            }
                            Rectangle {
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: 5
                                width: 20; height: 20; radius: Theme.radiusSm
                                color: dHover.hovered ? Theme.redSoft : "transparent"
                                MaterialIcon { anchors.centerIn: parent; name: "close"; size: 12; color: Theme.red }
                                HoverHandler { id: dHover }
                                TapHandler {
                                    onTapped: {
                                        // Same unsaved-edit race as Add Comment above: flush
                                        // any other comment still mid-edit before the reload.
                                        page.forceActiveFocus()
                                        DesktopAppController.deleteComment(cCard.index)
                                        DesktopAppController.refreshTrackerComments()
                                    }
                                }
                            }
                        }
                        // Editable comment text
                        TextArea {
                            id: noteArea
                            Layout.fillWidth: true
                            text: (cCard.model.update_note || "").toString()
                            color: Theme.text
                            wrapMode: TextEdit.WordWrap
                            selectByMouse: true
                            background: null
                            font.pixelSize: 12
                            SpellCheckField { dialog: spellDialog }
                            onEditingFinished: cCard._saveComment()
                        }
                    }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (ev) => cCard._menu(ev.scenePosition.x, ev.scenePosition.y)
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }
        }
    }

    // Routed to Main.exportRecord — fired by a comment row's menu exporting
    // XML, and by the item's own selfMenu below.
    signal exportRequested(string table, string id)
    // Routed to Main → MoveToDialog when "Move To…" is chosen — fired by the
    // explicit button above and by selfMenu's Move To… entry.
    signal moveToRequested(string itemId)
    // Duplicate on selfMenu opens the copy — routed through Main so it gets a
    // breadcrumb and a history entry like any other item.
    signal itemActivated(string itemId)

    // Shared record/plugin menu for the comments list.
    RecordRowMenu {
        id: rowMenu
        onExportRecord: (table, id) => page.exportRequested(table, id)
        // A copied comment stays on this item; it just appears in the list.
        onDuplicateRecord: (table, id) => DesktopAppController.duplicateRecordInTable(table, id)
    }

    // The item's own record/plugin menu — opened by the title row's kebab and
    // by right-clicking the page background. Lists plugins whose dataexport is
    // "item_tracker" below the built-in actions. Parity with
    // ProjectDetailPage/ProjectNoteDetailPage's selfMenu; New/Filter aren't
    // offered — there's no page-level equivalent action for a single open item.
    RecordContextMenu {
        id: selfMenu
        recordType: qsTr("Item")
        model: DesktopAppController.trackerItemDetailModel
        recordId: page.itemId
        canOpen: false
        canNew: false
        canMoveTo: true
        canFilter: false
        onDeleteRequested: page.deleteRequested()
        onDuplicateRequested: {
            // Copy what's on screen, not what was last written.
            page._saveNow()
            var newId = DesktopAppController.copyTrackerItem(page.itemId)
            if (newId !== "") page.itemActivated(newId)
        }
        onExportRequested: page.exportRequested(page.exportTable, page.exportId)
        onRefreshRequested: page._reload()
        onMoveToRequested: (id) => page.moveToRequested(id)
    }

    // Shared full-field spell-check dialog (opened by fields / right-click).
    SpellCheckDialog { id: spellDialog }
}
