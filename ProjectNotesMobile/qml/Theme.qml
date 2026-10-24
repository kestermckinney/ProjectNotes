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

    // One background behind lists, forms and the bars above them: the color
    // the iOS style paints list rows (its palette.base). Main.qml sets it as
    // the window's palette.window so pages and forms use it too, and the
    // header and search bars set it explicitly, so no band of a different
    // shade shows between them.
    readonly property color pageBackground:  isDarkMode ? "#1c1c1e" : "#ffffff"

    // Height of Main.qml's frosted title bar, which pages run up underneath.
    // Main.qml keeps it current; pages start their top row this far down.
    property real topInset: 0

    // Section label text above form fields. Navy in light mode; lifted in dark
    // mode so it stays readable on the page background.
    readonly property color sectionText:     isDarkMode ? "#9db4e6" : navyMid

    // Button tint: monochrome, black in light mode and white in dark mode, in
    // place of the iOS style's system blue. Main.qml sets it as the window's
    // palette.button, which the style reads for flat button text, filled
    // button backgrounds, and ToolButton glyphs; BarButton also sets it.
    readonly property color iconColor:         isDarkMode ? "#ffffff" : "#000000"
    // The same tint while a button is held down or disabled.
    readonly property color iconColorPressed:  isDarkMode ? "#8e8e93" : "#636366"
    readonly property color iconColorDisabled: isDarkMode ? "#48484a" : "#c7c7cc"

    // Grouped settings: rounded cards on the page background, a hairline
    // between rows, gray section titles and values (iOS grouped-table colors).
    readonly property color cardBackground:   isDarkMode ? "#2c2c2e" : "#f2f2f7"
    readonly property color separator:        isDarkMode ? Qt.rgba(84/255, 84/255, 88/255, 0.65)
                                                         : Qt.rgba(60/255, 60/255, 67/255, 0.18)
    readonly property color secondaryText:    isDarkMode ? "#98989f" : "#6c6c70"

    // MonoSwitch track: gray while off, iOS system green while on.
    readonly property color switchOnTrack:    isDarkMode ? "#30d158" : "#34c759"
    readonly property color switchOffTrack:   isDarkMode ? Qt.rgba(120/255, 120/255, 128/255, 0.40)
                                                         : Qt.rgba(120/255, 120/255, 128/255, 0.20)

    // Focus ring and glow for GlassField inputs: monochrome to match the
    // buttons, white in dark mode and black in light mode.
    readonly property color focusRing:       iconColor
    readonly property color focusGlow:       isDarkMode ? Qt.rgba(1, 1, 1, 0.22)
                                                        : Qt.rgba(0, 0, 0, 0.14)

    // ── Type scale ────────────────────────────────────────────────────────────
    // Apple's iOS text styles (Large size, the default Dynamic Type setting),
    // so the app reads like the App Store and other system apps. Sizes are
    // logical pixels, which equal points on iOS.
    // Every size scales with the Appearance page's Text Size setting; Main.qml
    // keeps textScale current (1.0 is the iOS default, Large).
    property real textScale: 1.0
    function scaled(size) { return Math.round(size * textScale) }

    readonly property int largeTitle:  scaled(34)   // bold
    readonly property int title1:      scaled(28)   // bold for display text
    readonly property int title2:      scaled(22)   // bold for empty-state headings
    readonly property int title3:      scaled(20)   // semibold
    readonly property int headline:    scaled(17)   // semibold
    readonly property int body:        scaled(17)
    readonly property int callout:     scaled(16)
    readonly property int subheadline: scaled(15)
    readonly property int footnote:    scaled(13)
    readonly property int caption1:    scaled(12)
    readonly property int caption2:    scaled(11)

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
