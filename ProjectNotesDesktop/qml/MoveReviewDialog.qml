// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Confirms a tracker item → project move when it needs the user's attention:
// the item number will be renumbered (a collision in the destination
// project), or the item's linked meeting will be unlinked (meetings are
// project-scoped and can't follow the item). Opened only by
// Main.requestTrackerItemMove() — a plain move (including any silent
// destination-team auto-add) skips this dialog and applies immediately.
Popup {
    id: dlg

    modal: true
    dim: true
    padding: 0
    parent: Overlay.overlay
    scale: Theme.uiScale   // match the zoomed workspace (centered origin)
    width: 340
    height: content.implicitHeight + topPadding + bottomPadding
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0

    property string _itemId: ""
    property string _projectId: ""
    property var    _info: ({})

    // info is the QVariantMap returned by DesktopAppController.checkTrackerItemMove().
    function openFor(itemId, projectId, info) {
        dlg._itemId = itemId
        dlg._projectId = projectId
        dlg._info = info
        open()
    }

    function _confirm() {
        // Drag/drop has no meeting-picker step, so the linked meeting (if any)
        // is simply cleared — meetings are project-scoped.
        DesktopAppController.moveTrackerItem(dlg._itemId, dlg._projectId, "")
        dlg._dismiss()
    }
    // Close on the next tick rather than synchronously inside the tap handler —
    // see FilterDialog._dismiss() for why (avoids the tap falling through to
    // whatever record row sits behind the modal).
    function _dismiss() { Qt.callLater(close) }

    background: Rectangle {
        radius: Theme.radiusLg
        color: Theme.bg
        border.color: Theme.border

        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons }
    }

    contentItem: ColumnLayout {
        id: content
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 13
            spacing: 8
            MaterialIcon { name: "drive_file_move"; size: 17; color: Theme.accent; Layout.alignment: Qt.AlignVCenter }
            Text {
                text: qsTr("Move Tracker Item")
                color: Theme.text; font.pixelSize: 14; font.weight: Font.Bold
                Layout.fillWidth: true
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 13
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: qsTr("Move this item to %1?").arg(dlg._info.projectName || "")
                color: Theme.text; font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                visible: dlg._info.willRenumber === true
                text: qsTr("Item number will change from %1 to %2 — %2 is the next available number in the destination project.")
                        .arg(dlg._info.oldNumber || "").arg(dlg._info.newNumber || "")
                color: Theme.text2; font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                visible: dlg._info.willClearMeeting === true
                text: qsTr("This item is linked to the meeting “%1” — that link will be removed, since meetings are specific to one project.")
                        .arg(dlg._info.meetingTitle || "")
                color: Theme.text2; font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                visible: (dlg._info.membersToAdd || []).length > 0
                text: qsTr("Will also be added to the destination project's team: %1")
                        .arg((dlg._info.membersToAdd || []).map(function(m){ return m.name }).join(", "))
                color: Theme.text3; font.pixelSize: 10
                wrapMode: Text.WordWrap
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 13
            spacing: 8
            Item { Layout.fillWidth: true }
            Rectangle {
                implicitWidth: 70; implicitHeight: 28; radius: Theme.radiusSm
                color: cancelHover.hovered ? Theme.surface2 : "transparent"
                border.color: Theme.border
                Text { anchors.centerIn: parent; text: qsTr("Cancel"); color: Theme.text2; font.pixelSize: 12 }
                HoverHandler { id: cancelHover }
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: dlg._dismiss() }
            }
            Rectangle {
                implicitWidth: 78; implicitHeight: 28; radius: Theme.radiusSm
                color: moveHover.hovered ? Theme.accentStrong : Theme.accent
                Text { anchors.centerIn: parent; text: qsTr("Move"); color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold }
                HoverHandler { id: moveHover }
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: dlg._confirm() }
            }
        }
    }
}
