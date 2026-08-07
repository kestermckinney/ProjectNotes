// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Row right-click menu. Exposes the same record actions the Widgets table-view
// context menu offers (Open · New · Delete · Export · Filter · Refresh), styled to
// match the mockup's plugin-aware context menu. Open at the cursor via openAt().
Popup {
    id: menu

    property string recordType: qsTr("Record")  // header title, e.g. "Person"
    property string recordLabel: ""              // the row's display label
    property bool   canOpen: true
    property bool   canNew: true
    property bool   canDelete: true
    property bool   canDuplicate: false
    property bool   canMoveTo: false
    property bool   canExport: true
    property bool   canFilter: true
    property bool   canRefresh: true
    // Quick Filter: pre-filled column-filter shortcuts for the row this menu
    // was opened for — set by the page (from the clicked row's own field
    // values) just before calling openAt(). Each entry: {icon, label, field,
    // values}. Empty hides the trigger row entirely — pages that don't wire
    // Quick Filter (or a row with nothing fillable, e.g. no client set)
    // don't need to opt out explicitly.
    property var    quickFilters: []
    readonly property bool canQuickFilter: quickFilters.length > 0

    // True when any of the top-group actions (Open/New/Delete/Duplicate/Move To)
    // is present — drives the divider that separates that group from
    // Export/Filter/Refresh, so the menu doesn't open with a stray divider when
    // the top group is fully hidden.
    readonly property bool _hasTopGroup: canOpen || canNew || canDelete || canDuplicate || canMoveTo

    // Plugin menus: the list model this row belongs to + the row's id. When set,
    // openAt() queries the controller for plugin menus whose table matches, and
    // lists them below the built-in actions (like the Widgets table-view menu).
    property var    model: null
    property string recordId: ""
    // Alternative to `model` for heterogeneous lists (search results): when set,
    // plugins are resolved/run by this table name instead of the model's table.
    property string recordTable: ""
    property var    _plugins: []
    // All of this record's plugin menus collapse into a single "Plugins" entry
    // (icon/label + nested items), mirroring AppMenu.qml's globalPluginGroup:
    // {name, items}, where items are either plain leaf entries (no submenu) or
    // nested group rows ({icon,label,group:{name,items}}) for plugins that
    // share a submenu. null when the record has no plugin menus at all.
    property var    pluginGroup: null

    signal openRequested()
    signal newRequested()
    signal deleteRequested()
    signal duplicateRequested()
    signal moveToRequested()
    signal exportRequested()
    signal filterRequested()
    // Carries the menu's own scene position (Overlay.overlay space, same as
    // openAt()'s sx,sy) as the anchor for SortMenu — it's a shared instance
    // living in Main.qml, section-driven and positioned by coordinates rather
    // than an Item reference (see SortMenu.qml's doc comment).
    signal sortRequested(real sx, real sy)
    signal refreshRequested()

    modal: true
    dim: false
    padding: 5
    width: 232
    // Cap the popup height and let it scroll once the plugin/menu list grows
    // past that — otherwise a table with many plugins produces a menu taller
    // than the window. Also clamped to the overlay so the drawn (scaled) menu
    // always fits on-screen.
    readonly property int maxMenuHeight:
        Math.min(480, parent ? parent.height / Theme.uiScale - 12 : 480)
    height: Math.min(_scroll.implicitHeight + topPadding + bottomPadding, maxMenuHeight)
    // Deliberately an in-scene popup (default Popup.Item) — popupType:
    // Popup.Window (Qt 6.10) forwards clicks on the menu rows to the main window
    // underneath, activating whatever record sits behind the menu instead of the
    // row's action, so a detached window is unusable here.
    parent: Overlay.overlay
    scale: Theme.uiScale            // match the zoomed workspace
    transformOrigin: Item.TopLeft   // grow down-right from the cursor anchor

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    // Open at a scene/window coordinate (kept inside the overlay bounds).
    function openAt(sx, sy) {
        if (typeof DesktopAppController === "undefined")
            menu._plugins = []
        else if (menu.recordTable !== "")
            menu._plugins = DesktopAppController.pluginMenusForTable(menu.recordTable)
        else if (menu.model)
            menu._plugins = DesktopAppController.pluginMenusForModel(menu.model)
        else
            menu._plugins = []
        menu.pluginGroup = menu._buildPluginGroup(menu._plugins)
        // sx,sy are scene (window) coordinates — the overlay's own space. The
        // menu is drawn at Theme.uiScale, so clamp with its scaled footprint to
        // keep it inside the window.
        var sw = width * Theme.uiScale
        var sh = height * Theme.uiScale
        x = Math.max(6, Math.min(sx, (parent ? parent.width  : sx + sw) - sw - 6))
        y = Math.max(6, Math.min(sy, (parent ? parent.height : sy + sh) - sh - 6))
        open()
    }

    // The plugin section makes the menu taller; keep it on-screen once laid out.
    onHeightChanged: if (visible && parent)
        y = Math.max(6, Math.min(y, parent.height - height * Theme.uiScale - 6))

    function _fire(sig) { close(); sig() }
    function _runPlugin(idx) {
        close()
        if (menu.recordTable !== "")
            DesktopAppController.runPluginMenuForTable(menu.recordTable, menu.recordId, idx)
        else
            DesktopAppController.runPluginMenu(menu.model, menu.recordId, idx)
    }

    // Group the flat plugin list by submenu into a single "Plugins" entry,
    // mirroring AppMenu.qml's globalPluginGroup: entries with no submenu become
    // plain leaf rows; entries that share a submenu become a nested group row
    // (see MenuFlyout's group support and pluginSubFlyout below). First-seen
    // submenu order is preserved.
    function _buildPluginGroup(list) {
        var order = []
        var byKey = ({})
        for (var i = 0; i < list.length; i++) {
            var m = list[i]
            var key = (m.submenu && m.submenu !== "") ? m.submenu : ""
            if (byKey[key] === undefined) { byKey[key] = []; order.push(key) }
            byKey[key].push({ icon: "extension", label: m.title, index: m.index })
        }
        var items = []
        for (var j = 0; j < order.length; j++) {
            var k = order[j]
            if (k === "")
                items = items.concat(byKey[k])
            else
                items.push({ icon: "extension", label: k, group: { name: k, items: byKey[k] } })
        }
        return items.length > 0 ? { name: qsTr("Plugins"), items: items } : null
    }

    // Whether the (single) Plugins trigger row's flyout is open. Mirrors
    // AppMenu.qml's group-trigger state machine — see there for the fuller
    // rationale on why closing isn't hover-driven (an earlier hover-grace-timer
    // version raced the pointer crossing from the row to the flyout and would
    // often vanish before it could be clicked).
    property bool pluginOpen: false
    Timer {
        id: openDelay
        interval: 300
        onTriggered: menu._activatePluginGroup()
    }
    function _activatePluginGroup() {
        if (!menu.pluginGroup) return
        menu.pluginOpen = true
        flyout.openBeside(pluginTriggerRow)
    }
    onClosed: {
        openDelay.stop(); pluginSubOpenDelay.stop(); qfOpenDelay.stop()
        flyout.close(); pluginSubFlyout.close(); qfFlyout.close()
        pluginOpen = false; pluginSubIndex = -1; quickFilterOpen = false
    }

    // ── Quick Filter ─────────────────────────────────────────────────────────
    // One flyout level (unlike Plugins' two) — a flat list of pre-filled
    // column-filter shortcuts plus a trailing "Clear Filters" row. Mirrors the
    // Plugins trigger/flyout pattern above.
    property bool quickFilterOpen: false
    Timer {
        id: qfOpenDelay
        interval: 300
        onTriggered: menu._activateQuickFilter()
    }
    function _activateQuickFilter() {
        if (!menu.canQuickFilter) return
        menu.quickFilterOpen = true
        qfFlyout.openBeside(quickFilterTriggerRow)
    }
    // True if `spec`'s field/values (from activeColumnFilters) exactly match
    // a quick-filter entry — drives its checkmark.
    function _quickFilterMatches(qf, activeSpecs) {
        for (var i = 0; i < activeSpecs.length; i++) {
            if (activeSpecs[i].field !== qf.field) continue
            var v = activeSpecs[i].values
            if (v.length !== qf.values.length) continue
            var same = true
            for (var j = 0; j < v.length; j++) {
                if (v[j] !== qf.values[j]) { same = false; break }
            }
            if (same) return true
        }
        return false
    }
    MenuFlyout {
        id: qfFlyout
        items: {
            // Reading filterRev makes this binding re-evaluate whenever any
            // quick/manual filter changes, so checkmarks stay live while the
            // menu is open (activeColumnFilters() itself has no change
            // notification of its own — see DesktopAppController.filterRev).
            var rev = DesktopAppController.filterRev
            var activeSpecs = menu.model ? DesktopAppController.activeColumnFilters(menu.model) : []
            var list = menu.quickFilters.map(function(qf) {
                return { icon: qf.icon || "filter_alt", label: qf.label || "",
                         trailingText: "", toggle: true,
                         checked: menu._quickFilterMatches(qf, activeSpecs) }
            })
            list.push({ icon: "filter_alt_off", label: qsTr("Clear Filters"),
                        trailingText: "", toggle: false, checked: false })
            return list
        }
        onItemActivated: (i) => {
            if (i === menu.quickFilters.length) {
                DesktopAppController.clearColumnFilters(menu.model)
                menu.close()
                return
            }
            var qf = menu.quickFilters[i]
            DesktopAppController.applyQuickFilter(menu.model, qf.field, qf.values)
            // Deliberately don't close — quick filters are checkable/
            // stackable (like AppMenu's View toggles), so the user can tick
            // several from one open menu; `items` above re-evaluates live.
        }
    }

    MenuFlyout {
        id: flyout
        // Plugins entry items are either plain leaf entries ({icon,label,index})
        // or nested group rows ({icon,label,group}); the flyout expects
        // {icon, label, trailingText, toggle, checked} for leaves and passes
        // group rows straight through — see AppMenu.qml's identical mapping.
        items: (menu.pluginGroup ? menu.pluginGroup.items : []).map(function(it) {
            if (it.group) return { icon: it.icon || "", label: it.label || "", group: it.group }
            return { icon: it.icon || "extension", label: it.label || "", trailingText: "", toggle: false, checked: false }
        })
        onItemActivated: (i) => menu._runPlugin(menu.pluginGroup.items[i].index)
        onGroupHoverChanged: (i, hovered) => {
            if (hovered) { pluginSubOpenDelay.pendingIndex = i; pluginSubOpenDelay.restart() }
            else if (pluginSubOpenDelay.pendingIndex === i) pluginSubOpenDelay.stop()
        }
        onGroupActivated: (i) => { pluginSubOpenDelay.stop(); menu._activatePluginSub(i) }
    }

    // Second flyout level: a nested submenu row (a plugin submenu shared by
    // more than one entry) opens beside the first flyout, one level deeper —
    // see AppMenu.qml's pluginSubFlyout for the identical pattern.
    property int pluginSubIndex: -1
    Timer {
        id: pluginSubOpenDelay
        interval: 300
        property int pendingIndex: -1
        onTriggered: menu._activatePluginSub(pendingIndex)
    }
    function _activatePluginSub(idx) {
        var raw = menu.pluginGroup ? menu.pluginGroup.items[idx] : null
        if (!raw || !raw.group) return
        menu.pluginSubIndex = idx
        pluginSubFlyout.openBeside(flyout.rowItemAt(idx))
    }
    MenuFlyout {
        id: pluginSubFlyout
        items: {
            var raw = menu.pluginGroup ? menu.pluginGroup.items[menu.pluginSubIndex] : null
            return (raw && raw.group) ? raw.group.items.map(function(it) {
                return { icon: it.icon || "extension", label: it.label || "", trailingText: "", toggle: false, checked: false }
            }) : []
        }
        onItemActivated: (i) => {
            var raw = menu.pluginGroup ? menu.pluginGroup.items[menu.pluginSubIndex] : null
            if (raw && raw.group) menu._runPlugin(raw.group.items[i].index)
        }
    }

    contentItem: MenuScrollView {
        id: _scroll

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 9; Layout.rightMargin: 9
            Layout.topMargin: 2; Layout.bottomMargin: 5
            spacing: 8
            Text {
                text: menu.recordType.toUpperCase(); color: Theme.text3
                font.pixelSize: 10; font.weight: Font.Bold
            }
            Text {
                text: menu.recordLabel; color: Theme.text2; font.pixelSize: 12
                Layout.fillWidth: true; elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft; Layout.bottomMargin: 3 }

        MenuRow { icon: "open_in_full"; label: qsTr("Open");        visible: menu.canOpen;      onActivated: menu._fire(menu.openRequested) }
        MenuRow { icon: "add";          label: qsTr("New");         visible: menu.canNew;       onActivated: menu._fire(menu.newRequested) }
        MenuRow { icon: "content_copy"; label: qsTr("Duplicate");   visible: menu.canDuplicate; onActivated: menu._fire(menu.duplicateRequested) }
        MenuRow { icon: "drive_file_move"; label: qsTr("Move To…"); visible: menu.canMoveTo;   onActivated: menu._fire(menu.moveToRequested) }
        MenuRow { icon: "delete";       label: qsTr("Delete");      visible: menu.canDelete;    danger: true; onActivated: menu._fire(menu.deleteRequested) }
        Rectangle {
            visible: menu._hasTopGroup
            Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft
            Layout.topMargin: 3; Layout.bottomMargin: 3
        }
        MenuRow { icon: "ios_share";    label: qsTr("Export XML…"); visible: menu.canExport;  onActivated: menu._fire(menu.exportRequested) }
        MenuRow {
            id: quickFilterTriggerRow
            icon: "filter_alt"
            label: qsTr("Quick Filter")
            visible: menu.canQuickFilter
            showChevron: true
            highlighted: menu.quickFilterOpen
            onHoveredChanged: {
                if (hovered) qfOpenDelay.restart()
                else qfOpenDelay.stop()
            }
            onActivated: {
                qfOpenDelay.stop()
                menu._activateQuickFilter()
            }
        }
        MenuRow { icon: "filter_list";  label: qsTr("Filter…");     visible: menu.canFilter;  onActivated: menu._fire(menu.filterRequested) }
        MenuRow {
            icon: "swap_vert"; label: qsTr("Sort…")
            visible: menu.canFilter
            onActivated: { menu.close(); menu.sortRequested(menu.x, menu.y) }
        }
        MenuRow { icon: "refresh";      label: qsTr("Refresh");     visible: menu.canRefresh; onActivated: menu._fire(menu.refreshRequested) }

        // Plugin menus for this table (dataexport == model table), like the
        // Widgets right-click, collapsed into a single "Plugins" trigger row
        // (see _buildPluginGroup) that opens `flyout` beside it — submenus
        // nest one level deeper via pluginSubFlyout — rather than one
        // top-level row per plugin submenu.
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            visible: menu.pluginGroup !== null

            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 1
                color: Theme.borderSoft; Layout.topMargin: 3; Layout.bottomMargin: 3
            }
            MenuRow {
                id: pluginTriggerRow
                icon: "extension"
                label: menu.pluginGroup ? menu.pluginGroup.name : ""
                showChevron: true
                highlighted: menu.pluginOpen
                onHoveredChanged: {
                    if (hovered) openDelay.restart()
                    else openDelay.stop()
                }
                onActivated: {
                    openDelay.stop()
                    menu._activatePluginGroup()
                }
            }
        }
    }
}
