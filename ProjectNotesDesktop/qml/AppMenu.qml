// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// The application ("hamburger") menu, opened from the icon rail. Mirrors the
// Widgets menu bar (File / Edit / View / Help) so the QML app exposes the same
// functionality. Navigation actions are emitted via triggered(); view toggles
// act directly on the Theme / controller and show a live check mark.
Popup {
    id: menu

    // Emitted for navigation-style actions handled by the shell (Main.qml).
    signal triggered(string action)

    modal: true
    dim: false
    padding: 6
    width: 232
    // Rendered in the window overlay so it can float over the sidebar/content.

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    // group model: { name, items:[{icon,label,key,action, toggle, on}] }
    readonly property var groups: [
        { name: qsTr("File"), items: [
            { icon: "note_add",  label: qsTr("New Record"),  key: "",   action: "new" },
            { icon: "search",    label: qsTr("Search…"),     key: "⌘K", action: "search" },
            { icon: "ios_share", label: qsTr("Export XML…"), key: "",   action: "export" },
            { icon: "download",  label: qsTr("Import XML…"), key: "",   action: "import" },
            { icon: "settings",  label: qsTr("Preferences"), key: "⌘,", action: "preferences" },
            { icon: "sync",      label: qsTr("Sync Now"),    key: "",   action: "sync" },
            { icon: "logout",    label: qsTr("Exit"),        key: "",   action: "exit" },
        ]},
        { name: qsTr("Edit"), items: [
            { icon: "search",       label: qsTr("Find"),        key: "⌘F", action: "find" },
            { icon: "filter_list",  label: qsTr("Filter Data…"),key: "",   action: "filter" },
        ]},
        { name: qsTr("View"), items: [
            { icon: Theme.dark ? "light_mode" : "dark_mode", label: qsTr("Dark Mode"),
              key: "", action: "toggle_theme", toggle: true, on: Theme.dark },
            { icon: "attach_money", label: qsTr("Show Internal / Budget Items"),
              key: "", action: "toggle_internal", toggle: true, on: DesktopAppController.showInternalItems },
            { icon: "folder", label: qsTr("Show Closed Projects"),
              key: "", action: "toggle_closed", toggle: true, on: DesktopAppController.showClosedProjects },
            { icon: "task_alt", label: qsTr("Show Resolved Items"),
              key: "", action: "toggle_resolved", toggle: true, on: DesktopAppController.newAndAssignedOnly },
        ]},
        { name: qsTr("Help"), items: [
            { icon: "info", label: qsTr("About"), key: "", action: "about" },
        ]},
    ]

    function _act(a) {
        switch (a) {
        case "toggle_theme":    Theme.toggle(); return
        case "toggle_internal": DesktopAppController.showInternalItems = !DesktopAppController.showInternalItems; return
        case "toggle_closed":   DesktopAppController.showClosedProjects = !DesktopAppController.showClosedProjects; return
        case "toggle_resolved": DesktopAppController.newAndAssignedOnly = !DesktopAppController.newAndAssignedOnly; return
        default: menu.triggered(a); menu.close()
        }
    }

    contentItem: ColumnLayout {
        spacing: 0
        Repeater {
            model: menu.groups
            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 0
                Text {
                    text: modelData.name.toUpperCase()
                    color: Theme.text3
                    font.pixelSize: 10; font.weight: Font.Bold
                    Layout.leftMargin: 10; Layout.topMargin: 7; Layout.bottomMargin: 3
                }
                Repeater {
                    model: modelData.items
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: 32
                        radius: Theme.radiusSm
                        color: rowHover.hovered ? Theme.surface2 : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10; anchors.rightMargin: 10
                            spacing: 10
                            MaterialIcon {
                                name: modelData.icon; size: 18; color: Theme.text2
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Text {
                                text: modelData.label; color: Theme.text; font.pixelSize: 13
                                Layout.fillWidth: true; elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }
                            // check mark for active toggle items
                            MaterialIcon {
                                visible: modelData.toggle === true && modelData.on === true
                                name: "check"; size: 17; color: Theme.accent
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Text {
                                visible: (modelData.key || "") !== "" && !(modelData.toggle === true)
                                text: modelData.key || ""; color: Theme.text3; font.pixelSize: 11
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        HoverHandler { id: rowHover }
                        TapHandler { onTapped: menu._act(modelData.action) }
                    }
                }
            }
        }
    }
}
