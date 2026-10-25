// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A group of settings rows on a rounded card, with an optional gray title
// above it, as in the iOS Settings app. Put SettingsItem rows inside; the
// section draws a separator under every row but the last.
//
//     SettingsSection {
//         title: qsTr("App")
//         SettingsItem { text: qsTr("Appearance"); iconName: "circle.lefthalf.filled" }
//     }
ColumnLayout {
    id: root

    property string title: ""
    default property alias rows: rowColumn.data

    Layout.fillWidth: true
    Layout.leftMargin: 16
    Layout.rightMargin: 16
    Layout.topMargin: title !== "" ? 20 : 16
    spacing: 8

    function _updateSeparators() {
        var shown = []
        for (var i = 0; i < rowColumn.children.length; i++) {
            var row = rowColumn.children[i]
            if (row.visible && row.showSeparator !== undefined)
                shown.push(row)
        }
        for (var j = 0; j < shown.length; j++)
            shown[j].showSeparator = j < shown.length - 1
    }

    Label {
        visible: root.title !== ""
        Layout.leftMargin: 16
        text: root.title
        font.pixelSize: Theme.subheadline; font.letterSpacing: Theme.tracking(Theme.subheadline)
        font.weight: Font.Medium
        color: Theme.secondaryText
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: rowColumn.implicitHeight
        radius: 22
        color: Theme.cardBackground

        Column {
            id: rowColumn
            width: parent.width
            onChildrenChanged: root._updateSeparators()
        }
    }

    Component.onCompleted: _updateSeparators()
}
