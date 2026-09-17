// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
// `popup` is typed against the template Popup so both Popup and Dialog fit —
// see the matching note in ClickShield.qml.
import QtQuick.Templates as T
import ProjectNotesDesktop

// The title-bar "X" shared by every themed dialog. Declare it in the dialog's
// header row and point `popup` at the dialog:
//     DialogCloseButton { popup: myDialog }
//
// A MouseArea rather than a TapHandler on the bare glyph: the glyph's Text
// bounds made a small, unreliable hit target, and a MouseArea accepts the press
// outright, so the click lands here and nowhere else. Closing is deferred a
// beat (as in FilterDialog._dismiss()) so the release finishes delivery while
// the modal is still up, instead of falling through to whatever sits behind.
Rectangle {
    id: btn

    property T.Popup popup: null
    property int iconSize: 20

    implicitWidth: Math.max(24, iconSize + 4)
    implicitHeight: implicitWidth
    radius: Theme.radiusSm
    color: area.containsMouse ? Theme.surface2 : "transparent"

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Close")

    MaterialIcon {
        anchors.centerIn: parent
        name: "close"
        size: btn.iconSize
        color: area.containsMouse ? Theme.text : Theme.text3
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            var p = btn.popup
            if (p)
                Qt.callLater(function() { p.close() })
        }
    }
}
