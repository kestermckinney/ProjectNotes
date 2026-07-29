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
    property bool   canExport: true
    property bool   canFilter: true
    property bool   canRefresh: true

    // True when any of the top-group actions (Open/New/Delete) is present — drives
    // the divider that separates that group from Export/Filter/Refresh, so the
    // menu doesn't open with a stray divider when the top group is fully hidden.
    readonly property bool _hasTopGroup: canOpen || canNew || canDelete

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
    signal exportRequested()
    signal filterRequested()
    signal refreshRequested()

    modal: true
    dim: false
    padding: 5
    width: 232
    // A real floating window so the menu can spill past the main-window edges when
    // the plugin list makes it taller than the window (Qt keeps it on-screen).
    // As a detached window it renders at 1x — the workspace zoom (Theme.uiScale)
    // no longer applies, matching the LogViewerWindow precedent.
    // NOTE: a window popup anchors to its PARENT ITEM's surface, so we keep the
    // default parent (the page it's declared in) — NOT Overlay.overlay, which on
    // Wayland collapses the anchor and dumps the menu in a window corner.
    popupType: Popup.Window

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
        // sx,sy are scene (window) coordinates. A window popup positions relative to
        // its parent item's surface, so convert to parent-local coords (this also
        // accounts for the zoomLayer scale). It can still extend past the window
        // edge — Qt keeps it on-screen.
        var lp = menu.parent ? menu.parent.mapFromItem(null, sx, sy) : Qt.point(sx, sy)
        x = lp.x
        y = lp.y
        open()
    }

    function _fire(sig) { close(); sig() }
    function _runPlugin(idx) {
        close()
        if (menu.recordTable !== "")
            DesktopAppController.runPluginMenuForTable(menu.recordTable, menu.recordId, idx)
        else
            DesktopAppController.runPluginMenu(menu.model, menu.recordId, idx)
    }

    // Group the flat plugin list by submenu so each submenu renders as its own
    // group-title header (like the record-type / "PLUGINS" headers) instead of a
    // "Submenu › Title" prefix. First-seen submenu order is preserved; entries
    // with no submenu collect under a plain "Plugins" header.
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

    contentItem: ColumnLayout {
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

        MenuRow { icon: "open_in_full"; label: qsTr("Open");        visible: menu.canOpen;   onActivated: menu._fire(menu.openRequested) }
        MenuRow { icon: "add";          label: qsTr("New");         visible: menu.canNew;    onActivated: menu._fire(menu.newRequested) }
        MenuRow { icon: "delete";       label: qsTr("Delete");      visible: menu.canDelete; danger: true; onActivated: menu._fire(menu.deleteRequested) }
        Rectangle {
            visible: menu._hasTopGroup
            Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft
            Layout.topMargin: 3; Layout.bottomMargin: 3
        }
        MenuRow { icon: "ios_share";    label: qsTr("Export XML…"); visible: menu.canExport;  onActivated: menu._fire(menu.exportRequested) }
        MenuRow { icon: "filter_list";  label: qsTr("Filter…");     visible: menu.canFilter;  onActivated: menu._fire(menu.filterRequested) }
        MenuRow { icon: "refresh";      label: qsTr("Refresh");     visible: menu.canRefresh; onActivated: menu._fire(menu.refreshRequested) }

        // Plugin menus for this table (dataexport == model table), like the
        // Widgets right-click. Each submenu becomes its own group-title header
        // (see _groupPlugins) so nested plugin actions read as clean sections
        // rather than a "Submenu › Title" one-liner.
        Repeater {
            model: menu._sections
            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 1
                    color: Theme.borderSoft; Layout.topMargin: 3; Layout.bottomMargin: 3
                }
                Text {
                    text: modelData.header.toUpperCase(); color: Theme.text3
                    font.pixelSize: 10; font.weight: Font.Bold
                    Layout.leftMargin: 9; Layout.topMargin: 1; Layout.bottomMargin: 2
                }
                Repeater {
                    model: modelData.items
                    delegate: MenuRow {
                        required property var modelData
                        icon: "extension"
                        label: modelData.title
                        onActivated: menu._runPlugin(modelData.index)
                    }
                }
            }
        }
    }

    component MenuRow: Rectangle {
        id: mr
        property string icon: ""
        property string label: ""
        property bool danger: false
        signal activated()
        Layout.fillWidth: true
        implicitHeight: 32
        radius: Theme.radiusSm
        color: rHover.hovered ? Theme.surface2 : "transparent"
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 9; anchors.rightMargin: 9
            spacing: 10
            MaterialIcon {
                name: mr.icon; size: 17
                color: mr.danger ? Theme.red : Theme.text2
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: mr.label
                color: mr.danger ? Theme.red : Theme.text
                font.pixelSize: 13
                Layout.fillWidth: true; elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }
        HoverHandler { id: rHover }
        TapHandler { onTapped: mr.activated() }
    }
}
