// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import ProjectNotesMobile

// ActiveIndicator — badge for a toolbar button whose feature is currently on
// (an active column filter, or a sort override).
//
// Declare it as a child of the ToolButton and bind the button's icon.color to
// iconColor, so an active filter/sort both tints the glyph and hangs a badge
// off its top-right corner:
//
//     ToolButton {
//         icon.name: "arrow.up.arrow.down"
//         icon.color: sortBadge.iconColor
//         onClicked: sortSheet.openFor(...)
//         ActiveIndicator { id: sortBadge; active: ... }
//     }
//
// The badge carries a ring in the toolbar's own background color so it stays
// readable where it overlaps the glyph underneath.
Rectangle {
    id: root

    property bool active: false

    // Edge length of the glyph the badge hangs off — 25 is the iOS style's
    // ToolButton icon size (see its ToolButton.qml).
    property real iconSize: 25

    // Bind the ToolButton's icon.color to this. The inactive branch repeats
    // what the iOS style's ToolButton would have used on its own.
    readonly property color iconColor: !_button
        ? Theme.accentOrange
        : (active ? Theme.accentOrange
                  : (_button.down ? _button.palette.highlight : _button.palette.button))

    // Every palette read goes through the parent button. A plain Item resolves
    // `palette` against the system palette, not the Controls style's, so
    // reading it here directly would pick up colors that don't match the
    // toolbar at all.
    property T.AbstractButton _button: parent as T.AbstractButton
    readonly property real _corner: iconSize / 2 - 2

    visible: active
    z: 1
    width: 18
    height: 18
    radius: width / 2
    // Matches the iOS style's ToolBar background, which is palette.base in
    // light mode and palette.light in dark (see the style's ToolBar.qml).
    color: !_button ? "transparent"
                    : (Application.styleHints.colorScheme === Qt.Dark ? _button.palette.light
                                                                      : _button.palette.base)

    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
    anchors.verticalCenter: parent ? parent.verticalCenter : undefined
    anchors.horizontalCenterOffset: _corner
    anchors.verticalCenterOffset: -_corner

    Rectangle {
        anchors.centerIn: parent
        width: 13
        height: 13
        radius: width / 2
        color: Theme.accentOrange
    }
}
