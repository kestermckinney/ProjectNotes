// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Client detail — just the client name for now (matches the schema).
Item {
    id: page
    property int    clientRow: -1
    property string clientId: ""
    property bool   _changed: false
    readonly property string exportTable: "clients"
    readonly property string exportId: clientId

    Component.onCompleted: _reload()

    function _reload() {
        var d = DesktopAppController.getClientData(page.clientRow)
        nameField.text = (d.client_name || "").toString()
        page._changed = false
    }

    function _saveNow() {
        if (!page._changed) return true
        var ok = DesktopAppController.saveClient(page.clientRow, nameField.text)
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
            FormField { label: qsTr("Client Name"); id: nameField; onEdited: page._changed = true }
            Item { Layout.preferredHeight: 8 }
        }
    }
}
