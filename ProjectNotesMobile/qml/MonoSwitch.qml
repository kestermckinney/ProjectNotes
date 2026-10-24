// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Effects

// Switch in the iOS 26 style: a wide pill-shaped knob that stretches while
// pressed. Off is a gray track, on is the standard iOS green; the knob is
// always white. Use it everywhere in place of Switch; a `text` label works
// as it does on Switch.
Switch {
    id: control

    indicator: Item {
        implicitWidth: 64
        implicitHeight: 28
        // Same placement as the iOS style's own indicator, so the label
        // (contentItem) still lines up beside it.
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
                        : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        opacity: control.enabled ? 1 : 0.5

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: control.checked ? Theme.switchOnTrack : Theme.switchOffTrack
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        RectangularShadow {
            anchors.fill: knob
            radius: knob.radius
            offset.y: 1
            blur: 4
            color: Qt.rgba(0, 0, 0, 0.18)
        }

        Rectangle {
            id: knob
            readonly property real _pos: Math.max(0, Math.min(1, control.visualPosition))
            width: control.pressed ? 44 : 38
            height: parent.height - 4
            radius: height / 2
            y: 2
            x: 2 + _pos * (parent.width - width - 4)
            color: "#ffffff"
            Behavior on x { enabled: !control.down; NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
            Behavior on width { NumberAnimation { duration: 120 } }
        }
    }
}
