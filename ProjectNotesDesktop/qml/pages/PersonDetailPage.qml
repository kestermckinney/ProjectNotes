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
        return ok
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: page.width - 40
            spacing: 14

            FormField { label: qsTr("Name"); id: nameField; onEdited: page._changed = true }
            GridLayout {
                Layout.fillWidth: true; columns: 2; columnSpacing: 14; rowSpacing: 12
                FormField { label: qsTr("Email"); id: emailField; onEdited: page._changed = true }
                ComboField {
                    label: qsTr("Client"); id: clientCombo; options: page._clientNames()
                    onActivated: (v) => { page._clientId = page._idForName(v); page._changed = true }
                }
                FormField { label: qsTr("Office Phone"); id: officeField; onEdited: page._changed = true }
                FormField { label: qsTr("Cell Phone"); id: cellField; onEdited: page._changed = true }
                FormField { label: qsTr("Role"); id: roleField; onEdited: page._changed = true }
            }
            Item { Layout.preferredHeight: 8 }
        }
    }
}
