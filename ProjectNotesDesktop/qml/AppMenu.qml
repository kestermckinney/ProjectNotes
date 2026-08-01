// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// The application ("hamburger") menu, opened from the icon rail. Mirrors the
// Widgets menu bar (File / Edit / View / Help) so the QML app exposes the same
// functionality. Navigation actions are emitted via triggered(); view toggles
// act directly on the Theme / controller and show a live check mark.
Popup {
    id: menu

    // Emitted for navigation-style actions handled by the shell (Main.qml).
    signal triggered(string action)

    modal: true
    dim: false
    padding: 6
    width: 232
    // A real floating window so the full File/Edit/View/Help list can spill past
    // the window edge instead of being clipped (Qt keeps it on-screen). As a
    // detached window it renders at 1x — the workspace zoom no longer applies.
    popupType: Popup.Window

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    // group model: { name, items:[{icon,label,key,action, toggle, on}] }
    readonly property var groups: [
        { name: qsTr("File"), items: [
            { icon: "note_add",  label: qsTr("New Record"),  key: "",   action: "new" },
            { icon: "search",    label: qsTr("Search…"),     key: "⌘K", action: "search" },
            { icon: "ios_share", label: qsTr("Export XML…"), key: "",   action: "export" },
            { icon: "download",  label: qsTr("Import XML…"), key: "",   action: "import" },
            { icon: "settings",  label: qsTr("Preferences"), key: "⌘,", action: "preferences" },
            { icon: "sync",      label: qsTr("Sync Now"),    key: "",   action: "sync" },
            { icon: "sync_alt",  label: qsTr("Sync All"),    key: "",   action: "sync_all" },
            { icon: "logout",    label: qsTr("Exit"),        key: "",   action: "exit" },
        ]},
        { name: qsTr("Edit"), items: [
            { icon: "search",       label: qsTr("Find"),        key: "⌘F", action: "find" },
            { icon: "filter_list",  label: qsTr("Filter Data…"),key: "",   action: "filter" },
        ]},
        { name: qsTr("View"), items: [
            { icon: Theme.dark ? "light_mode" : "dark_mode", label: qsTr("Dark Mode"),
              key: "", action: "toggle_theme", toggle: true, on: Theme.dark },
            { icon: "attach_money", label: qsTr("Show Internal / Budget Items"),
              key: "", action: "toggle_internal", toggle: true, on: DesktopAppController.showInternalItems },
            { icon: "folder", label: qsTr("Show Closed Projects"),
              key: "", action: "toggle_closed", toggle: true, on: DesktopAppController.showClosedProjects },
            { icon: "task_alt", label: qsTr("Show Resolved Items"),
              key: "", action: "toggle_resolved", toggle: true, on: !DesktopAppController.newAndAssignedOnly },
            { icon: "zoom_in",     label: qsTr("Zoom In"),    key: "⌘+", action: "zoom_in" },
            { icon: "zoom_out",    label: qsTr("Zoom Out"),   key: "⌘−", action: "zoom_out" },
            { icon: "restart_alt", label: qsTr("Reset Zoom"), key: "⌘0", action: "zoom_reset" },
            { icon: "description", label: qsTr("Log Viewer"), key: "", action: "logs" },
        ]},
        { name: qsTr("Help"), items: [
            { icon: "menu_book", label: qsTr("User Guide"), key: "F1", action: "help" },
            { icon: "system_update", label: qsTr("Check for Updates…"), key: "", action: "check_updates" },
            { icon: "forward_to_inbox", label: qsTr("Send Logs to Support…"), key: "", action: "support_logs" },
            { icon: "info", label: qsTr("About"), key: "", action: "about" },
        ]},
    ]

    // Global (dataless) plugin menus — Plugins > Settings / Utilities / etc. in
    // the Widgets menu bar. Rebuilt each time the menu opens since plugins can
    // hot-reload. One group per distinct submenu (e.g. "Settings", "Utilities").
    property var pluginGroups: []
    function _rebuildPluginGroups() {
        var bysubmenu = {}
        var order = []
        var entries = DesktopAppController.globalPluginMenus()
        for (var i = 0; i < entries.length; i++) {
            var e = entries[i]
            var sub = e.submenu || qsTr("Plugins")
            if (!bysubmenu[sub]) { bysubmenu[sub] = []; order.push(sub) }
            bysubmenu[sub].push({ icon: "extension", label: e.title, key: "", action: "plugin:" + e.index })
        }
        var groups = []
        for (var g = 0; g < order.length; g++)
            groups.push({ name: qsTr("Plugins") + " · " + order[g], items: bysubmenu[order[g]] })
        pluginGroups = groups
    }
    onAboutToShow: _rebuildPluginGroups()

    function _act(a) {
        switch (a) {
        case "toggle_theme":    Theme.toggle(); return
        case "toggle_internal": DesktopAppController.showInternalItems = !DesktopAppController.showInternalItems; return
        case "toggle_closed":   DesktopAppController.showClosedProjects = !DesktopAppController.showClosedProjects; return
        case "toggle_resolved": DesktopAppController.newAndAssignedOnly = !DesktopAppController.newAndAssignedOnly; return
        // Zoom acts in place and keeps the menu open so it can be nudged repeatedly.
        case "zoom_in":         Theme.zoomIn();    return
        case "zoom_out":        Theme.zoomOut();   return
        case "zoom_reset":      Theme.zoomReset(); return
        default:
            if (a.indexOf("plugin:") === 0) {
                DesktopAppController.runGlobalPluginMenu(parseInt(a.substring(7), 10))
                menu.close()
                return
            }
            menu.triggered(a); menu.close()
        }
    }

    contentItem: ColumnLayout {
        spacing: 0
        Repeater {
            model: menu.groups.concat(menu.pluginGroups)
            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 0
                Text {
                    text: modelData.name.toUpperCase()
                    color: Theme.text3
                    font.pixelSize: 10; font.weight: Font.Bold
                    Layout.leftMargin: 10; Layout.topMargin: 7; Layout.bottomMargin: 3
                }
                Repeater {
                    model: modelData.items
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: 32
                        radius: Theme.radiusSm
                        color: rowHover.hovered ? Theme.surface2 : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10; anchors.rightMargin: 10
                            spacing: 10
                            MaterialIcon {
                                name: modelData.icon; size: 18; color: Theme.text2
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Text {
                                text: modelData.label; color: Theme.text; font.pixelSize: 13
                                Layout.fillWidth: true; elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }
                            // check mark for active toggle items
                            MaterialIcon {
                                visible: modelData.toggle === true && modelData.on === true
                                name: "check"; size: 17; color: Theme.accent
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Text {
                                visible: (modelData.key || "") !== "" && !(modelData.toggle === true)
                                text: modelData.key || ""; color: Theme.text3; font.pixelSize: 11
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        HoverHandler { id: rowHover }
                        TapHandler { onTapped: menu._act(modelData.action) }
                    }
                }
            }
        }
    }
}
