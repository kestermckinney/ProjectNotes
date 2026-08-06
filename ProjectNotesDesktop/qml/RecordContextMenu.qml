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
    property var    _sections: []

    signal openRequested()
    signal newRequested()
    signal deleteRequested()
    signal duplicateRequested()
    signal moveToRequested()
    signal exportRequested()
    signal filterRequested()
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
    height: Math.min(_content.implicitHeight + topPadding + bottomPadding, maxMenuHeight)
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
        menu._sections = menu._groupPlugins(menu._plugins)
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

    // Group the flat plugin list by submenu so each submenu becomes its own
    // group-title submenu (see openGroupIndex/flyout below) instead of a
    // "Submenu › Title" prefix. First-seen submenu order is preserved; entries
    // with no submenu collect under a plain "Plugins" group.
    function _groupPlugins(list) {
        var order = []
        var byKey = ({})
        for (var i = 0; i < list.length; i++) {
            var m = list[i]
            var key = (m.submenu && m.submenu !== "") ? m.submenu : ""
            if (byKey[key] === undefined) { byKey[key] = []; order.push(key) }
            byKey[key].push(m)
        }
        var out = []
        for (var j = 0; j < order.length; j++) {
            var k = order[j]
            out.push({ header: (k === "" ? qsTr("Plugins") : k), items: byKey[k] })
        }
        return out
    }

    // Which plugin group (index into _sections) currently has its flyout open,
    // or -1. Mirrors AppMenu.qml's group-trigger state machine — see there for
    // the fuller rationale on why closing isn't hover-driven (an earlier
    // hover-grace-timer version raced the pointer crossing from the row to the
    // flyout and would often vanish before it could be clicked).
    property int openGroupIndex: -1
    readonly property var _openRawItems:
        (openGroupIndex >= 0 && openGroupIndex < _sections.length) ? _sections[openGroupIndex].items : []

    Timer {
        id: openDelay
        interval: 300
        property int pendingIndex: -1
        onTriggered: menu._activateGroup(pendingIndex)
    }
    function _activateGroup(idx) {
        if (idx < 0 || idx >= menu._sections.length) return
        menu.openGroupIndex = idx
        flyout.openBeside(pluginTriggerRepeater.itemAt(idx).triggerRow)
    }
    onClosed: {
        openDelay.stop()
        flyout.close()
        openGroupIndex = -1
    }

    MenuFlyout {
        id: flyout
        // Plugin descriptors carry {title, index, submenu}; the flyout expects
        // {icon, label, trailingText, toggle, checked}.
        items: menu._openRawItems.map(function(it) {
            return { icon: "extension", label: it.title, trailingText: "", toggle: false, checked: false }
        })
        onItemActivated: (i) => menu._runPlugin(menu._openRawItems[i].index)
    }

    contentItem: ScrollView {
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            id: _content
            width: menu.availableWidth
            spacing: 0

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
            MenuRow { icon: "filter_list";  label: qsTr("Filter…");     visible: menu.canFilter;  onActivated: menu._fire(menu.filterRequested) }
            MenuRow { icon: "refresh";      label: qsTr("Refresh");     visible: menu.canRefresh; onActivated: menu._fire(menu.refreshRequested) }

            // Plugin menus for this table (dataexport == model table), like the
            // Widgets right-click. Each submenu is a group-title trigger row
            // (see _groupPlugins/openGroupIndex) that opens `flyout` beside it,
            // rather than an inline bold header followed by its items.
            Repeater {
                id: pluginTriggerRepeater
                model: menu._sections
                delegate: ColumnLayout {
                    required property var modelData
                    required property int index
                    // Repeater.itemAt() returns this wrapper (divider + row), so
                    // expose the row itself for positioning/hover-engagement.
                    property alias triggerRow: row
                    Layout.fillWidth: true
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 1
                        color: Theme.borderSoft; Layout.topMargin: 3; Layout.bottomMargin: 3
                    }
                    MenuRow {
                        id: row
                        icon: "extension"
                        label: modelData.header
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
}
