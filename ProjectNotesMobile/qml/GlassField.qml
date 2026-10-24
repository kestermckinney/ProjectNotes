// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick

// Rounded, frosted background shared by every text field and combo box, in
// the style of iOS 26 glass controls: a translucent fill, a hairline edge,
// a faint sheen across the top, and an accent ring with a soft glow on focus.
//
//     TextField { id: field; background: GlassField { focused: field.activeFocus } }
Rectangle {
    id: root

    property bool focused: false
    property bool pressed: false

    readonly property bool _dark: Theme.isDarkMode

    implicitWidth: 120
    implicitHeight: 36
    radius: 14
    color: _dark ? Qt.rgba(118/255, 118/255, 128/255, pressed ? 0.34 : 0.24)
                 : Qt.rgba(118/255, 118/255, 128/255, pressed ? 0.16 : 0.10)
    border.width: focused ? 1.5 : 1
    border.color: focused ? Theme.focusRing
                          : (_dark ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.08))

    Behavior on border.color { ColorAnimation { duration: 150 } }
    Behavior on color { ColorAnimation { duration: 100 } }

    // Sheen: light catching the top edge of the glass.
    Rectangle {
        anchors.fill: parent
        anchors.margins: root.border.width
        radius: root.radius - root.border.width
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, root._dark ? 0.07 : 0.55) }
            GradientStop { position: 0.6; color: Qt.rgba(1, 1, 1, 0.0) }
        }
    }

    // Focus glow just outside the edge.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -3
        radius: root.radius + 3
        color: "transparent"
        border.width: 3
        border.color: Theme.focusGlow
        opacity: root.focused ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }
}
