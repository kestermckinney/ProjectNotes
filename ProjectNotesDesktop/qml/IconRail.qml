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

    readonly property var contactSyncModel: DesktopAppController.contactSyncSettingsModel
    readonly property bool contactsNeedReview: contactSyncModel && contactSyncModel.reviews.length > 0

    readonly property alias applicationMenu: appMenu
    function closeMenu() { appMenu.close() }

    property string currentSection: "projects"
    // Forwarded to AppMenu so it can add the current record's table-scoped
    // plugin menus (e.g. Export / Templates) alongside the global ones.
    property string pluginMenuTable: ""
    property string pluginMenuRecordId: ""
    signal sectionActivated(string section)
    // Navigation-style menu actions bubbled up to the shell (Main.qml).
    signal menuAction(string action)

    implicitWidth: Theme.railWidth

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 3

        // Application ("hamburger") menu
        Item {
            id: menuBtn
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: Theme.railButtonSize; implicitHeight: Theme.railButtonSize
            Rectangle {
                anchors.centerIn: parent
                width: Theme.railHoverSize; height: Theme.railHoverSize; radius: Theme.radiusSm
                color: appMenu.opened ? Theme.surface2
                                      : (menuHover.hovered ? Theme.surface2 : "transparent")
                MaterialIcon {
                    anchors.centerIn: parent
                    name: "menu"
                    size: Theme.railIconSize
                    weight: Font.Light
                    color: Theme.text
                }
            }
            HoverHandler { id: menuHover }
            TapHandler { onTapped: appMenu.opened ? appMenu.close() : appMenu.open() }

            AppMenu {
                id: appMenu
                objectName: "appMenu"
                x: Theme.railWidth - 8
                y: 0
                // Keep the in-scene popup inside the window (rail.height is the
                // window's logical height; the menu opens ~10px from the top).
                maxMenuHeight: rail.height - 16
                pluginMenuTable: rail.pluginMenuTable
                pluginMenuRecordId: rail.pluginMenuRecordId
                onTriggered: (a) => rail.menuAction(a)
            }
        }

        RailButton { icon: "description"; section: "projects" }
        RailButton { icon: "task_alt";    section: "items" }
        RailButton { icon: "group";       section: "contacts" }
        RailButton { icon: "apartment";   section: "companies" }
        RailButton { icon: "search";      section: "search" }

        Item { Layout.fillHeight: true }

        // Show / hide the project folder panel — the rail counterpart of
        // View ▸ Show Project Folders. Only the projects section shows the
        // panel, so the button is disabled (dimmed) elsewhere rather than
        // moving around as sections change.
        RailButton {
            icon: DesktopAppController.showProjectSidebar ? "left_panel_close" : "left_panel_open"
            selectable: false
            enabled: rail.currentSection === "projects"
            onClicked: DesktopAppController.showProjectSidebar = !DesktopAppController.showProjectSidebar
            tooltip: DesktopAppController.showProjectSidebar ? qsTr("Hide project folders")
                                                             : qsTr("Show project folders")
        }

        // Cloud sync indicator + trigger
        SyncRailButton {}

        // Theme toggle
        RailButton {
            icon: Theme.dark ? "light_mode" : "dark_mode"
            selectable: false
            onClicked: Theme.toggle()
        }
        RailButton { icon: "menu_book"; section: "help" }
        RailButton { icon: "settings"; section: "settings" }

        // Avatar
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 5
            width: Theme.railAvatarSize; height: Theme.railAvatarSize; radius: Theme.railAvatarSize / 2
            color: Theme.accentStrong
            Text {
                anchors.centerIn: parent
                text: DesktopAppController.projectManagerInitials !== ""
                      ? DesktopAppController.projectManagerInitials : "PM"
                color: "#ffffff"
                font.pixelSize: Theme.fontXs
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
        readonly property var destination: AppShortcuts.destination(section)
        property string tooltip: destination
                                 ? destination.label + " (" + AppShortcuts.text(destination.action) + ")"
                                 : ""
        objectName: section !== "" ? "rail_" + section : ""
        signal clicked()

        readonly property bool active: selectable && rail.currentSection === section

        Layout.alignment: Qt.AlignHCenter
        implicitWidth: Theme.railButtonSize
        implicitHeight: Theme.railButtonSize
        opacity: btn.enabled ? 1.0 : 0.4

        Rectangle {
            anchors.centerIn: parent
            width: Theme.railHoverSize; height: Theme.railHoverSize; radius: Theme.radiusSm
            color: btn.active ? Theme.accentSoft
                              : (hover.hovered ? Theme.surface2 : "transparent")

            MaterialIcon {
                anchors.centerIn: parent
                name: btn.icon
                size: Theme.railIconSize
                weight: Font.Light
                color: btn.active ? Theme.accent : Theme.text2
            }
        }

        Rectangle {
            objectName: "railContactReviewDot"
            width: 8; height: 8; radius: 4
            color: Theme.red
            visible: btn.section === "settings" && rail.contactsNeedReview
            x: (btn.width + Theme.railIconSize) / 2 - 3
            y: (btn.height - Theme.railIconSize) / 2 - 3
        }

        HoverHandler { id: hover }
        TapHandler {
            onTapped: {
                if (btn.section !== "")
                    rail.sectionActivated(btn.section)
                btn.clicked()
            }
        }

        ToolTip.visible: btn.tooltip !== "" && hover.hovered
        ToolTip.text: btn.tooltip
        ToolTip.delay: 300
    }

    // Sync indicator: state-colored cloud icon with a circular progress ring
    // (percentage, same manner as the Widgets status-bar progress). Click to sync.
    component SyncRailButton: Item {
        id: sb
        Layout.alignment: Qt.AlignHCenter
        implicitWidth: Theme.railButtonSize
        implicitHeight: Theme.railButtonSize

        readonly property bool active:   DesktopAppController.syncActive
        readonly property bool netError: DesktopAppController.syncNetworkError
        readonly property bool anyError: DesktopAppController.syncHasError || netError

        Rectangle {
            anchors.centerIn: parent
            width: Theme.railHoverSize; height: Theme.railHoverSize; radius: Theme.radiusSm
            color: sbHover.hovered ? Theme.surface2 : "transparent"
        }

        // Circular progress ring (only while actively syncing without an error).
        Canvas {
            id: ring
            anchors.centerIn: parent
            width: Theme.railRingSize; height: Theme.railRingSize
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
                ctx.lineWidth = 2
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
            size: Theme.railSyncIconSize
            weight: Font.Light
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
