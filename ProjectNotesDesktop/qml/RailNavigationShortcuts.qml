// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQml
import ProjectNotesDesktop

// Window shortcuts are scoped to the active page/menu popup. Qt then blocks
// them automatically when another modal dialog is above that context.
Instantiator {
    id: shortcuts
    property bool shortcutsEnabled: true
    signal triggered(string action)
    model: AppShortcuts.navigation
    delegate: Shortcut {
        required property var modelData
        sequence: AppShortcuts.map[modelData.action]
        enabled: shortcuts.shortcutsEnabled
        context: Qt.WindowShortcut
        autoRepeat: false
        onActivated: shortcuts.triggered(modelData.action)
    }
}
