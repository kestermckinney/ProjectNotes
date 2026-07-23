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
    property bool   canDelete: true
    property bool   canExport: true

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
    parent: Overlay.overlay

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    // Open at a scene/window coordinate (kept inside the overlay bounds).
    function openAt(sx, sy) {
        var maxX = (parent ? parent.width : sx + width) - width - 6
        var maxY = (parent ? parent.height : sy + 320) - 320
        x = Math.max(6, Math.min(sx, maxX))
        y = Math.max(6, Math.min(sy, maxY))
        open()
    }

    function _fire(sig) { close(); sig() }

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
        MenuRow { icon: "add";          label: qsTr("New");                                  onActivated: menu._fire(menu.newRequested) }
        MenuRow { icon: "delete";       label: qsTr("Delete");      visible: menu.canDelete; danger: true; onActivated: menu._fire(menu.deleteRequested) }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft; Layout.topMargin: 3; Layout.bottomMargin: 3 }
        MenuRow { icon: "ios_share";    label: qsTr("Export XML…"); visible: menu.canExport; onActivated: menu._fire(menu.exportRequested) }
        MenuRow { icon: "filter_list";  label: qsTr("Filter…");                              onActivated: menu._fire(menu.filterRequested) }
        MenuRow { icon: "refresh";      label: qsTr("Refresh");                              onActivated: menu._fire(menu.refreshRequested) }
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
