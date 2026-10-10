// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Company detail — just the company name for now (matches the schema).
Item {
    id: page
    property int    companyRow: -1
    property string companyId: ""
    property bool   _changed: false
    readonly property string exportTable: "clients"
    readonly property string exportId: companyId

    Component.onCompleted: _reload()

    function _reload() {
        var d = DesktopAppController.getCompanyData(page.companyRow)
        nameField.text = (d.client_name || "").toString()
        page._changed = false
    }

    function _saveNow() {
        if (!page._changed) return true
        var ok = DesktopAppController.saveCompany(page.companyRow, nameField.text)
        if (ok) page._changed = false
        else _reload()   // revert fields to last valid values when an edit is rejected
        return ok
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
            FormField { label: qsTr("Company Name"); id: nameField; onEdited: page._changed = true }
            Item { Layout.preferredHeight: 6 }
        }
    }
}
