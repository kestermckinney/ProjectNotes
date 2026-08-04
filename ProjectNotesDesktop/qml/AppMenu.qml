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

    // The table + id of whatever record is currently open (set by Main.qml via
    // IconRail — same signal the TopBar's Export XML button uses). When present,
    // table-scoped plugin menus (dataexport matching this table, e.g. the
    // "Export" / "Templates" groups on a project) are added below the global
    // ones, mirroring the Widgets Plugins menu bar (buildPluginMenu() adds both
    // the global entries and BasePage::buildPluginMenu()'s current-page entries
    // to the same menu).
    property string pluginMenuTable: ""
    property string pluginMenuRecordId: ""

    modal: true
    dim: false
    padding: 6
    // Size to the widest row rather than a fixed width, so long labels
    // (e.g. "Show Internal / Budget Items") are never clipped. Clamped so the
    // menu stays readable but doesn't grow unbounded.
    width: Math.max(232, Math.min(implicitContentWidth + leftPadding + rightPadding, 460))
    // Cap the height and scroll once the File/Edit/View/Help + plugin groups
    // grow past that. The rail lowers this to the window's logical height so the
    // in-scene popup never clips at the window edge; the ScrollView takes over.
    property int maxMenuHeight: 760
    height: Math.min(_content.implicitHeight + topPadding + bottomPadding, maxMenuHeight)
    // Deliberately an in-scene popup (default Popup.Item) — popupType:
    // Popup.Window (Qt 6.10) forwards clicks on the menu rows to the main window
    // underneath, activating whatever record sits behind the menu instead of the
    // row's action, so a detached window is unusable here.
    scale: Theme.uiScale            // match the zoomed workspace
    transformOrigin: Item.TopLeft   // grow down-right from the rail anchor

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
            { icon: "system_update_alt", label: qsTr("Check for Updates…"), key: "", action: "check_updates" },
            { icon: "forward_to_inbox", label: qsTr("Send Logs to Support…"), key: "", action: "support_logs" },
            { icon: "info", label: qsTr("About"), key: "", action: "about" },
        ]},
    ]

    // Global (dataless) plugin menus, plus the open record's table-scoped ones
    // when there is one — Plugins > Settings / Utilities / Export / Templates /
    // etc. in the Widgets menu bar. Rebuilt each time the menu opens since
    // plugins can hot-reload. One group per distinct submenu.
    property var pluginGroups: []
    function _rebuildPluginGroups() {
        var bysubmenu = {}
        var order = []
        function addEntries(entries, actionPrefix) {
            for (var i = 0; i < entries.length; i++) {
                var e = entries[i]
                var sub = e.submenu || qsTr("Plugins")
                if (!bysubmenu[sub]) { bysubmenu[sub] = []; order.push(sub) }
                bysubmenu[sub].push({ icon: "extension", label: e.title, key: "", action: actionPrefix + e.index })
            }
        }
        addEntries(DesktopAppController.globalPluginMenus(), "plugin:")
        if (menu.pluginMenuTable !== "" && menu.pluginMenuRecordId !== "")
            addEntries(DesktopAppController.pluginMenusForTable(menu.pluginMenuTable), "tableplugin:")
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
            if (a.indexOf("tableplugin:") === 0) {
                DesktopAppController.runPluginMenuForTable(menu.pluginMenuTable, menu.pluginMenuRecordId,
                                                            parseInt(a.substring(12), 10))
                menu.close()
                return
            }
            if (a.indexOf("plugin:") === 0) {
                DesktopAppController.runGlobalPluginMenu(parseInt(a.substring(7), 10))
                menu.close()
                return
            }
            menu.triggered(a); menu.close()
        }
    }

    contentItem: ScrollView {
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            id: _content
            width: menu.availableWidth
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
                            // Report the row's natural width (content + the 10px side
                            // margins) so the Popup can size to its widest row. The
                            // RowLayout is anchored, so it won't drive width on its own.
                            implicitWidth: rowContent.implicitWidth + 20
                            radius: Theme.radiusSm
                            color: rowHover.hovered ? Theme.surface2 : "transparent"
                            RowLayout {
                                id: rowContent
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
                            // Exclusive grab (not a plain passive-grab TapHandler): a
                            // passive grab lets the same tap fall through to the record
                            // list behind the menu, selecting a row instead of firing
                            // the action. Matches the dialog-button fix in Main.qml.
                            TapHandler {
                                gesturePolicy: TapHandler.ReleaseWithinBounds
                                onTapped: menu._act(modelData.action)
                            }
                        }
                    }
                }
            }
        }
    }
}
