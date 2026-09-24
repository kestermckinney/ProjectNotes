// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// "You are here" badge for records that live inside a project — meeting notes
// and tracker items. Those detail pages fill the whole content area, so without
// this there is nothing on screen naming the project the record belongs to
// (the breadcrumb only says "Note" / "Item").
//
// Clicking it opens that project, which makes it a shortcut back to the parent
// as well as a label. A record with no project (a tracker item that was never
// assigned one) still gets the badge, greyed out and inert, so the absence
// reads as deliberate rather than as a missing widget.
Rectangle {
    id: chip

    property string projectId: ""

    // Re-read whenever the page swaps the record it is showing.
    readonly property string projectNumber:
        projectId === "" ? "" : DesktopAppController.projectNumberForId(projectId)
    readonly property string projectName:
        projectId === "" ? "" : DesktopAppController.projectNameForId(projectId)
    readonly property bool linked: projectId !== "" && (projectNumber !== "" || projectName !== "")

    signal activated()

    implicitHeight: 26
    implicitWidth: Math.min(row.implicitWidth + 18, 460)
    radius: Theme.radiusSm
    color: chip.linked && hover.hovered ? Theme.accentSoft : Theme.surface
    border.color: chip.linked ? Theme.accent : Theme.border

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.leftMargin: 9
        anchors.right: parent.right
        anchors.rightMargin: 9
        anchors.verticalCenter: parent.verticalCenter
        spacing: 5

        MaterialIcon {
            name: "description"
            size: 14
            color: chip.linked ? Theme.accent : Theme.text3
        }
        Text {
            text: chip.projectNumber
            visible: text !== ""
            color: chip.linked ? Theme.accent : Theme.text3
            font.pixelSize: Theme.fontBody
            font.weight: Font.DemiBold
        }
        Text {
            Layout.fillWidth: true
            text: chip.linked ? chip.projectName : qsTr("No project")
            color: chip.linked ? Theme.text2 : Theme.text3
            font.pixelSize: Theme.fontBody
            font.italic: !chip.linked
            elide: Text.ElideRight
        }
    }

    HoverHandler {
        id: hover
        enabled: chip.linked
        cursorShape: Qt.PointingHandCursor
    }
    TapHandler {
        enabled: chip.linked
        // Exclusive grab so the tap opens the project and does not also reach
        // the page-background MouseArea underneath — same reasoning as
        // KebabButton's handler.
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: chip.activated()
    }

    ToolTip.visible: chip.linked && hover.hovered
    ToolTip.delay: 400
    ToolTip.text: qsTr("Go to project %1 %2").arg(chip.projectNumber).arg(chip.projectName)
}
