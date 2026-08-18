// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// The submenu popup opened beside a group-title row in AppMenu / RecordContextMenu
// — the traditional "File ▸ submenu" flyout that replaced those menus' old bold
// group-title-header-plus-inline-items layout. One instance is shared per host
// menu (only one group can be open at a time); call openBeside() to (re)point it
// at a new trigger row + item list, and close() when the host wants it gone.
//
// Closing is deliberately NOT driven by "did the mouse leave the row/flyout" —
// an earlier hover-grace-timer version of this popup would often vanish while
// the pointer was still crossing from the trigger row to the flyout (a race
// against the timer, worse at high UI zoom or a slow mouse). Like a native menu
// bar, once open it stays open until: a leaf item is chosen, a sibling trigger
// row is hovered/clicked (which repoints this same instance), the host menu
// closes, or the user clicks elsewhere / presses Escape (Popup's own default
// closePolicy handles both).
Popup {
    id: flyout

    // Items: [{icon, label, trailingText, toggle, checked}] for a leaf row, or
    // [{icon, label, group: {name, items}}] for a *nested* submenu trigger row
    // (AppMenu's "Plugins" entry uses this for its per-plugin-submenu rows) —
    // index-addressed. The host binds this declaratively from its own data, so
    // live changes (e.g. a toggle item's checkmark) are reflected without
    // needing to reopen the flyout. This popup stays dumb about what a leaf
    // index *means* or what a group's items contain; dispatch and any further
    // nesting are the host's job — see groupHoverChanged/groupActivated below.
    property var items: []
    signal itemActivated(int index)
    // Fired for rows that carry a `group` instead of being a plain leaf action.
    // The host owns opening a second flyout beside the row for these — see
    // AppMenu.qml's pluginSubFlyout for the one place that currently uses this.
    signal groupHoverChanged(int index, bool hovered)
    signal groupActivated(int index)
    // Fired by rows that carry `custom: "<kind>"` instead of being a plain leaf
    // action — a bespoke row (see ZoomMenuRow) that can emit more than one
    // action depending on which part of the row was clicked. The host maps
    // `action` straight into its own _act()-style dispatch.
    signal customActivated(int index, string action)
    // So the host can position a child flyout beside a specific row.
    function rowItemAt(index) { return rowRepeater.itemAt(index) }

    // Anchor points captured in openBeside(), in Overlay.overlay coordinates.
    // Recomputing position from these (instead of only at open time) keeps the
    // flyout correctly placed if its own size changes once content lays out.
    property real _anchorRightX: 0
    property real _anchorLeftX: 0
    property real _anchorY: 0

    // Deliberately Overlay-parented regardless of which popup hosts the trigger
    // row (AppMenu isn't itself Overlay-parented) — this decouples the flyout's
    // coordinate space from its host, mirroring RecordContextMenu's own existing
    // "parent: Overlay.overlay + clamp to overlay bounds" pattern.
    parent: Overlay.overlay
    // Deliberately *non-modal*, unlike the menu that hosts it: a modal flyout
    // is the only popup the overlay will deliver input to, so a click on one of
    // the host menu's other rows was spent closing the flyout and the row itself
    // stayed unactivated (it took a second click), and a click away from both
    // closed only the flyout, leaving the menu behind. Non-modal, the host menu
    // stays the input target underneath: sibling rows activate on the first
    // click, and a click outside dismisses flyout and menu together. The host's
    // ClickShield keeps that dismissal from reaching the page behind the menu.
    modal: false
    dim: false
    padding: 3
    width: Math.max(160, Math.min(implicitContentWidth + leftPadding + rightPadding, 340))
    property int maxFlyoutHeight: 480
    height: Math.min(_scroll.implicitHeight + topPadding + bottomPadding, maxFlyoutHeight)
    scale: Theme.uiScale
    transformOrigin: Item.TopLeft

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    // Point the flyout at a trigger row (positioning only — `items` is a
    // binding the host owns, see above). Safe to call while already open — the
    // host uses this to retarget the flyout as the pointer moves between
    // sibling trigger rows, without closing/reopening the popup itself.
    function openBeside(triggerItem) {
        var topRight = triggerItem.mapToItem(Overlay.overlay, triggerItem.width, 0)
        var topLeft = triggerItem.mapToItem(Overlay.overlay, 0, 0)
        _anchorRightX = topRight.x
        _anchorLeftX = topLeft.x
        _anchorY = topRight.y
        _reposition()
        if (!flyout.opened) open()
    }

    // Open to the right of the trigger by default; flip to its left if that
    // would overflow the window, and clamp vertically — same idea as
    // RecordContextMenu.openAt()'s clamp, generalized to react to size changes
    // too (this popup's width/height depend on content, unlike that one's fixed
    // 232px width).
    function _reposition() {
        if (!Overlay.overlay) return
        var sw = width * Theme.uiScale
        var sh = height * Theme.uiScale
        var ox = _anchorRightX
        if (ox + sw > Overlay.overlay.width - 6)
            ox = _anchorLeftX - sw
        x = Math.max(6, Math.min(ox, Overlay.overlay.width - sw - 6))
        y = Math.max(6, Math.min(_anchorY, Overlay.overlay.height - sh - 6))
    }
    onWidthChanged: _reposition()
    onHeightChanged: _reposition()

    contentItem: MenuScrollView {
        id: _scroll
        Repeater {
            id: rowRepeater
            model: flyout.items
            // Both row kinds are declared unconditionally (not swapped in via
            // a Loader) so `modelData`/`index` stay ordinary lexically-scoped
            // properties of this one delegate, same as every other row here —
            // a Loader would parent the loaded item to itself, needing
            // parent.modelData indirection instead, for no real benefit given
            // how few items ever populate one of these flyouts.
            delegate: Item {
                id: rowWrap
                required property var modelData
                required property int index
                Layout.fillWidth: true
                implicitHeight: modelData.custom === "zoom" ? zoomRow.implicitHeight : leafRow.implicitHeight
                implicitWidth: modelData.custom === "zoom" ? zoomRow.implicitWidth : leafRow.implicitWidth

                MenuRow {
                    id: leafRow
                    anchors.fill: parent
                    visible: rowWrap.modelData.custom === undefined
                    icon: modelData.icon || ""
                    label: modelData.label || ""
                    trailingText: modelData.trailingText || ""
                    toggle: modelData.toggle === true
                    checked: modelData.checked === true
                    showChevron: modelData.group !== undefined
                    onHoveredChanged: if (modelData.group !== undefined)
                                          flyout.groupHoverChanged(index, hovered)
                    onActivated: modelData.group !== undefined ? flyout.groupActivated(index)
                                                                : flyout.itemActivated(index)
                }
                ZoomMenuRow {
                    id: zoomRow
                    anchors.fill: parent
                    visible: rowWrap.modelData.custom === "zoom"
                    onAction: (name) => flyout.customActivated(index, name)
                }
            }
        }
    }
}
