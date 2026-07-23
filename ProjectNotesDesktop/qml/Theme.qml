// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

pragma Singleton
import QtQuick

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

    // ── Metrics ───────────────────────────────────────────────────────────────
    readonly property int radiusSm: 8
    readonly property int radius:   10
    readonly property int radiusLg: 14
    readonly property int railWidth: 56
    readonly property int sidebarWidth: 264

    // Palette for folder color swatches in the settings editor.
    readonly property var folderColors: [
        "#c98a1a", "#27ae60", "#2a3f6f", "#c0442e",
        "#7d5bb5", "#0e8a8a", "#b5652f", "#8A857A"
    ]
    // A small, safe set of Material Symbols usable as folder icons.
    readonly property var folderIcons: [
        "star", "folder", "favorite", "bookmark", "flag",
        "label", "bolt", "workspaces", "inventory_2", "grade"
    ]
}
