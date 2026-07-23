// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Left navigation rail: brand, primary destinations, sync, theme toggle, avatar.
Rectangle {
    id: rail
    color: Theme.rail

    property string currentSection: "projects"
    signal sectionActivated(string section)
    // Navigation-style menu actions bubbled up to the shell (Main.qml).
    signal menuAction(string action)

    implicitWidth: Theme.railWidth

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        spacing: 4

        // Application ("hamburger") menu
        Item {
            id: menuBtn
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 40; implicitHeight: 40
            Rectangle {
                anchors.centerIn: parent
                width: 38; height: 38; radius: 9
                color: appMenu.opened ? Theme.surface2
                                      : (menuHover.hovered ? Theme.surface2 : "transparent")
                MaterialIcon { anchors.centerIn: parent; name: "menu"; size: 22; color: Theme.text }
            }
            HoverHandler { id: menuHover }
            TapHandler { onTapped: appMenu.opened ? appMenu.close() : appMenu.open() }

            AppMenu {
                id: appMenu
                x: 46
                y: 0
                onTriggered: (a) => rail.menuAction(a)
            }
        }

        // Brand mark
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 4
            Layout.bottomMargin: 8
            width: 30; height: 30; radius: 8
            color: Theme.accent
            MaterialIcon { anchors.centerIn: parent; name: "description"; size: 19; color: "#ffffff" }
        }

        RailButton { icon: "description"; section: "projects" }
        RailButton { icon: "task_alt";    section: "items" }
        RailButton { icon: "group";       section: "people" }
        RailButton { icon: "apartment";   section: "clients" }
        RailButton { icon: "search";      section: "search" }

        Item { Layout.fillHeight: true }

        // Cloud sync indicator + trigger
        SyncRailButton {}

        // Theme toggle
        RailButton {
            icon: Theme.dark ? "light_mode" : "dark_mode"
            selectable: false
            onClicked: Theme.toggle()
        }
        RailButton { icon: "settings"; section: "settings" }

        // Avatar
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 6
            width: 30; height: 30; radius: 15
            color: Theme.accentStrong
            Text {
                anchors.centerIn: parent
                text: "PM"
                color: "#ffffff"
                font.pixelSize: 11
                font.bold: true
            }
        }
    }

    // Inline rail button component
    component RailButton: Item {
        id: btn
        property string icon: ""
        property string section: ""
        property bool selectable: true
        signal clicked()

        readonly property bool active: selectable && rail.currentSection === section

        Layout.alignment: Qt.AlignHCenter
        implicitWidth: 40
        implicitHeight: 40

        Rectangle {
            anchors.centerIn: parent
            width: 38; height: 38; radius: 9
            color: btn.active ? Theme.accentSoft
                              : (hover.hovered ? Theme.surface2 : "transparent")

            MaterialIcon {
                anchors.centerIn: parent
                name: btn.icon
                size: 22
                color: btn.active ? Theme.accent : Theme.text2
            }
        }

        HoverHandler { id: hover }
        TapHandler {
            onTapped: {
                if (btn.section !== "")
                    rail.sectionActivated(btn.section)
                btn.clicked()
            }
        }
    }

    // Sync indicator: state-colored cloud icon with a circular progress ring
    // (percentage, same manner as the Widgets status-bar progress). Click to sync.
    component SyncRailButton: Item {
        id: sb
        Layout.alignment: Qt.AlignHCenter
        implicitWidth: 40
        implicitHeight: 40

        readonly property bool active:   DesktopAppController.syncActive
        readonly property bool netError: DesktopAppController.syncNetworkError
        readonly property bool anyError: DesktopAppController.syncHasError || netError

        Rectangle {
            anchors.centerIn: parent
            width: 38; height: 38; radius: 9
            color: sbHover.hovered ? Theme.surface2 : "transparent"
        }

        // Circular progress ring (only while actively syncing without an error).
        Canvas {
            id: ring
            anchors.centerIn: parent
            width: 34; height: 34
            visible: sb.active && !sb.anyError
            property real prog: Math.max(0.03, DesktopAppController.syncProgress)
            property color trackColor: Theme.border
            property color fillColor: Theme.accent
            onProgChanged: requestPaint()
            onVisibleChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                var cx = width / 2, cy = height / 2, r = width / 2 - 2
                ctx.lineWidth = 2.5
                ctx.lineCap = "round"
                ctx.strokeStyle = trackColor
                ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI); ctx.stroke()
                ctx.strokeStyle = fillColor
                ctx.beginPath()
                ctx.arc(cx, cy, r, -Math.PI / 2, -Math.PI / 2 + 2 * Math.PI * prog)
                ctx.stroke()
            }
        }

        MaterialIcon {
            id: syncIcon
            anchors.centerIn: parent
            name: sb.netError ? "cloud_off"
                  : (sb.active ? "sync" : "cloud_done")
            size: 20
            color: sb.netError ? Theme.red
                   : (sb.active ? Theme.accent
                      : (DesktopAppController.syncEnabled ? Theme.green : Theme.text3))

            // Spin the sync glyph while a cycle is running.
            RotationAnimator on rotation {
                running: sb.active && !sb.anyError
                loops: Animation.Infinite
                from: 0; to: 360; duration: 1100
                onRunningChanged: if (!running) syncIcon.rotation = 0
            }
        }

        HoverHandler { id: sbHover }
        TapHandler { onTapped: DesktopAppController.syncNow() }

        ToolTip.visible: sbHover.hovered
        ToolTip.text: DesktopAppController.syncDetail
        ToolTip.delay: 300
    }
}
