// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

pragma Singleton
import QtQuick
import ProjectNotesDesktop

// Central design system — ported from the "ProjectNotes UI Mockup" tokens.
// Warm off-white light theme / neutral dark theme with a navy accent.
QtObject {
    id: theme

    // Theme mode: "system" | "light" | "dark". Toggled from the icon rail.
    property string mode: "system"

    readonly property bool systemDark: Qt.styleHints.colorScheme === Qt.Dark
    readonly property bool dark: mode === "dark" || (mode === "system" && systemDark)

    function toggle() {
        // Cycle to the opposite of what is currently shown.
        mode = dark ? "light" : "dark"
    }

    // ── UI zoom (screen-sharing / presentation) ─────────────────────────────────
    // A global magnification factor. The shell applies it as a vector scale on a
    // logical viewport (see Main.qml zoomLayer) so the whole UI grows/shrinks and
    // REFLOWS to keep filling the window — crisp at any level because Qt Quick
    // re-rasterises distance-field glyphs at the composited scale (no blockiness).
    property real uiScale: 1.0
    readonly property real minScale:  0.7
    readonly property real maxScale:  2.5
    readonly property real scaleStep: 0.1

    function _clampScale(s) { return Math.max(minScale, Math.min(maxScale, Math.round(s * 100) / 100)) }
    function zoomIn()    { uiScale = _clampScale(uiScale + scaleStep) }
    function zoomOut()   { uiScale = _clampScale(uiScale - scaleStep) }
    function zoomReset() { uiScale = 1.0 }

    // Restore the last-saved zoom (persisted per-user, same local settings
    // store as window geometry); falls back to the 1.0 default above the
    // first time the app runs, before anything has been saved. Every change
    // afterwards — from the wheel, keyboard, or menu — is saved right back out
    // so it's still in effect on the next launch.
    Component.onCompleted: {
        var savedPercent = DesktopAppController.uiZoomPercent()
        if (savedPercent > 0)
            uiScale = _clampScale(savedPercent / 100)
    }
    onUiScaleChanged: DesktopAppController.setUiZoomPercent(Math.round(uiScale * 100))

    // ── Icon font ─────────────────────────────────────────────────────────────
    // One FontLoader for the whole app. MaterialIcon binds to iconFont — a
    // FontLoader declared inside MaterialIcon itself would be instantiated per
    // icon (hundreds per list page).
    readonly property FontLoader _iconFontLoader: FontLoader {
        source: "qrc:/qt/qml/ProjectNotesDesktop/resources/MaterialSymbolsRounded.ttf"
    }
    readonly property string iconFont: _iconFontLoader.name

    // ── Surfaces ──────────────────────────────────────────────────────────────
    readonly property color bg:        dark ? "#1E1D1B" : "#F3F1EA"
    readonly property color surface:   dark ? "#262523" : "#FBFAF6"
    readonly property color surface2:  dark ? "#302E2B" : "#ECE9E0"
    readonly property color raise:     dark ? "#2C2A27" : "#FFFFFF"
    readonly property color rail:      dark ? "#191817" : "#E7E3D9"
    readonly property color sidebar:   dark ? "#222120" : "#EFEDE4"

    // ── Borders ───────────────────────────────────────────────────────────────
    readonly property color border:     dark ? Qt.rgba(1,1,1,0.08) : "#E0DBCE"
    readonly property color borderSoft:  dark ? Qt.rgba(1,1,1,0.05) : "#EAE6DA"

    // ── Text ──────────────────────────────────────────────────────────────────
    readonly property color text:   dark ? "#ECEAE3" : "#292722"
    readonly property color text2:  dark ? "#A8A399" : "#5F5B52"
    readonly property color text3:  dark ? "#7C776D" : "#8A857A"

    // ── Accent ────────────────────────────────────────────────────────────────
    readonly property color accent:       dark ? "#7d9bd6" : "#2a3f6f"
    readonly property color accentStrong: dark ? "#a9bfe6" : "#1a2744"
    readonly property color accentSoft:   dark ? Qt.rgba(125/255,155/255,214/255,0.14)
                                               : Qt.rgba(42/255,63/255,111/255,0.09)

    // ── Status ────────────────────────────────────────────────────────────────
    readonly property color green:     dark ? "#2ecc71" : "#27ae60"
    readonly property color greenSoft:  dark ? Qt.rgba(46/255,204/255,113/255,0.15)
                                             : Qt.rgba(39/255,174/255,96/255,0.12)
    readonly property color amber:      dark ? "#e0a83a" : "#c98a1a"
    readonly property color amberSoft:  dark ? Qt.rgba(224/255,168/255,58/255,0.16)
                                             : Qt.rgba(201/255,138/255,26/255,0.14)
    readonly property color red:        dark ? "#e06a52" : "#c0442e"
    readonly property color redSoft:    dark ? Qt.rgba(224/255,106/255,82/255,0.14)
                                             : Qt.rgba(192/255,68/255,46/255,0.12)

    // Drop-down / drag highlight
    readonly property color dropHighlight: accentSoft

    // ── Font metrics ──────────────────────────────────────────────────────────
    // Two type ramps, both in logical pixels (Theme.uiScale magnifies them at
    // draw time, so these are the sizes seen at 100% zoom). Both now trace
    // back to a single anchor — the platform's own menu font size — rather
    // than a fixed design constant, so the whole app scales together on a
    // platform with a larger or smaller native UI font instead of just its
    // menus. macOS draws menus at 13pt where Windows uses 12px; the old
    // hardcoded 12px body/11px menu read as undersized against both.

    // Menu ramp — every hand-rolled popup menu (AppMenu, RecordContextMenu,
    // SortMenu, MenuFlyout and the rows they share). These stand in for
    // native menus, so they take their size straight from the platform's own
    // menu font. Everything else in a menu row (icon, checkmark, chevron, row
    // height) is derived from it so a bigger font grows the row instead of
    // clipping inside it; the offsets reproduce the proportions the 11px
    // design shipped with.
    readonly property int menuFont:   DesktopAppController.menuFontPixelSize
    readonly property int menuFontSm: menuFont - 1   // shortcut text, ALL-CAPS headers
    readonly property int menuFontLg: menuFont + 1   // the context menu's record label
    readonly property int menuIconSize:    menuFont + 3
    readonly property int menuCheckSize:   menuFont + 2
    readonly property int menuChevronSize: menuFont + 1
    readonly property int menuRowHeight:   menuFont + 13

    // Page ramp — the workspace ramp used by every page, card, dialog and
    // field. fontBody IS the menu font above (not a fixed 12px anymore), so
    // body text always matches the platform's own menu size; every other
    // step is an offset from fontBody, preserving the original
    // 8/9/10/11/12/13/14/15/16/18 proportions the design shipped with.
    readonly property int fontBody: menuFont            // default body / field text
    readonly property int font3xs:  fontBody - 4         // the smallest badges (list status pips)
    readonly property int font2xs:  fontBody - 3         // ALL-CAPS group labels, tiny chips
    readonly property int fontXs:   fontBody - 2         // secondary meta lines, table sub-text
    readonly property int fontSm:   fontBody - 1         // dense secondary text, compact rows
    readonly property int fontLg:   fontBody             // emphasised body, card titles
    readonly property int fontXl:   fontBody             // section + card headings
    readonly property int font2xl:  fontBody + 1         // dialog titles
    readonly property int font3xl:  fontBody + 1         // About block product name
    readonly property int font4xl:  fontBody             // empty-state / stub display text

    // List/folder-panel ramp — every font.pixelSize in ProjectsListPage's card
    // rows and the sidebar's FolderGroup rows. Equal to fontBody/menuFont
    // (kept as its own name for what it documents: these two panels are the
    // ones the user scans most, pinned to the platform's menu size on
    // purpose rather than incidentally because it's also the body size).
    readonly property int listFont: menuFont

    // Earned-value / financial-strip ramp — the Budget/Actual/BCWP/BCWS/BAC/
    // Consumed/EAC/CV/SV/CPI/Complete chips on a project card (the "Show
    // Internal" strip). Deliberately a step below listFont: this is the
    // densest row on the card and it's secondary/optional (hidden by
    // default), so it should read as quieter than the always-visible date
    // chips above it rather than competing with them at the same size.
    readonly property int financialFont: listFont - 2

    // Icon rail ramp — the left navigation rail (IconRail.qml): hamburger,
    // section buttons, sync indicator, theme toggle, avatar. Also anchored on
    // menuFont, but offset ~15% larger than the flat 34px button / 18px icon
    // the compact pass left it at — those read as too small once everything
    // else started following the platform's own menu size upward.
    readonly property int railButtonSize:   menuFont + 27   // ≈34px×1.15 at menuFont 12
    readonly property int railHoverSize:    railButtonSize - 2   // hover/active pill inside the button
    readonly property int railIconSize:     menuFont + 9    // ≈18px×1.15
    readonly property int railSyncIconSize: railIconSize - 1   // sync glyph reads a touch smaller
    readonly property int railRingSize:     railHoverSize - 4   // sync progress ring, inset in the hover pill
    readonly property int railAvatarSize:   railButtonSize - 9  // ≈26px×1.15

    // ── Metrics ───────────────────────────────────────────────────────────────
    readonly property int radiusSm: 6
    readonly property int radius:   8
    readonly property int radiusLg: 10
    readonly property int railWidth: railButtonSize + 14   // same ~7px margin each side as before
    readonly property int sidebarWidth: 228

    // Palette for folder color swatches in the settings editor.
    readonly property var folderColors: [
        "#c98a1a", "#27ae60", "#2a3f6f", "#c0442e",
        "#7d5bb5", "#0e8a8a", "#b5652f", "#8A857A"
    ]
    // A small, safe set of Material Symbols usable as folder icons.
    readonly property var folderIcons: [
        "star", "folder", "favorite", "bookmark", "flag",
        "label", "bolt", "workspaces", "inventory_2", "stars"
    ]
}
