// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Person detail — editable fields, saved on navigate-away.
Item {
    id: page
    property int    personRow: -1
    property string personId: ""
    property bool   _changed: false
    readonly property string exportTable: "people"
    readonly property string exportId: personId
    property var    _clients: []
    property string _clientId: ""

    // Navigation signal
    signal goToClientRequested(string clientId)
    // Duplicate opens the copy — routed through Main so it gets a breadcrumb and
    // a history entry like any other person opened from the list.
    signal personActivated(int row, string personId)

    function _clientNames() { return _clients.map(function(c){ return c.name }) }
    function _idForName(n) { for (var i=0;i<_clients.length;i++) if (_clients[i].name===n) return _clients[i].id; return "" }
    function _nameForId(id){ for (var i=0;i<_clients.length;i++) if (_clients[i].id===id) return _clients[i].name; return "" }

    Component.onCompleted: {
        _clients = DesktopAppController.clientList()
        _reload()
    }

    function _reload() {
        var d = DesktopAppController.getPersonData(page.personRow)
        nameField.text  = (d.name || "").toString()
        emailField.text = (d.email || "").toString()
        officeField.text = (d.office_phone || "").toString()
        cellField.text  = (d.cell_phone || "").toString()
        roleField.text  = (d.role || "").toString()
        page._clientId  = (d.client_id || "").toString()
        clientCombo.value = _nameForId(page._clientId)
        page._changed = false
    }

    function _saveNow() {
        if (!page._changed) return true
        var ok = DesktopAppController.savePerson(page.personRow, nameField.text, emailField.text,
                    officeField.text, cellField.text, page._clientId, roleField.text)
        if (ok) page._changed = false
        else _reload()   // revert fields to last valid values when an edit is rejected
        return ok
    }

    function _openSelfMenu(sx, sy) {
        selfMenu.recordLabel = (nameField.text || "").toString()
        selfMenu.clientId = page._clientId
        selfMenu.openAt(sx, sy)
    }

    ScrollView {
        id: pageScroll
        anchors.fill: parent
        anchors.margins: 14
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: pageScroll.availableWidth
            spacing: 10

            // Header with name field and kebab menu
            RowLayout {
                Layout.fillWidth: true
                spacing: 9
                FormField {
                    label: qsTr("Name"); id: nameField; onEdited: page._changed = true
                    Layout.fillWidth: true
                }
                KebabButton {
                    Layout.alignment: Qt.AlignBottom
                    Layout.bottomMargin: 4
                    onClicked: (sx, sy) => page._openSelfMenu(sx, sy)
                }
            }
            GridLayout {
                Layout.fillWidth: true; columns: 2; columnSpacing: 10; rowSpacing: 9
                FormField { label: qsTr("Email"); id: emailField; onEdited: page._changed = true }
                ComboField {
                    label: qsTr("Client"); id: clientCombo; options: page._clientNames()
                    includeNone: true; searchable: true
                    onActivated: (v) => { page._clientId = page._idForName(v); page._changed = true }
                }
                FormField { label: qsTr("Office Phone"); id: officeField; onEdited: page._changed = true }
                FormField { label: qsTr("Cell Phone"); id: cellField; onEdited: page._changed = true }
                FormField { label: qsTr("Role"); id: roleField; onEdited: page._changed = true }
            }
            Item { Layout.preferredHeight: 8 }
        }
    }

    // Person's own record/plugin menu — opened by the kebab button
    RecordContextMenu {
        id: selfMenu
        recordType: qsTr("Person")
        model: DesktopAppController.peopleModel
        recordId: page.personId
        canOpen: false
        canNew: false
        canDelete: false
        canMoveTo: false
        canExport: true
        canFilter: false
        canRefresh: true
        onExportRequested: {} // Handled by parent (Main.qml)
        onDuplicateRequested: {
            // Copy what's on screen, not what was last written.
            page._saveNow()
            var newId = DesktopAppController.duplicateRecord(DesktopAppController.peopleModel, page.personId)
            if (newId !== "") page.personActivated(DesktopAppController.peopleRowForId(newId), newId)
        }
        onRefreshRequested: page._reload()
        onGoToClientRequested: page.goToClientRequested(clientId)
    }
}
