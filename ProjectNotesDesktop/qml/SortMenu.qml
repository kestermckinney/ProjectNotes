// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Sort picker popup — one shared instance (Main.qml), opened via
// openFor(section, sx, sy) with scene coordinates (Overlay.overlay space,
// same convention as RecordContextMenu.openAt()) — coordinates, not an Item
// reference, since triggers live on both sides of StackView page boundaries
// (the top bar in Main.qml's own scope, but also each project sub-tab's
// SectionBar chip inside a pushed ProjectDetailPage — an Item id can't cross
// that boundary, a couple of numbers can, routed up via a signal the same way
// FilterDialog's subFilterRequested already does it).
//
// Section-driven like FilterDialog (not a dumb items-in/events-out component
// like MenuFlyout): resolves its own model, column list, and current sort
// straight from DesktopAppController, and dispatches applySort()/clearSort()
// itself — every trigger point just calls openFor(), no per-call site wiring.
//
// Deliberately a lightweight popup, not a modal dialog like FilterDialog:
// sort is one column + one direction, not a multi-column editor. Picking a
// column or a direction doesn't close the menu — like RecordContextMenu's
// Quick Filter, both are freely re-choosable in one sitting; only Clear Sort
// or dismissing the popup (click-away/Escape) ends the session.
Popup {
    id: menu

    property string _section: ""
    property var    _model: null
    property var    _cols: []          // [{field,label}]
    property string _field: ""
    property bool   _descending: false

    function openFor(section, sx, sy) {
        _section = section
        _model = DesktopAppController.modelForSection(section)
        if (!_model) return
        _cols = DesktopAppController.sortColumns(_model)
        var active = DesktopAppController.activeSort(_model)
        _field = active.field || ""
        _descending = active.descending === true
        // sx,sy are scene (Overlay.overlay) coordinates — clamp inside the
        // overlay bounds, same as RecordContextMenu.openAt().
        var sw = width * Theme.uiScale
        var sh = height * Theme.uiScale
        x = Math.max(6, Math.min(sx, (parent ? parent.width  : sx + sw) - sw - 6))
        y = Math.max(6, Math.min(sy, (parent ? parent.height : sy + sh) - sh - 6))
        open()
    }

    function _pick(field) {
        menu._field = field
        DesktopAppController.applySort(menu._model, field, menu._descending)
    }
    function _setDirection(desc) {
        menu._descending = desc
        if (menu._field !== "")
            DesktopAppController.applySort(menu._model, menu._field, desc)
    }
    function _clearSort() {
        DesktopAppController.clearSort(menu._model)
        menu._field = ""
        menu._descending = false
        menu.close()
    }

    // The column list can grow the popup after it's already open (rare, but
    // matches RecordContextMenu.onHeightChanged) — keep it on-screen.
    onHeightChanged: if (visible && parent)
        y = Math.max(6, Math.min(y, parent.height - height * Theme.uiScale - 6))

    parent: Overlay.overlay
    modal: true
    dim: false
    padding: 3
    width: Math.max(170, Math.min(implicitContentWidth + leftPadding + rightPadding, 265))
    property int maxMenuHeight: 420
    height: Math.min(_scroll.implicitHeight + topPadding + bottomPadding, maxMenuHeight)
    scale: Theme.uiScale
    transformOrigin: Item.TopLeft

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    // Clicking away dismisses the menu and nothing else — see ClickShield.qml.
    ClickShield { host: menu }

    contentItem: MenuScrollView {
        id: _scroll

        Text {
            text: qsTr("SORT BY"); color: Theme.text3
            font.pixelSize: 10; font.weight: Font.Bold
            Layout.leftMargin: 9; Layout.topMargin: 2; Layout.bottomMargin: 3
        }

        Repeater {
            model: menu._cols
            delegate: MenuRow {
                required property var modelData
                label: modelData.label || ""
                toggle: true
                checked: modelData.field === menu._field
                onActivated: menu._pick(modelData.field)
            }
        }

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 1
            color: Theme.borderSoft; Layout.topMargin: 3; Layout.bottomMargin: 3
        }

        MenuRow {
            icon: "arrow_upward"; label: qsTr("Ascending")
            toggle: true; checked: !menu._descending
            onActivated: menu._setDirection(false)
        }
        MenuRow {
            icon: "arrow_downward"; label: qsTr("Descending")
            toggle: true; checked: menu._descending
            onActivated: menu._setDirection(true)
        }

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 1
            color: Theme.borderSoft; Layout.topMargin: 3; Layout.bottomMargin: 3
        }

        MenuRow {
            icon: "backspace"; label: qsTr("Clear Sort")
            onActivated: menu._clearSort()
        }
    }
}
