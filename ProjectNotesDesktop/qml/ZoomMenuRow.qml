// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Brave-style zoom control row for AppMenu's View submenu — replaces the old
// three separate Zoom In / Zoom Out / Reset Zoom rows with a single row:
// icon, "Zoom" label, − / <percent> / + steppers, a divider, and a fullscreen
// toggle. Emits one of four action strings rather than a single fixed action
// (that's why this isn't just another MenuRow) — see MenuFlyout's
// `custom: "zoom"` item handling, which hosts this alongside plain MenuRow
// delegates and forwards action to the host menu's _act().
Rectangle {
    id: zr

    signal action(string name)

    Layout.fillWidth: true
    implicitHeight: 24
    implicitWidth: rowContent.implicitWidth + 12
    radius: Theme.radiusSm
    color: "transparent"

    RowLayout {
        id: rowContent
        anchors.fill: parent
        anchors.leftMargin: 6; anchors.rightMargin: 6
        spacing: 4

        Item {
            Layout.preferredWidth: 14; Layout.preferredHeight: 14
            Layout.alignment: Qt.AlignVCenter
            MaterialIcon { anchors.centerIn: parent; name: "zoom_in"; size: 14; color: Theme.text2 }
        }
        Text {
            text: qsTr("Zoom")
            color: Theme.text
            font.pixelSize: 11
            Layout.fillWidth: true
            verticalAlignment: Text.AlignVCenter
        }

        // − step
        Rectangle {
            implicitWidth: 18; implicitHeight: 18; radius: Theme.radiusSm
            Layout.alignment: Qt.AlignVCenter
            color: minusHover.hovered ? Theme.surface2 : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: "remove"; size: 12; color: Theme.text2 }
            HoverHandler { id: minusHover }
            TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: zr.action("zoom_out") }
        }

        // Percentage readout — click resets to 100%, same as the old
        // Reset Zoom (⌘0) menu item.
        Rectangle {
            implicitWidth: pctText.implicitWidth + 8; implicitHeight: 18; radius: Theme.radiusSm
            Layout.alignment: Qt.AlignVCenter
            color: pctHover.hovered ? Theme.surface2 : "transparent"
            Text {
                id: pctText
                anchors.centerIn: parent
                text: Math.round(Theme.uiScale * 100) + "%"
                color: Theme.text2
                font.pixelSize: 10
            }
            HoverHandler { id: pctHover }
            TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: zr.action("zoom_reset") }
            ToolTip.visible: pctHover.hovered
            ToolTip.text: qsTr("Reset Zoom")
            ToolTip.delay: 400
        }

        // + step
        Rectangle {
            implicitWidth: 18; implicitHeight: 18; radius: Theme.radiusSm
            Layout.alignment: Qt.AlignVCenter
            color: plusHover.hovered ? Theme.surface2 : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: "add"; size: 12; color: Theme.text2 }
            HoverHandler { id: plusHover }
            TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: zr.action("zoom_in") }
        }

        Rectangle {
            Layout.preferredWidth: 1; Layout.preferredHeight: 16
            Layout.alignment: Qt.AlignVCenter
            color: Theme.border
        }

        // Fullscreen toggle
        Rectangle {
            implicitWidth: 18; implicitHeight: 18; radius: Theme.radiusSm
            Layout.alignment: Qt.AlignVCenter
            color: fsHover.hovered ? Theme.surface2 : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: "fullscreen"; size: 12; color: Theme.text2 }
            HoverHandler { id: fsHover }
            TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: zr.action("toggle_fullscreen") }
            ToolTip.visible: fsHover.hovered
            ToolTip.text: qsTr("Toggle Fullscreen (%1)").arg(AppShortcuts.text("toggle_fullscreen"))
            ToolTip.delay: 400
        }
    }
}
