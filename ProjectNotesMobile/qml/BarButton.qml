// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls

// ToolButton in the app's monochrome tint. The iOS style colors a ToolButton's
// glyph and text with palette.button (system blue) and switches to
// palette.highlight while it is held down; both are replaced here, and the
// disabled group is set on its own so a disabled button still reads as one.
//
// Set `glass: true` for a round glass button, as on the header's menu and
// back buttons.
ToolButton {
    id: control

    property bool glass: false

    palette.active.button:   Theme.iconColor
    palette.inactive.button: Theme.iconColor
    palette.disabled.button: Theme.iconColorDisabled
    palette.highlight:       Theme.iconColorPressed

    // A Loader's implicit size is read-only, so a plain Item carries it and
    // the glass is only created for buttons that ask for it.
    background: Item {
        implicitWidth:  control.glass ? 44 : 0
        implicitHeight: control.glass ? 44 : 49   // 49 matches the iOS style
        Loader {
            anchors.fill: parent
            active: control.glass
            sourceComponent: GlassBar {
                contentPadding: 0
                solid: true
                scale: control.down ? 0.92 : 1
                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
            }
        }
    }
}
