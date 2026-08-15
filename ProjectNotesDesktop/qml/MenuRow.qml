// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import ProjectNotesDesktop

// Shared row visual for every hand-rolled popup menu (AppMenu, RecordContextMenu,
// MenuFlyout): a leaf action row (icon + label [+ shortcut/checkmark]) and, with
// showChevron set, a submenu-trigger row (icon + label + trailing chevron_right).
// Extracted from what used to be near-identical copies in AppMenu.qml's inline
// delegate and RecordContextMenu.qml's local `component MenuRow`.
Rectangle {
    id: mr

    property string icon: ""
    property string label: ""
    // Shown right-aligned when set (e.g. a keyboard shortcut like "⌘K").
    // Ignored while toggle or showChevron is set — those own the trailing slot.
    property string trailingText: ""
    property bool danger: false
    // Toggle items (View menu's Dark Mode, Show Closed Projects, …) show a
    // checkmark instead of trailingText when checked.
    property bool toggle: false
    property bool checked: false
    // Submenu-trigger rows (a group's title, replacing the old bold header):
    // shows a trailing chevron instead of trailingText/checkmark.
    property bool showChevron: false
    // Forced highlight — e.g. "this trigger row's flyout is currently open" —
    // kept lit even after the pointer has moved off the row and onto its flyout.
    property bool highlighted: false

    readonly property alias hovered: rHover.hovered
    signal activated()

    Layout.fillWidth: true
    implicitHeight: 24
    // Report the row's natural width (content + the 6px side margins) so a
    // Popup can size itself to its widest row — the RowLayout is anchored, so
    // it won't drive width on its own.
    implicitWidth: rowContent.implicitWidth + 12
    radius: Theme.radiusSm
    color: (rHover.hovered || mr.highlighted) ? Theme.surface2 : "transparent"

    RowLayout {
        id: rowContent
        anchors.fill: parent
        anchors.leftMargin: 6; anchors.rightMargin: 6
        spacing: 6

        // Fixed-size icon slot even when icon is "" (AppMenu's group-trigger
        // rows have no icon), so labels stay column-aligned across rows.
        Item {
            Layout.preferredWidth: 14; Layout.preferredHeight: 14
            Layout.alignment: Qt.AlignVCenter
            MaterialIcon {
                anchors.centerIn: parent
                visible: mr.icon !== ""
                name: mr.icon; size: 14
                color: mr.danger ? Theme.red : Theme.text2
            }
        }
        Text {
            text: mr.label
            color: mr.danger ? Theme.red : Theme.text
            font.pixelSize: 11
            Layout.fillWidth: true; elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        MaterialIcon {
            visible: mr.toggle && mr.checked
            name: "check"; size: 13; color: Theme.accent
            Layout.alignment: Qt.AlignVCenter
        }
        Text {
            visible: mr.trailingText !== "" && !mr.toggle && !mr.showChevron
            text: mr.trailingText; color: Theme.text3; font.pixelSize: 10
            verticalAlignment: Text.AlignVCenter
        }
        MaterialIcon {
            visible: mr.showChevron
            name: "chevron_right"; size: 12; color: Theme.text3
            Layout.alignment: Qt.AlignVCenter
        }
    }

    HoverHandler { id: rHover }
    // Exclusive grab (not a plain passive-grab TapHandler): a passive grab lets
    // the same tap fall through to whatever's behind the menu (a record list, the
    // window underneath), activating it instead of firing the row's action.
    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: mr.activated()
    }
}
