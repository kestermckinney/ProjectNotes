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

    readonly property var map: ({
        "new":               "Ctrl+N",
        // "search" (File menu) and "find" (Edit menu) both land on
        // selectSection("search") in Main.qml's handleMenuAction — same
        // destination, so they share one key rather than fighting over two
        // (a second Shortcut on the same sequence would just make both
        // ambiguous and silently fire neither).
        "search":            "Ctrl+K",
        "find":               "Ctrl+K",
        "export":            "Ctrl+Shift+E",
        "import":            "Ctrl+Shift+I",
        "preferences":       "Ctrl+,",
        "exit":              "Ctrl+Q",
        "filter":            "Ctrl+Shift+F",
        "toggle_fullscreen": root.fullscreen,
        "help":              "F1"
    })

    // Native, per-OS display text for an action's shortcut ("" if it has none).
    function text(action) {
        var seq = root.map[action] || ""
        return seq === "" ? "" : DesktopAppController.nativeShortcutText(seq)
    }
}
