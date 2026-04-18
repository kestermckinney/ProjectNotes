// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

pragma Singleton
import QtQuick

// Central color palette — matches the IFS Login page navy/green theme.
QtObject {
    // Toolbar / drawer header backgrounds
    readonly property color navyDark:        "#1a2744"
    // Section header text, accent dots, links
    readonly property color navyMid:         "#2a3f6f"
    // Active / success indicator dot
    readonly property color accentGreen:     "#2ecc71"
    // Active / success text
    readonly property color accentGreenDark: "#27ae60"
    // Section header background tint (6 % opacity navy)
    readonly property color sectionBg:       Qt.rgba(42/255, 63/255, 111/255, 0.06)
}
