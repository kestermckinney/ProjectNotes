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
    // Toolbar badge + glyph tint marking an active filter or sort. Orange, not
    // the green above, which already reads as "resolved / active status" in the
    // lists. The light-mode value is deepened from iOS system orange so a small
    // dot still carries against a near-white toolbar.
    readonly property color accentOrange:    isDarkMode ? "#ff9f0a" : "#e2760a"
    // Section header background tint (6 % opacity navy)
    readonly property color sectionBg:       Qt.rgba(42/255, 63/255, 111/255, 0.06)

    // Subtitle / secondary text on lists. The system palette's placeholderText
    // is a medium gray that fails contrast in both modes — this stays close to
    // the primary text color so the second line in a list cell stays readable.
    readonly property bool  isDarkMode:      Qt.styleHints.colorScheme === Qt.Dark
    readonly property color mutedText:       isDarkMode ? "#e8e8eb" : "#1c1c1e"

    // Section label text above form fields. Navy in light mode; lifted in dark
    // mode so it stays readable on the page background.
    readonly property color sectionText:     isDarkMode ? "#9db4e6" : navyMid

    // Focus ring and glow for GlassField inputs (iOS system blue).
    readonly property color focusRing:       isDarkMode ? "#0a84ff" : "#007aff"
    readonly property color focusGlow:       isDarkMode ? Qt.rgba(10/255, 132/255, 1, 0.30)
                                                        : Qt.rgba(0, 122/255, 1, 0.22)

    // ── Type scale ────────────────────────────────────────────────────────────
    // Apple's iOS text styles (Large size, the default Dynamic Type setting),
    // so the app reads like the App Store and other system apps. Sizes are
    // logical pixels, which equal points on iOS.
    readonly property int largeTitle:  34   // bold
    readonly property int title1:      28   // bold for display text
    readonly property int title2:      22   // bold for empty-state headings
    readonly property int title3:      20   // semibold
    readonly property int headline:    17   // semibold
    readonly property int body:        17
    readonly property int callout:     16
    readonly property int subheadline: 15
    readonly property int footnote:    13
    readonly property int caption1:    12
    readonly property int caption2:    11

    // SF Pro tracking in points per size, from Apple's iOS tracking table.
    // UIKit applies it automatically; Qt lays text out with the font's raw
    // advances, so without this, body text sits looser than native text and
    // titles sit tighter. Use as:
    //     font.letterSpacing: Theme.tracking(font.pixelSize)
    readonly property var _trackingTable: ({
        10: 0.12, 11: 0.06, 12: 0.0, 13: -0.08, 14: -0.15, 15: -0.23,
        16: -0.31, 17: -0.43, 18: -0.44, 19: -0.45, 20: -0.45, 21: -0.36,
        22: -0.26, 23: -0.17, 24: -0.07, 25: 0.0, 26: 0.07, 27: 0.14,
        28: 0.36, 29: 0.37, 30: 0.37, 31: 0.38, 32: 0.39, 33: 0.40, 34: 0.40
    })
    function tracking(size) {
        var s = Math.round(size)
        if (s <= 0)  return 0
        if (s < 10)  return 0.12
        if (s > 34)  return 0.37
        return _trackingTable[s]
    }
}
