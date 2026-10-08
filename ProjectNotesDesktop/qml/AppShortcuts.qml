// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

pragma Singleton
import QtQuick
import ProjectNotesDesktop

// Single source of truth for the app's keyboard shortcuts, keyed by the same
// action string AppMenu.qml's items carry and Main.qml's handleMenuAction()
// dispatches on. Main.qml (which wires the real Shortcut items) and
// AppMenu.qml (which just displays the key beside the menu label) both read
// from this map instead of hardcoding the sequence twice, so the two can't
// drift apart.
//
// Sequences use Qt's portable modifier names ("Ctrl", "Shift", "Alt", "Meta").
// Qt's own platform integration remaps Ctrl<->Cmd on macOS for every shortcut
// built from these tokens — a long-standing Qt/Cocoa convention, not
// something this app has to branch on — so "Ctrl+N" already fires on Cmd+N
// there. See DesktopAppController::nativeShortcutText(), which renders the
// on-screen label via QKeySequence::toString(NativeText) so what's displayed
// always matches whatever key actually fires.
//
// Toggle Fullscreen is the one exception: macOS's native fullscreen shortcut
// (physical Control+Command+F) genuinely differs from Windows/Linux's F11
// rather than just swapping Ctrl for Cmd, so it's computed per-OS below
// instead of being a fixed portable string like everything else. In Qt's
// portable vocabulary that physical combo is "Meta+Ctrl+F" — on macOS the
// "Ctrl" token maps to physical Command and "Meta" maps to physical Control
// (the same swap that makes plain "Ctrl+X" fire on Cmd+X there).
QtObject {
    id: root

    readonly property string fullscreen: Qt.platform.os === "osx" ? "Meta+Ctrl+F" : "F11"

    // Navigation destinations follow the rail's order. Menus, tooltips and
    // Shortcut instances all use these same actions and section identifiers.
    readonly property var navigation: [
        { section: "projects", label: qsTr("Projects"), icon: "description", action: "navigate_projects" },
        { section: "items", label: qsTr("Action Items"), icon: "task_alt", action: "navigate_items" },
        { section: "people", label: qsTr("People"), icon: "group", action: "navigate_people" },
        { section: "clients", label: qsTr("Clients"), icon: "apartment", action: "navigate_clients" },
        { section: "search", label: qsTr("Search"), icon: "search", action: "navigate_search" },
        { section: "help", label: qsTr("User Guide"), icon: "menu_book", action: "navigate_help" },
        { section: "settings", label: qsTr("Settings"), icon: "settings", action: "navigate_settings" }
    ]

    function destination(section) {
        for (var i = 0; i < navigation.length; ++i)
            if (navigation[i].section === section) return navigation[i]
        return null
    }

    function sectionForAction(action) {
        for (var i = 0; i < navigation.length; ++i)
            if (navigation[i].action === action) return navigation[i].section
        return ""
    }

    readonly property var map: ({
        "navigate_projects": "Ctrl+1",
        "navigate_items":    "Ctrl+2",
        "navigate_people":   "Ctrl+3",
        "navigate_clients":  "Ctrl+4",
        "navigate_search":   "Ctrl+5",
        "navigate_help":     "Ctrl+6",
        "navigate_settings": "Ctrl+7",
        "new":               "Ctrl+N",
        // "search" (File menu) and "find" (Edit menu) both land on
        // selectSection("search") in Main.qml's handleMenuAction — same
        // destination, so they share one key rather than fighting over two
        // (a second Shortcut on the same sequence would just make both
        // ambiguous and silently fire neither).
        "search":            "Ctrl+K",
        "find":               "Ctrl+K",
        // Drops the cursor into the top bar's quick search field. The only
        // entry here without an AppMenu row of its own — nothing displays it in
        // a menu, it lives in this map so every sequence the app binds stays
        // visible in one list. Ctrl+L is the browser address-bar convention and
        // collides with nothing else in this frontend (the note editor's
        // alignment commands are toolbar-only; the deprecated Widgets frontend
        // binds Ctrl+L to Align Left, but that's a separate application).
        "focus_quick_search": "Ctrl+L",
        "export":            "Ctrl+Shift+E",
        "import":            "Ctrl+Shift+I",
        "preferences":       "Ctrl+,",
        "exit":              "Ctrl+Q",
        "filter":            "Ctrl+Shift+F",
        "sort":              "Ctrl+Shift+S",
        "toggle_fullscreen": root.fullscreen,
        "back":              "Alt+Left",
        "help":              "F1"
    })

    // Native, per-OS display text for an action's shortcut ("" if it has none).
    function text(action) {
        var seq = root.map[action] || ""
        return seq === "" ? "" : DesktopAppController.nativeShortcutText(seq)
    }
}
