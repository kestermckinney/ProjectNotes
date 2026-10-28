// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic as Controls

// Shared push-button chrome, ported from ScheduleVault's Button.qml so the two
// apps' buttons read as one family. Neutral by default (a raised card that
// darkens on hover and darkens further while pressed); set `primary: true`
// for an accent-filled call-to-action button (Save, Add, Sync Now, …).
//
// Because this file lives in the same QML module as every other page/dialog
// (see CMakeLists' QML_FILES), a plain `Button { text: "…" }` anywhere in the
// app now resolves to this component instead of QtQuick.Controls.Basic's
// default — no per-instance background/padding boilerplate required.
Controls.Button {
    id: control
    property int radius: Theme.radiusSm
    property bool primary: false

    implicitWidth: Math.max(implicitContentWidth + leftPadding + rightPadding, 72)
    implicitHeight: 30
    leftPadding: 10
    rightPadding: 10

    // Label colours follow the theme, not the system palette, which stays
    // light-mode dark text when only the app is in dark mode. Same standard
    // as the dialog buttons: Theme.text on the neutral card, white on an
    // accent fill (primary, or any button while pressed). Setting the palette
    // rather than replacing contentItem keeps the style's icon support.
    font.pixelSize: Theme.fontBody
    font.weight: Font.DemiBold
    palette.buttonText: control.primary || control.down ? "#ffffff" : Theme.text
    palette.brightText: "#ffffff"
    palette.windowText: Theme.text
    palette.highlight: Theme.accent
    palette.disabled.buttonText: control.primary ? "#ffffff" : Theme.text3

    background: Rectangle {
        radius: control.radius
        color: control.primary
               ? (control.down ? Theme.accentStrong : Theme.accent)
               : (control.down ? Theme.accentStrong
                                : (control.hovered ? Theme.surface2 : Theme.raise))
        border.color: control.primary ? "transparent" : Theme.border
        border.width: control.primary ? 0 : .5
        opacity: control.enabled ? 1.0 : 0.5
    }
}
