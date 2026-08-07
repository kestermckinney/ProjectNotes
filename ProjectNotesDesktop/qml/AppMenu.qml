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
    // "Export" / "Templates" groups on a project) are folded into the same
    // single "Plugins" entry as the global ones (see _rebuildPluginGroups),
    // mirroring the Widgets Plugins menu bar (buildPluginMenu() adds both the
    // global entries and BasePage::buildPluginMenu()'s current-page entries to
    // the same menu).
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

    // group model: { name, items:[{icon,label,key,action, toggle, on}] }.
    // `key` is the native display text for whatever real Shortcut Main.qml
    // wires for that action — see AppShortcuts.qml, the single source of
    // truth both sides read from. Blank means the action has no assigned key.
    readonly property var groups: [
        { name: qsTr("File"), items: [
            { icon: "note_add",  label: qsTr("New Record"),  key: AppShortcuts.text("new"),         action: "new" },
            { icon: "search",    label: qsTr("Search…"),     key: AppShortcuts.text("search"),      action: "search" },
            { icon: "ios_share", label: qsTr("Export XML…"), key: AppShortcuts.text("export"),      action: "export" },
            { icon: "download",  label: qsTr("Import XML…"), key: AppShortcuts.text("import"),      action: "import" },
            { icon: "settings",  label: qsTr("Preferences"), key: AppShortcuts.text("preferences"), action: "preferences" },
            { icon: "sync",      label: qsTr("Sync Now"),    key: "",   action: "sync" },
            { icon: "sync_alt",  label: qsTr("Sync All"),    key: "",   action: "sync_all" },
            { icon: "logout",    label: qsTr("Exit"),        key: AppShortcuts.text("exit"),        action: "exit" },
        ]},
        { name: qsTr("Edit"), items: [
            { icon: "search",       label: qsTr("Find"),        key: AppShortcuts.text("find"),   action: "find" },
            { icon: "filter_list",  label: qsTr("Filter Data…"),key: AppShortcuts.text("filter"), action: "filter" },
            { icon: "swap_vert",    label: qsTr("Sort…"),       key: AppShortcuts.text("sort"),   action: "sort" },
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
            // Brave-style zoom control (− / percent / + / fullscreen), replacing
            // the old separate Zoom In / Zoom Out / Reset Zoom rows — rendered by
            // ZoomMenuRow, see MenuFlyout's `custom: "zoom"` handling. Its own
            // fullscreen button shows AppShortcuts.text("toggle_fullscreen") in
            // its tooltip.
            { custom: "zoom" },
            { icon: "description", label: qsTr("Log Viewer"), key: "", action: "logs" },
        ]},
        { name: qsTr("Help"), items: [
            { icon: "menu_book", label: qsTr("User Guide"), key: AppShortcuts.text("help"), action: "help" },
            { icon: "system_update_alt", label: qsTr("Check for Updates…"), key: "", action: "check_updates" },
            { icon: "forward_to_inbox", label: qsTr("Send Logs to Support…"), key: "", action: "support_logs" },
            { icon: "info", label: qsTr("About"), key: "", action: "about" },
        ]},
    ]

    // All plugin menus — both the global (dataless) ones and, when a record is
    // open, its table-scoped ones (Export / Templates / etc., tied to whatever's
    // in pluginMenuTable/pluginMenuRecordId) — collapse into a single "Plugins"
    // entry (see _allGroups, which slots it in right after File) instead of
    // spilling one top-level row per plugin submenu into the menu. Entries with
    // no submenu become plain leaf rows inside it; entries that share a submenu
    // become a nested group row (icon/label only, no "Plugins ·" prefix needed
    // now that they're visually nested under Plugins already) — see
    // MenuFlyout's group support and pluginSubFlyout below. Actions are tagged
    // "plugin:" vs "tableplugin:" (see _act()) so global and table-scoped
    // entries still dispatch to the right controller call despite sharing one
    // menu.
    property var globalPluginGroup: null
    function _rebuildPluginGroups() {
        var byKey = {}
        var order = []
        function addEntries(entries, actionPrefix) {
            for (var i = 0; i < entries.length; i++) {
                var e = entries[i]
                var key = e.submenu || ""
                if (byKey[key] === undefined) { byKey[key] = []; order.push(key) }
                byKey[key].push({ icon: "extension", label: e.title, key: "", action: actionPrefix + e.index })
            }
        }
        addEntries(DesktopAppController.globalPluginMenus(), "plugin:")
        if (menu.pluginMenuTable !== "" && menu.pluginMenuRecordId !== "")
            addEntries(DesktopAppController.pluginMenusForTable(menu.pluginMenuTable), "tableplugin:")

        var items = []
        for (var k = 0; k < order.length; k++) {
            var gkey = order[k]
            if (gkey === "")
                items = items.concat(byKey[gkey])
            else
                items.push({ icon: "extension", label: gkey, key: "", group: { name: gkey, items: byKey[gkey] } })
        }
        globalPluginGroup = items.length > 0 ? { name: qsTr("Plugins"), items: items } : null
    }
    onAboutToShow: _rebuildPluginGroups()

    // Which top-level group (index into _allGroups) currently has its flyout
    // open, or -1. Only one flyout is open at a time — hovering a sibling
    // trigger row reassigns this and repoints the single shared flyout instead
    // of opening a second popup. File/Edit/View/Help with the dynamic Plugins
    // entry (global + table-scoped plugin menus, see _rebuildPluginGroups)
    // spliced in right after File.
    readonly property var _allGroups:
        [menu.groups[0]].concat(menu.globalPluginGroup ? [menu.globalPluginGroup] : [])
                         .concat(menu.groups.slice(1))
    property int openGroupIndex: -1
    // The raw (unmapped) items of the open group — a live binding (not a
    // one-shot snapshot) so a toggle item's checkmark (Dark Mode, Show Closed
    // Projects, …) updates immediately when clicked without the flyout needing
    // to be closed and reopened.
    readonly property var _openRawItems:
        (openGroupIndex >= 0 && openGroupIndex < _allGroups.length) ? _allGroups[openGroupIndex].items : []
    // groups/globalPluginGroup items carry {icon,label,key,action,toggle,on};
    // the flyout expects {icon,label,trailingText,toggle,checked}.
    function _mapLeaf(it) {
        return { icon: it.icon || "", label: it.label || "", trailingText: it.key || "",
                 toggle: it.toggle === true, checked: it.on === true }
    }

    // Hover-to-open, like a native menu bar: entering a trigger row arms this
    // and it fires _activateGroup() after a short delay. Clicking bypasses it.
    // Deliberately no matching "hover-away closes it" timer — an earlier
    // version raced a close-grace timer against the pointer crossing from the
    // row to the flyout and would often vanish before it could be clicked.
    // Like a native menu, once open it stays open until something else closes
    // it (see the various onClosed/onOpenGroupIndexChanged handlers below).
    Timer {
        id: openDelay
        interval: 300
        property int pendingIndex: -1
        onTriggered: menu._activateGroup(pendingIndex)
    }
    function _activateGroup(idx) {
        if (idx < 0 || idx >= menu._allGroups.length) return
        menu.openGroupIndex = idx
        flyout.openBeside(triggerRepeater.itemAt(idx))
    }
    // Switching top-level groups invalidates whatever nested Plugins submenu
    // (pluginSubFlyout) was open, since it belonged to the old group's items.
    onOpenGroupIndexChanged: {
        pluginSubOpenDelay.stop()
        pluginSubFlyout.close()
        pluginSubIndex = -1
    }
    onClosed: {
        openDelay.stop(); pluginSubOpenDelay.stop()
        flyout.close(); pluginSubFlyout.close()
        openGroupIndex = -1; pluginSubIndex = -1
    }

    MenuFlyout {
        id: flyout
        // A declarative binding (not an imperative copy) so it stays live —
        // see _openRawItems. Group rows (the Plugins entry's per-submenu rows)
        // pass their raw `group` straight through — MenuFlyout doesn't need to
        // understand it, only pluginSubFlyout below does.
        items: menu._openRawItems.map(function(it) {
            if (it.group) return { icon: it.icon || "", label: it.label || "", group: it.group }
            if (it.custom) return { custom: it.custom }
            return menu._mapLeaf(it)
        })
        onItemActivated: (i) => menu._act(menu._openRawItems[i].action)
        onGroupHoverChanged: (i, hovered) => {
            if (hovered) { pluginSubOpenDelay.pendingIndex = i; pluginSubOpenDelay.restart() }
            else if (pluginSubOpenDelay.pendingIndex === i) pluginSubOpenDelay.stop()
        }
        onGroupActivated: (i) => { pluginSubOpenDelay.stop(); menu._activatePluginSub(i) }
        onCustomActivated: (i, action) => menu._act(action)
    }

    // Second flyout level: only the Plugins entry ever has group rows (a
    // plugin submenu, e.g. "Settings", nested one level under Plugins itself)
    // — one extra flyout instance covers that, rather than making MenuFlyout
    // generically self-nesting.
    property int pluginSubIndex: -1
    Timer {
        id: pluginSubOpenDelay
        interval: 300
        property int pendingIndex: -1
        onTriggered: menu._activatePluginSub(pendingIndex)
    }
    function _activatePluginSub(idx) {
        var raw = menu._openRawItems[idx]
        if (!raw || !raw.group) return
        menu.pluginSubIndex = idx
        pluginSubFlyout.openBeside(flyout.rowItemAt(idx))
    }
    MenuFlyout {
        id: pluginSubFlyout
        items: {
            var raw = menu._openRawItems[menu.pluginSubIndex]
            return (raw && raw.group) ? raw.group.items.map(menu._mapLeaf) : []
        }
        onItemActivated: (i) => {
            var raw = menu._openRawItems[menu.pluginSubIndex]
            if (raw && raw.group) menu._act(raw.group.items[i].action)
        }
    }

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
            // One row per group (File / Plugins / Edit / View / Help, plus any
            // table-scoped plugin submenu groups for the open record) — each is
            // a submenu trigger that opens `flyout` beside it, rather than an
            // inline bold header followed by its items. Keeps the top-level
            // menu short regardless of how many plugins are installed.
            Repeater {
                id: triggerRepeater
                model: menu._allGroups
                delegate: MenuRow {
                    required property var modelData
                    required property int index
                    label: modelData.name
                    showChevron: true
                    highlighted: menu.openGroupIndex === index
                    onHoveredChanged: {
                        if (hovered) {
                            openDelay.pendingIndex = index
                            openDelay.restart()
                        } else if (openDelay.pendingIndex === index) {
                            openDelay.stop()
                        }
                    }
                    onActivated: {
                        openDelay.stop()
                        menu._activateGroup(index)
                    }
                }
            }
        }
    }
}
