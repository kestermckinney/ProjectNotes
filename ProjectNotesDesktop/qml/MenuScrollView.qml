// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Scrollable menu body shared by every hand-rolled popup menu (AppMenu,
// RecordContextMenu, SortMenu, MenuFlyout) — a ScrollView around a
// ColumnLayout of rows, plus a top/bottom fade-and-chevron hint that appears
// once the popup's own maxMenuHeight has clipped the row list. Without it, a
// scrolled menu looks identical to one short enough to show every row, and
// there's no native menu-bar affordance to fall back on since these are all
// hand-rolled Popups rather than QtQuick.Controls Menu instances — a user who
// doesn't happen to nudge the mouse onto the thin auto-hide scrollbar has no
// way to tell more rows exist off-screen.
//
// Drop-in replacement for the old per-menu
//   contentItem: ScrollView {
//       clip: true
//       ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
//       ColumnLayout { id: _content; width: <popup>.availableWidth; spacing: 0; ... }
//   }
// — put the same rows as this item's default content (still spacing: 0 by
// default) and reference `<id>.implicitHeight` / `<id>.implicitWidth` in the
// host popup's own height/width bindings where it used to reference its
// local `_content`.
Item {
    id: root

    default property alias content: _content.data
    property alias spacing: _content.spacing
    // Natural (unclipped) content size, for the host popup's own
    // `height: Math.min(implicitHeight + ..., maxMenuHeight)` and
    // `width: Math.max(..., Math.min(implicitWidth + ..., ...))` bindings —
    // mirrors what Popup.implicitContentHeight/Width read off a plain
    // ScrollView contentItem, so this is a transparent swap.
    implicitWidth: _content.implicitWidth
    implicitHeight: _content.implicitHeight

    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            id: _content
            width: scroll.availableWidth
            spacing: 0
        }
    }

    // ScrollBar.vertical is created automatically by ScrollView even while
    // its transient handle is hidden, so position/size (both 0..1) are live
    // regardless of whether the bar itself is currently drawn.
    readonly property real _barPos:  scroll.ScrollBar.vertical.position
    readonly property real _barSize: scroll.ScrollBar.vertical.size
    readonly property bool _moreAbove: _barSize < 1 && _barPos > 0.001
    readonly property bool _moreBelow: _barSize < 1 && _barPos + _barSize < 0.999

    Rectangle {
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 18
        visible: root._moreAbove
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.surface }
            GradientStop { position: 1.0; color: Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0) }
        }
        MaterialIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            name: "expand_less"; size: 14; color: Theme.text3
        }
    }
    Rectangle {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 18
        visible: root._moreBelow
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0) }
            GradientStop { position: 1.0; color: Theme.surface }
        }
        MaterialIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            name: "expand_more"; size: 14; color: Theme.text3
        }
    }
}
