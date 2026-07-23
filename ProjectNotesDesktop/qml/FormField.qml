// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Labeled single-line text field.
ColumnLayout {
    id: root
    property string label: ""
    property alias text: field.text
    property string placeholder: ""
    property bool readOnly: false
    signal edited(string text)

    spacing: 4
    Layout.fillWidth: true

    Text {
        text: root.label
        visible: root.label !== ""
        color: Theme.text3
        font.pixelSize: 11
        font.weight: Font.DemiBold
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 34
        radius: Theme.radiusSm
        color: root.readOnly ? Theme.surface2 : Theme.surface
        border.color: field.activeFocus ? Theme.accent : Theme.border
        TextField {
            id: field
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            verticalAlignment: Text.AlignVCenter
            color: Theme.text
            placeholderText: root.placeholder
            placeholderTextColor: Theme.text3
            readOnly: root.readOnly
            background: null
            font.pixelSize: 13
            selectByMouse: true
            onTextEdited: root.edited(text)
        }
    }
}
