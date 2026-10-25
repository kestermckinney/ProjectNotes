// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Checkbox drawn like the ones on the Settings page: a small rounded box in
// the accent color with a white check, and a themed label beside it.
CheckBox {
    id: cb
    padding: 0
    spacing: 8
    implicitHeight: Math.max(16, label.implicitHeight)

    indicator: Rectangle {
        implicitWidth: 16; implicitHeight: 16; radius: 4
        x: 0; y: cb.height / 2 - height / 2
        color: cb.checked ? Theme.accent : Theme.surface
        border.color: cb.checked ? Theme.accent : Theme.border
        MaterialIcon { anchors.centerIn: parent; visible: cb.checked; name: "check"; size: 12; color: "#ffffff" }
    }
    contentItem: Text {
        id: label
        leftPadding: 16 + cb.spacing
        text: cb.text
        color: Theme.text
        font.pixelSize: Theme.fontBody
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
    }
}
